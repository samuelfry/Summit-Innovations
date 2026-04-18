import tensorflow as tf
import librosa
import numpy as np
import pandas as pd
import os
import matplotlib.pyplot as plt
import tkinter as tk
from tkinter import messagebox
import threading
import time
import database_reader

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
    # Load situation labels from YAMNet class map
    situation_labels = pd.read_csv('./yamnet-tensorflow2-yamnet-v1/assets/yamnet_class_map.csv')['display_name'].tolist()
    
    # Load the downloaded local YAMNet saved model
    model = tf.saved_model.load('yamnet-tensorflow2-yamnet-v1')

    # Get mappings
    mappings = map_clips_to_situations(audio_clips, model, situation_labels)

    # Return results for each audio sample
    # for i in range(len(audio_clips)):
    #     result = mappings[i]
    #     print(f"{result['clip']}: {result['situation']} ({result['confidence']:.2%})")

    return mappings

def display_results(sound_result, background_noise):
    '''
    Pop up a window that stays open until closed by user
    Display what the sound is and if there is background noise
    '''
    root = tk.Tk()
    root.title("Audio Analysis Results")
    root.geometry("400x250")
    
    # Create a frame for content
    content_frame = tk.Frame(root)
    content_frame.pack(pady=20, padx=20, fill=tk.BOTH, expand=True)
    
    # File label (will be updated)
    file_label = tk.Label(content_frame, text="Processing audio file...", font=("Arial", 11, "italic"), fg="gray")
    file_label.pack(pady=5)
    
    # Display sound results
    if sound_result:
        tk.Label(content_frame, text=f"Detected Situation: {sound_result['situation']}", font=("Arial", 14, "bold")).pack(pady=10)
        tk.Label(content_frame, text=f"Confidence: {100 * sound_result['confidence']:50%}", font=("Arial", 12)).pack(pady=5)
    else:
        tk.Label(content_frame, text="No sound results available", font=("Arial", 14)).pack(pady=10)
    
    # Display background noise
    noise_text = "Background Noise Detected" if background_noise else "No Background Noise"
    noise_color = "red" if background_noise else "green"
    tk.Label(content_frame, text=noise_text, font=("Arial", 12, "bold"), fg=noise_color).pack(pady=10)
    
    # Close button
    tk.Button(root, text="Close", command=root.quit).pack(pady=20)
    
    root.mainloop()

def display_results_update(root, labels_dict, sound_result, background_noise, file_path):
    '''
    Update the window with new results
    '''
    # Update situation
    if sound_result:
        labels_dict['situation'].config(text=f"Detected Situation: {sound_result['situation']}")
        labels_dict['confidence'].config(text=f"Confidence: {sound_result['confidence']:.2%}")
    
    # Update background noise
    noise_text = "Background Noise Detected" if background_noise else "No Background Noise"
    noise_color = "red" if background_noise else "green"
    labels_dict['noise'].config(text=noise_text, fg=noise_color)
    
    # Update file name
    file_name = os.path.basename(file_path)
    labels_dict['file'].config(text=f"File: {file_name}")
    
    root.update()

# 4. Usage example
if __name__ == '__main__':
    # Load model and labels once at startup
    print("Loading YAMNet model and class labels...")
    situation_labels = pd.read_csv('./yamnet-tensorflow2-yamnet-v1/assets/yamnet_class_map.csv')['display_name'].tolist()
    model = tf.saved_model.load('yamnet-tensorflow2-yamnet-v1')
    print("Model loaded successfully!")
    
    # Create the main window
    root = tk.Tk()
    root.title("Audio Analysis Results")
    root.geometry("450x300")
    
    # Create labels dictionary for updating
    labels_dict = {}
    
    # File label
    labels_dict['file'] = tk.Label(root, text="Waiting for audio files from database...", font=("Arial", 11, "italic"), fg="gray")
    labels_dict['file'].pack(pady=10)
    
    # Separator
    tk.Frame(root, height=2, bd=1, relief=tk.SUNKEN).pack(fill=tk.X, padx=5, pady=5)
    
    # Situation label
    labels_dict['situation'] = tk.Label(root, text="Detected Situation: --", font=("Arial", 14, "bold"))
    labels_dict['situation'].pack(pady=10)
    
    # Confidence label
    labels_dict['confidence'] = tk.Label(root, text="Confidence: --", font=("Arial", 12))
    labels_dict['confidence'].pack(pady=5)
    
    # Noise label
    labels_dict['noise'] = tk.Label(root, text="No Background Noise", font=("Arial", 12, "bold"), fg="green")
    labels_dict['noise'].pack(pady=10)
    
    # Close button
    tk.Button(root, text="Close", command=root.quit, font=("Arial", 11)).pack(pady=20)
    
    # Callback function to process downloaded files
    def process_audio_file(file_path):
        try:
            print(f"\nProcessing: {file_path}")
            sound_result = identify_sounds([file_path])[0]
            background_noise = check_background(file_path)
            display_results_update(root, labels_dict, sound_result, background_noise, file_path)
        except Exception as e:
            print(f"Error processing file {file_path}: {e}")
            labels_dict['situation'].config(text=f"Error: {str(e)}")
    
    # Start database listener in a separate thread
    print("\nStarting database listener...")
    listener_thread = database_reader.start_listener(on_file_downloaded=process_audio_file)
    
    # Keep the window open
    root.mainloop()
    print("Closing application...")



