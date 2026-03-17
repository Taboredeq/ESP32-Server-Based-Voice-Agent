"""
BMO Voice Effects
Applies pitch shift, bandpass filter, and "radio" effect to make TTS sound like BMO from Adventure Time series
"""

from pydub import AudioSegment, effects
import numpy as np
import soundfile as sf
from scipy.signal import butter, lfilter

def bandpass_filter(samples, rate, lowcut=700.0, highcut=4000.0, order=4):
    """Apply bandpass filter"""
    nyquist = 0.5 * rate
    low = lowcut / nyquist
    high = highcut / nyquist
    b, a = butter(order, [low, high], btype='band')
    return lfilter(b, a, samples)

def radio_effect(samples, rate):
    """Apply radio / robotic effect"""
    b_hp = [0.8, -0.8]
    a_hp = [1, -0.6]
    filtered = lfilter(b_hp, a_hp, samples)
    b_lp = [0.2, 0.2]
    a_lp = [1, -0.6]
    filtered = lfilter(b_lp, a_lp, filtered)
    filtered = np.tanh(filtered * 2.5)
    filtered /= np.max(np.abs(filtered))
    return filtered

def make_bmo_like(input_wav: str, output_wav: str):
    """Apply BMO effect to TTS WAV"""
    sound = AudioSegment.from_wav(input_wav)
    sound = sound.set_channels(1)  # mono

    # Pitch shift
    octaves = 0.25
    new_sample_rate = int(sound.frame_rate * (2.0 ** octaves))
    sound_high = sound._spawn(sound.raw_data, overrides={'frame_rate': new_sample_rate})
    sound_high = sound_high.set_frame_rate(44100)

    # Normalize
    sound_high = effects.normalize(sound_high)
    samples = np.array(sound_high.get_array_of_samples()).astype(np.float32)
    samples /= np.max(np.abs(samples))

    # Apply radio effect and bandpass filter
    robot_samples = radio_effect(samples, 44100)
    robot_samples = bandpass_filter(robot_samples, 44100)

    # Normalize final output
    final_samples = robot_samples / np.max(np.abs(robot_samples))
    sf.write(output_wav, final_samples, 44100)
    print(f"Saved BMO voice as: {output_wav}")