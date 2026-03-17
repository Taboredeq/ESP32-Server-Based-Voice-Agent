"""
GPT Helper Module
Handles transcription, GPT response, and TTS generation
"""

from openai import OpenAI

# --- Configure OpenAI API key ---
OPENAI_TOKEN = 'YOUR_OPENAI_API_KEY'
client = OpenAI(api_key=OPENAI_TOKEN)

def transcribe_audio(file_path: str) -> str:
    """Transcribe audio file using GPT-4o-mini-transcribe"""
    with open(file_path, "rb") as f:
        transcript = client.audio.transcriptions.create(
            model="gpt-4o-mini-transcribe",
            file=f
        )
    return transcript.text

def gpt_response(question: str) -> str:
    """Generate BMO-style text response"""
    response = client.chat.completions.create(
        model="gpt-3.5-turbo",
        messages=[
            {"role": "system", 
             "content": (
                 "You generate only text for TTS. "
                 "You are BMO (say your name as 'Bimo'), friendly and cheerful, "
                 "always laughing and helpful."
             )},
            {"role": "user", "content": question}
        ]
    )
    return response.choices[0].message.content

def tts(text: str):
    """Generate TTS audio file (wav) from text"""
    response = client.audio.speech.create(
        model="gpt-4o-mini-tts",
        voice="sage",
        input=text,
        response_format="wav"
    )
    with open("tts.wav", "wb") as f:
        f.write(response.read())