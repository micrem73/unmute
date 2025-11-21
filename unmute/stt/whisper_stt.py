import asyncio
import logging
import os
from collections import deque
from dataclasses import dataclass

import numpy as np
from openai import AsyncOpenAI

from unmute.stt.speech_to_text import STTMarkerMessage

logger = logging.getLogger(__name__)


@dataclass
class WhisperTranscription:
    """Messaggio con trascrizione da Whisper"""
    text: str
    start_time: float


class WhisperSTT:
    """Speech-to-Text usando OpenAI Whisper API"""
    
    def __init__(self, sample_rate: int = 16000):
        self.client = AsyncOpenAI(api_key=os.environ.get("OPENAI_API_KEY"))
        self.sample_rate = sample_rate
        self.audio_buffer = deque(maxlen=160000)
        self.current_time = 0.0
        self.is_processing = False
        self.last_transcription_time = 0.0
        self.bot_speaking = False  # <-- AGGIUNGI QUESTA RIGA
        
        # VAD simulato
        self.silence_threshold = 0.02
        self.silence_duration = 0
        self.min_audio_duration = 1.0  # Minimum audio before considering transcription
        
        # Aggiungi pause_prediction per compatibilità
        from unmute.stt.exponential_moving_average import ExponentialMovingAverage
        self.pause_prediction = ExponentialMovingAverage(0.5, 0.5)  # (attack_time, release_time)
        
        # Aggiungi anche delay_sec e sent_samples
        self.delay_sec = 0.5  # <-- AGGIUNGI
        self.sent_samples = 0  # <-- AGGIUNGI
        
        logger.info("WhisperSTT inizializzato")
            
    
    async def send_audio(self, audio: np.ndarray):
        """Riceve audio chunk e accumula nel buffer"""
        self.current_time += len(audio) / self.sample_rate
        self.sent_samples += len(audio)

        # Se il bot sta parlando, non accumulare audio e svuota il buffer
        if self.bot_speaking:
            if len(self.audio_buffer) > 0:
                logger.debug(f"🔇 STT ignoring audio: bot is speaking (cleared {len(self.audio_buffer)} samples)")
            self.audio_buffer.clear()
            self.silence_duration = 0
            self.pause_prediction.value = 1.0  # High value = not speaking
            return None

        # Accumula audio solo quando il bot NON sta parlando
        self.audio_buffer.extend(audio.flatten())

        # Rileva silenzio
        rms = np.sqrt(np.mean(audio ** 2))
        if rms < self.silence_threshold:
            self.silence_duration += len(audio) / self.sample_rate
            self.pause_prediction.value = 0.8
        else:
            self.silence_duration = 0
            self.pause_prediction.value = 0.2

        # Se c'è silenzio > 2.0s e abbastanza audio, trascrivi
        # Aumentato a 2.0s per dare più tempo all'utente di finire di parlare
        if (self.silence_duration > 2.0 and
            len(self.audio_buffer) > self.sample_rate * self.min_audio_duration and
            not self.is_processing):
            logger.info(f"🎤 Silence detected ({self.silence_duration:.1f}s), transcribing {len(self.audio_buffer)} samples")
            return await self._transcribe_buffer()

        return None
    
    async def _transcribe_buffer(self):
        """Trascrivi il buffer audio con Whisper"""
        if self.is_processing:
            return
        
        self.is_processing = True
        
        try:
            # Converti buffer in array
            audio_data = np.array(self.audio_buffer, dtype=np.float32)
            
            # Salva in file temporaneo (Whisper API richiede file)
            import tempfile
            import wave
            
            with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp_file:
                tmp_path = tmp_file.name
                
                # Scrivi WAV
                with wave.open(tmp_path, 'wb') as wav_file:
                    wav_file.setnchannels(1)
                    wav_file.setsampwidth(2)  # 16-bit
                    wav_file.setframerate(self.sample_rate)
                    # Converti float32 in int16
                    audio_int16 = (audio_data * 32767).astype(np.int16)
                    wav_file.writeframes(audio_int16.tobytes())
            
            # Chiamata a Whisper API
            with open(tmp_path, 'rb') as audio_file:
                transcription = await self.client.audio.transcriptions.create(
                    model="whisper-1",
                    file=audio_file,
                    response_format="text"
                )
            
            # Cleanup
            os.unlink(tmp_path)
            
            # Invia trascrizione
            if transcription and transcription.strip():
                # AGGIUNGI QUESTO FILTRO:
                if len(transcription.strip()) < 3:  # Ignora parole troppo corte
                    logger.debug(f"Whisper: trascrizione troppo corta ignorata: '{transcription}'")
                    return None
                
                logger.info(f"Whisper: '{transcription}'")
                return WhisperTranscription(
                    text=transcription,
                    start_time=self.current_time
                )
            
        except Exception as e:
            logger.error(f"Errore Whisper: {e}")
        
        finally:
            self.audio_buffer.clear()
            self.is_processing = False
            self.last_transcription_time = self.current_time
        
        return None
    
    async def shutdown(self):
        """Chiudi connessione"""
        logger.info("WhisperSTT shutdown")
    
    def state(self) -> str:
        """Stato corrente"""
        return "ready" if not self.is_processing else "processing"