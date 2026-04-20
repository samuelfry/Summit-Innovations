import tensorflow as tf
import librosa
import numpy as np
import pandas as pd
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import threading
import time
import database_reader
from supabase import create_client

_YAMNET_LOCK = threading.Lock()
_YAMNET_MODEL = None
_YAMNET_LABELS = None


def _yamnet_model_and_labels():
    """Load YAMNet once; all inference must run under _YAMNET_LOCK."""
    global _YAMNET_MODEL, _YAMNET_LABELS
    if _YAMNET_MODEL is not None:
        return _YAMNET_MODEL, _YAMNET_LABELS
    situation_labels = pd.read_csv(
        "./yamnet-tensorflow2-yamnet-v1/assets/yamnet_class_map.csv"
    )["display_name"].tolist()
    model = tf.saved_model.load("yamnet-tensorflow2-yamnet-v1")
    _YAMNET_MODEL = model
    _YAMNET_LABELS = situation_labels
    return model, situation_labels

# Initialize Supabase client for database updates
SUPABASE_URL = "https://gctbnsjsridmsilzqtpq.supabase.co"
SUPABASE_KEY = "sb_publishable_DqbujbP_YcPdU1U4q6XjiA_XjqICfuA"
supabase = create_client(SUPABASE_URL, SUPABASE_KEY)

# 1. Load and preprocess audio
def load_and_preprocess_audio(audio_path, sr=16000, duration=None):
    """Load audio waveform for YAMNet"""
    audio, _ = librosa.load(audio_path, sr=sr, duration=duration)
    return tf.convert_to_tensor(audio, dtype=tf.float32)

# 2. Load YAMNet model and predict
def detect_situation(audio_path, model):
    """Detect situation from audio"""
    audio = load_and_preprocess_audio(audio_path)
    scores, embeddings, spectrogram = model(audio)
    # YAMNet outputs frame-level scores, so average them over time before selecting the class.
    mean_scores = tf.reduce_mean(scores, axis=0)
    probabilities = tf.nn.softmax(mean_scores)
    situation_idx = tf.argmax(probabilities).numpy()
    return situation_idx, probabilities.numpy()

# 3. Map audio clips to situations
def map_clips_to_situations(audio_clips, model, situation_labels):
    """Map multiple audio clips to their detected situations"""
    results = []
    
    for clip_path in audio_clips:
        situation_idx, confidence = detect_situation(clip_path, model)
        situation_name = situation_labels[situation_idx]
        
        results.append({
            'clip': clip_path,
            'situation': situation_name,
            'confidence': confidence[situation_idx]
        })
    
    return results

