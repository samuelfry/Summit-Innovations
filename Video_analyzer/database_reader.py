import os
import asyncio
import threading
from collections import OrderedDict
from datetime import datetime, timezone
import requests
from supabase import acreate_client, AsyncClient
from realtime._async.client import AsyncRealtimeClient
from realtime.types import ChannelStates
import time

# realtime<=2.28.3: _reconnect calls asyncio.wait([]) when nothing is in JOINED/JOINING,
# which raises ValueError. Fixed upstream; patch until a release includes it.
async def _reconnect_fixed(self: AsyncRealtimeClient) -> None:
    self._ws_connection = None

    to_rejoin = [
        chan
        for chan in self.channels.values()
        if chan.state == ChannelStates.JOINED or chan.state == ChannelStates.JOINING
    ]
    for channel in to_rejoin:
        channel.state = ChannelStates.ERRORED

    await self.connect()

    if self.is_connected:
        for chan in to_rejoin:
            await chan._rejoin()


AsyncRealtimeClient._reconnect = _reconnect_fixed

SUPABASE_URL = "https://gctbnsjsridmsilzqtpq.supabase.co"  # https://<project-ref>.supabase.co
SUPABASE_KEY = "sb_publishable_DqbujbP_YcPdU1U4q6XjiA_XjqICfuA"  # anon/publishable is fine for reading public storage + listening
# If you’re using private Realtime, you’d need a user JWT and set_auth, but you said public folder.

WAV_BUCKET = "audio"  # Public bucket name
LOCAL_DIR = "./downloads"

TABLE_SCHEMA = "public"
TABLE_NAME = "wav_files"

OBJECT_PATH_COLUMN = "object_path"  # Column that holds Storage object_name/path
UPDATED_AT_COLUMN = "updated_at" # Column that holds a time stamp of when object was last updated

# In-memory dedupe with TTL + max-size to avoid repeated processing loops
# while keeping memory usage bounded for long-running processes.
_EVENT_TTL_SECONDS = 24 * 60 * 60  # 24 hours
_EVENT_MAX_KEYS = 50000
_PROCESSED_EVENTS = OrderedDict()
_PROCESSED_EVENTS_LOCK = threading.Lock()
_HEARTBEAT_SECONDS = 60
_RECONNECT_BASE_DELAY_SECONDS = 1
_RECONNECT_MAX_DELAY_SECONDS = 30


def _event_key(object_name: str, updated_at: str):
    return object_name, updated_at


def _mark_event_if_new(object_name: str, updated_at: str) -> bool:
    """Return True only if this (object_name, updated_at) has not been seen."""
    key = _event_key(object_name, updated_at)
    now = time.monotonic()
    with _PROCESSED_EVENTS_LOCK:
        # Remove expired entries (oldest first due to OrderedDict insertion order).
        cutoff = now - _EVENT_TTL_SECONDS
        while _PROCESSED_EVENTS:
            oldest_key, oldest_seen = next(iter(_PROCESSED_EVENTS.items()))
            if oldest_seen >= cutoff:
                break
            _PROCESSED_EVENTS.pop(oldest_key)

        if key in _PROCESSED_EVENTS:
            # Refresh recency to preserve active keys when trimming by max size.
            _PROCESSED_EVENTS.move_to_end(key)
            return False

        _PROCESSED_EVENTS[key] = now

        # Enforce max-size bound by evicting oldest entries.
        while len(_PROCESSED_EVENTS) > _EVENT_MAX_KEYS:
            _PROCESSED_EVENTS.popitem(last=False)

        return True


def infer_project_ref(supabase_url: str) -> str:
    # supabase_url like: https://abcd1234.supabase.co
    host = supabase_url.replace("https://", "").replace("http://", "")
    return host.split(".supabase.co")[0]


def public_storage_url(project_ref: str, bucket: str, object_name: str) -> str:
    return f"https://{project_ref}.supabase.co/storage/v1/object/public/{bucket}/{object_name}"


def download_file(url: str, dest_path: str):
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    with requests.get(url, stream=True, timeout=60) as r:
        r.raise_for_status()
        with open(dest_path, "wb") as f:
            for chunk in r.iter_content(chunk_size=1024 * 256):
                if chunk:
                    f.write(chunk)


async def _download_and_process(url: str, dest_path: str, on_file_downloaded=None):
    """Run blocking download/inference work off the asyncio event loop."""
    await asyncio.to_thread(download_file, url, dest_path)
    print("Saved:", dest_path)
    if on_file_downloaded:
        await asyncio.to_thread(on_file_downloaded, dest_path)


