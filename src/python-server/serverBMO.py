"""
BMO Voice Server
----------------
A server for ESP32 BMO project that:
- Receives raw audio from ESP32 over TCP
- Saves audio, converts to WAV
- Transcribes speech using OpenAI
- Generates BMO-style text response
- Applies "BMO-like" voice effect to TTS
- Sends the processed audio back to ESP32
"""

import socket
import time
import numpy as np
import wave
from scipy.io.wavfile import write
from pydub import AudioSegment, effects
from scipy.signal import butter, lfilter
import soundfile as sf

# Custom modules
import gpt
import bmo_voice

# --- Server configuration ---
HOST = ''          # Bind to all interfaces
PORT = 5000        # Listening port

# --- Main server setup ---
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(1)
print(f"Server listening on port {PORT}...")

# --- Server main loop ---
while True:
    conn, addr = server.accept()
    print(f"Connected by {addr}")

    # --- Receive audio from ESP32 ---
    raw_filename = "input.raw"
    with open(raw_filename, "wb") as f:
        while True:
            data = conn.recv(4096)
            if not data:
                break
            if data.endswith(b'END'):
                f.write(data[:-3])
                break
            f.write(data)
    print(f"Saved raw audio to {raw_filename}")

    start_time = time.time()

    # Convert raw audio to WAV
    data = np.fromfile(raw_filename, dtype=np.int32)
    wav_filename = "input.wav"
    write(wav_filename, 16000, data)
    print(f"Saved WAV audio to {wav_filename}")

    #Transcribe audio
    question = gpt.transcribe_audio(wav_filename)
    print("Transcribed text:", question)

    #Generate BMO response
    response_text = gpt.gpt_response(question)
    print("BMO Response:", response_text)

    #Text-to-speech
    gpt.tts(response_text)
    bmo_voice.make_bmo_like("tts.wav", "bmo_response.wav")

    #Send processed audio back to ESP32
    with wave.open("bmo_response.wav", "rb") as wav_in:
        frames = wav_in.readframes(wav_in.getnframes())
    conn.sendall(frames)

    print("Sent audio response to ESP32")
    print("Operation time: {:.2f} seconds\n".format(time.time() - start_time))
    conn.close()