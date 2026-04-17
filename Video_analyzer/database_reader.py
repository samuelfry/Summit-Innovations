import os
import asyncio
import threading
from datetime import datetime, timezone
import requests
from supabase import acreate_client, AsyncClient

SUPABASE_URL = "https://gctbnsjsridmsilzqtpq.supabase.co"  # https://<project-ref>.supabase.co
SUPABASE_KEY = "sb_publishable_DqbujbP_YcPdU1U4q6XjiA_XjqICfuA"  # anon/publishable is fine for reading public storage + listening
# If you’re using private Realtime, you’d need a user JWT and set_auth, but you said public folder.

WAV_BUCKET = "audio"  # Public bucket name
LOCAL_DIR = "./downloads"

TABLE_SCHEMA = "public"
TABLE_NAME = "wav_files"

OBJECT_PATH_COLUMN = "object_path"  # Column that holds Storage object_name/path
UPDATED_AT_COLUMN = "updated_at" # Column that holds a time stamp of when object was last updated


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
            if not object_name:
                continue
            
            url = public_storage_url(project_ref, WAV_BUCKET, object_name)
            filename = os.path.basename(object_name)
            dest_path = os.path.join(LOCAL_DIR, filename)
            
            # Skip if already downloaded
            if os.path.exists(dest_path):
                print(f"File already exists: {dest_path}")
                if on_file_downloaded:
                    on_file_downloaded(dest_path)
                continue
            
            print(f"Downloading existing file: {object_name} -> {dest_path}")
            try:
                download_file(url, dest_path)
                print("Saved:", dest_path)
                if on_file_downloaded:
                    on_file_downloaded(dest_path)
            except Exception as e:
                print("Download failed:", e)
    except Exception as e:
        print("Failed to fetch existing files:", e)

    async def handle_update(payload):
        event_data = payload.get("data", {})
        data = event_data.get("record", {})  # New record data
        old_data = event_data.get("old_record", {})  # Old record data (for updates)

        
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

        if not object_name:
            print("UPDATE received but object name missing:", payload)
            return

        url = public_storage_url(project_ref, WAV_BUCKET, object_name)
        filename = os.path.basename(object_name)
        dest_path = os.path.join(LOCAL_DIR, filename)

        print(f"[{datetime.now(timezone.utc).isoformat()}] Downloading: {object_name} -> {dest_path}")
        try:
            download_file(url, dest_path)
            print("Saved:", dest_path)
            # Call the callback if provided
            if on_file_downloaded:
                on_file_downloaded(dest_path)
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
        return  # Exit if subscription fails

    print("Waiting for changes...")
    while True:
        await asyncio.sleep(10)


def start_listener(on_file_downloaded=None):
    """Start the database listener in a non-blocking way"""
    def run_async():
        asyncio.run(main(on_file_downloaded=on_file_downloaded))
    
    listener_thread = threading.Thread(target=run_async, daemon=True)
    listener_thread.start()
    return listener_thread

if __name__ == "__main__":
    asyncio.run(main())