async def main(on_file_downloaded=None):
    os.makedirs(LOCAL_DIR, exist_ok=True)

    project_ref = infer_project_ref(SUPABASE_URL)
    supabase: AsyncClient = await acreate_client(SUPABASE_URL, SUPABASE_KEY)

    # Fetch and process existing files on startup
    print("Fetching existing audio files from database...")
    try:
        response = await supabase.table(TABLE_NAME).select(OBJECT_PATH_COLUMN, UPDATED_AT_COLUMN).execute()
        existing_files = response.data
        print(f"Found {len(existing_files)} existing files.")
        
        for file_data in existing_files:
            object_name = file_data.get(OBJECT_PATH_COLUMN)
            existing_updated_at = file_data.get(UPDATED_AT_COLUMN)
            if not object_name:
                continue
            
            url = public_storage_url(project_ref, WAV_BUCKET, object_name)
            filename = os.path.basename(object_name)
            dest_path = os.path.join(LOCAL_DIR, filename)
            if existing_updated_at:
                _mark_event_if_new(object_name, existing_updated_at)
            
            # Skip if already downloaded
            if os.path.exists(dest_path):
                print(f"File already exists: {dest_path}")
                continue
            
            print(f"Downloading existing file: {object_name} -> {dest_path}")
            try:
                await _download_and_process(url, dest_path, on_file_downloaded=on_file_downloaded)
            except Exception as e:
                print("Download failed:", e)
    except Exception as e:
        print("Failed to fetch existing files:", e)

    last_event_time = time.monotonic()

    async def handle_update(payload):
        nonlocal last_event_time
        event_data = payload.get("data", {})
        data = event_data.get("record", {})  # New record data
        old_data = event_data.get("old_record", {})  # Old record data (for updates)
        last_event_time = time.monotonic()

        
        new_updated_at = data.get(UPDATED_AT_COLUMN)
        print("New updated_at:", new_updated_at)
        old_updated_at = old_data.get(UPDATED_AT_COLUMN)

        if new_updated_at is None:
            print("UPDATE received but updated_at missing:", payload)
            return

        if old_updated_at is not None and new_updated_at == old_updated_at:
            print(f"UPDATE received but {UPDATED_AT_COLUMN} did not change; skipping:", payload)
            return

        object_name = data.get(OBJECT_PATH_COLUMN)
        old_object_name = old_data.get(OBJECT_PATH_COLUMN)

        if not object_name:
            print("UPDATE received but object name missing:", payload)
            return

        # Avoid feedback loops: audio_model updates analysis columns on wav_files,
        # which can emit UPDATE events without changing the storage object path.
        if old_object_name is not None and object_name == old_object_name:
            print("UPDATE received but object_path did not change; skipping.")
            return

        if not _mark_event_if_new(object_name, new_updated_at):
            print("Duplicate update event detected; skipping.")
            return

        url = public_storage_url(project_ref, WAV_BUCKET, object_name)
        filename = os.path.basename(object_name)
        dest_path = os.path.join(LOCAL_DIR, filename)

        print(f"[{datetime.now(timezone.utc).isoformat()}] Downloading: {object_name} -> {dest_path}")
        try:
            await _download_and_process(url, dest_path, on_file_downloaded=on_file_downloaded)
        except Exception as e:
            print("Download failed:", e)

    # Subscribe to UPDATEs on wav_files
    # NOTE: the channel name can be anything except "realtime"
    try:
        channel = await (
            supabase.channel("wav-files-updates")
            .on_postgres_changes(
                event="UPDATE",  # Revert to UPDATE now that it's working
                schema=TABLE_SCHEMA,
                table=TABLE_NAME,
                callback=lambda payload: asyncio.create_task(handle_update(payload)),
            )
            .subscribe()
        )
        print("Successfully subscribed to wav_files UPDATE events.")
    except Exception as e:
        print("Failed to subscribe to realtime updates:", e)
        raise RuntimeError("Subscription failed") from e

    print("Waiting for changes...")
    last_heartbeat = time.monotonic()
    while True:
        await asyncio.sleep(10)
        now = time.monotonic()
        if now - last_heartbeat >= _HEARTBEAT_SECONDS:
            channel_state = getattr(channel, "state", "unknown")
            seconds_since_event = int(now - last_event_time)
            print(
                f"[heartbeat] channel_state={channel_state}, "
                f"seconds_since_last_event={seconds_since_event}"
            )
            last_heartbeat = now


def start_listener(on_file_downloaded=None):
    """Start the database listener in a non-blocking way"""
    def run_async():
        delay = _RECONNECT_BASE_DELAY_SECONDS
        while True:
            try:
                asyncio.run(main(on_file_downloaded=on_file_downloaded))
                print("Listener exited; reconnecting with backoff...")
            except Exception as e:
                print(f"Listener crashed: {e}. Reconnecting with backoff...")

            print(f"Retrying listener in {delay}s...")
            time.sleep(delay)
            delay = min(delay * 2, _RECONNECT_MAX_DELAY_SECONDS)
    
    listener_thread = threading.Thread(target=run_async, daemon=True)
    listener_thread.start()
    return listener_thread

if __name__ == "__main__":
    asyncio.run(main())