def check_background(wave_file_path):
    '''
    Input: .wav file
    Output: Boolean indictating if there is background noise
    '''
    # Load audio
    audio, sr = librosa.load(wave_file_path, sr=None)  # sr=None to keep original sample rate
    
    # Compute FFT
    fft = np.fft.fft(audio)
    freqs = np.fft.fftfreq(len(audio), 1/sr)
    
    # Get magnitude and positive frequencies
    magnitude = np.abs(fft)
    positive_freqs = freqs[:len(freqs)//2]
    positive_magnitude = magnitude[:len(magnitude)//2]

    mag_range = [1250, 1500]
    background_noise = False
    if len(positive_magnitude) > mag_range[1]:
        mag_in_range = positive_magnitude[mag_range[0]: mag_range[1]]
        if np.sum(mag_in_range > 15) > 10:  # More than 10 values are greater than 15
            background_noise = True
    # If not enough data, assume no background noise
    return background_noise

def plot_fourier_transform(wave_files, plot=False, identify=False):
    """Plot the Fourier Transform (magnitude spectrum) for each wave file in the list."""
    for file_path in wave_files:
        # Load audio
        audio, sr = librosa.load(file_path, sr=None)  # sr=None to keep original sample rate
        
        # Compute FFT
        fft = np.fft.fft(audio)
        freqs = np.fft.fftfreq(len(audio), 1/sr)
        
        # Get magnitude and positive frequencies
        magnitude = np.abs(fft)
        positive_freqs = freqs[:len(freqs)//2]
        positive_magnitude = magnitude[:len(magnitude)//2]

        if identify == True:
            mag_range = [1250, 1500]
            background = None
            mag_in_range = positive_magnitude[mag_range[0]: mag_range[1]]
            if np.sum(mag_in_range > 15) > 10: # More than 5 values are greater than 15
                background = "background noise reduction"
            else:
                background = "normal"
            print(f"Recommended setting for {file_path} is {background}.")
        
        if plot == True: # Save plots in FFT_graphs folder
            # Plot
            plt.figure(figsize=(10, 6))
            # plt.plot(positive_freqs[100:], positive_magnitude[100:])
            plt.plot(positive_freqs[1250: 1500], positive_magnitude[1250: 1500])
            plt.title(f'Fourier Transform of {os.path.basename(file_path)}')
            plt.xlabel('Frequency (Hz)')
            plt.ylabel('Magnitude')
            plt.grid(True)
            
            # Save the plot instead of showing
            output_filename = f"fourier_{os.path.basename(file_path).replace('.wav', '')}.png"
            path = os.path.join("FFT_graphs", output_filename)
            plt.savefig(path)
            plt.close()  # Close the figure to free memory
            print(f"Plot saved as {output_filename}")

def identify_sounds(audio_clips):
    with _YAMNET_LOCK:
        model, situation_labels = _yamnet_model_and_labels()
        mappings = map_clips_to_situations(audio_clips, model, situation_labels)

    # Return results for each audio sample
    # for i in range(len(audio_clips)):
    #     result = mappings[i]
    #     print(f"{result['clip']}: {result['situation']} ({result['confidence']:.2%})")

    return mappings

def update_wav_file_record(file_path, environment_result, background_noise_result, confidence_result):
    """Update wav_files table with audio analysis results"""
    try:
        # Extract filename from path
        filename = os.path.basename(file_path)
        
        # Query for the record matching this file
        response = supabase.table('wav_files').select('*').ilike('object_path', f'%{filename}%').execute()
        
        if response.data and len(response.data) > 0:
            record = response.data[0]
            record_id = record['public_id']
            
            # Update the record with analysis results
            update_response = supabase.table('wav_files').update({
                'environment_result': environment_result,
                'background_noise_result': background_noise_result,
                'confidence_result': confidence_result
            }).eq('public_id', record_id).execute()
            
            print(f"Updated database record for {filename}")
        else:
            print(f"Warning: Could not find database record for {filename}")
    except Exception as e:
        print(f"Error updating database record: {e}")

# 4. Usage example
if __name__ == '__main__':
    _PROCESS_LOCK = threading.Lock()

    # Callback function to process downloaded files
    def process_audio_file(file_path):
        try:
            with _PROCESS_LOCK:
                print(f"\nProcessing: {file_path}")
                sound_result = identify_sounds([file_path])[0]
                background_noise = check_background(file_path)
                file_name = os.path.basename(file_path)

                environment_result = sound_result['situation'] if sound_result else "Unknown"
                confidence_result = float(sound_result['confidence']) if sound_result else 0.0
                background_noise_result = "Background Noise Detected" if background_noise else "No Background Noise"

                update_wav_file_record(file_path, environment_result, background_noise_result, confidence_result)

                print(f"File: {file_name}")
                print(f"Detected Situation: {environment_result}")
                print(f"Confidence: {confidence_result:.2%}")
                print(background_noise_result)
        except Exception as e:
            print(f"Error processing file {file_path}: {e}")
    
    # Start database listener in a separate thread
    print("\nStarting database listener...")
    listener_thread = database_reader.start_listener(on_file_downloaded=process_audio_file)
    
    # Keep the main thread alive
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Shutting down...")
