"""Central entry: listen for DB updates, then run audio analysis pipelines."""

from __future__ import annotations

import os
import threading
import time

import background_speech_ratio
import database_reader
import environment_detection
import frequency_filtering

_PROCESS_LOCK = threading.Lock()


def process_audio_file(file_path: str) -> None:
    try:
        with _PROCESS_LOCK:
            print(f"\nProcessing: {file_path}")
            file_name = os.path.basename(file_path)

            env_result = environment_detection.analyze(file_path)
            freq_result = frequency_filtering.analyze(file_path)
            bsr_result = background_speech_ratio.analyze(file_path)

            print(f"\n--- environment_detection ({file_name}) ---")
            print(env_result)
            print(f"\n--- frequency_filtering ({file_name}) ---")
            print(freq_result)
            print(f"\n--- background_speech_ratio ({file_name}) ---")
            print(bsr_result)
    except Exception as e:
        print(f"Error processing file {file_path}: {e}")


if __name__ == "__main__":
    print("\nStarting database listener...")
    database_reader.start_listener(on_file_downloaded=process_audio_file)
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Shutting down...")
