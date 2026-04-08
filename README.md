# BMO Voice Assistant (ESP32 + OpenAI)

Embedded AI voice assistant inspired by BMO (*Adventure Time animated series*), built with ESP32 and a custom Python backend.  
The system enables real-time voice interaction using speech recognition, LLM responses, and audio synthesis.

---

##  Demo


---

##  Hardware

<p align="center">
  <img src="./images/schematic.png" width="1000"/>
</p>

---

##  Key Features

-  Real-time audio capture (ESP32 + I2S microphone)  
-  Wi-Fi streaming to Python server (TCP)  
-  Speech-to-text using OpenAI API  
-  AI responses with custom personality (BMO-style)  
-  Text-to-speech with audio post-processing  
-  Streaming audio response back to ESP32  
-  TFT display with custom BMO-inspired UI  
-  Silence detection and automatic recording stop  

---

##  Tech Stack

**Embedded:**
- ESP32 (C++ / Arduino)
- I2S (audio input/output)
- TFT_eSPI (display)

**Backend:**
- Python (socket server)
- NumPy / SciPy (audio processing)
- PyDub / SoundFile

**AI:**
- OpenAI API (STT + GPT + TTS)

---

##  System Flow

ESP32 → audio → Python Server → STT → GPT → TTS → effects → ESP32

---

## 📂 Repository Structure---

---

##  Highlights

- End-to-end voice AI pipeline on embedded hardware  
- Custom real-time audio streaming over TCP  
- Working physical prototype (tested on development board)  
- Combines embedded systems, DSP, networking, and AI  

---

##  Future Work

- Custom BMO enclosure (in progress)

<p align="center">
  <img src="./images/BMO_case.png" width="500"/>
</p>

- RTC Watch (in progress)
- Physical interaction buttons  
- Wake-word detection  
- Lower latency streaming
