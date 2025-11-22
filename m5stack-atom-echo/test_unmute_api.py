#!/usr/bin/env python3
"""
Test Script per Unmute API - Simula M5Stack Atom Echo
======================================================

Questo script simula il comportamento dell'M5Stack Atom Echo per testare
le API essenziali di Unmute:

1. Connessione WebSocket su porta 443 (wss://)
2. Session update con voice "Anne"
3. Invio audio stream simulato
4. Eventi bambola (cordino tirato/rilasciato)
5. Ricezione audio stream da Unmute

Uso:
    python test_unmute_api.py --host your-vm.lightning.ai --voice Anne
"""

import asyncio
import json
import base64
import argparse
import time
import ssl
from typing import Optional
import websockets
import numpy as np

# Configurazione
SAMPLE_RATE = 24000
CHANNELS = 1
AUDIO_CHUNK_SIZE = 960  # 40ms at 24kHz
OPUS_FRAME_SIZE = 200  # Simulato, dimensione tipica frame Opus


class UnmuteAPITester:
    """Testa le API Unmute simulando comportamento M5Stack Atom Echo"""

    def __init__(self, host: str, port: int = 443, use_ssl: bool = True, voice: str = "Anne"):
        self.host = host
        self.port = port
        self.use_ssl = use_ssl
        self.voice = voice
        self.ws: Optional[websockets.WebSocketClientProtocol] = None
        self.running = False

        # Stats
        self.audio_chunks_sent = 0
        self.audio_chunks_received = 0
        self.text_deltas_received = 0
        self.events_received = {}

    def get_websocket_url(self) -> str:
        """Costruisce URL WebSocket"""
        protocol = "wss" if self.use_ssl else "ws"
        return f"{protocol}://{self.host}:{self.port}/api/v1/realtime"

    async def connect(self):
        """Connette al server Unmute WebSocket"""
        url = self.get_websocket_url()
        print(f"\n{'='*60}")
        print(f"🔌 CONNESSIONE WEBSOCKET")
        print(f"{'='*60}")
        print(f"URL: {url}")
        print(f"Voice: {self.voice}")
        print(f"SSL: {self.use_ssl}")

        # SSL context per wss://
        ssl_context = None
        if self.use_ssl:
            ssl_context = ssl.create_default_context()
            # Per testing, accetta certificati self-signed
            # ssl_context.check_hostname = False
            # ssl_context.verify_mode = ssl.CERT_NONE

        try:
            # Connetti con subprotocol "realtime" come richiesto da OpenAI Realtime API
            self.ws = await websockets.connect(
                url,
                subprotocols=["realtime"],
                ssl=ssl_context,
                ping_interval=20,
                ping_timeout=20
            )
            print(f"✅ Connesso a {url}")
            self.running = True
            return True

        except Exception as e:
            print(f"❌ Errore connessione: {e}")
            return False

    async def send_message(self, event_type: str, **kwargs):
        """Invia messaggio JSON al server"""
        message = {
            "type": event_type,
            **kwargs
        }

        if self.ws:
            await self.ws.send(json.dumps(message))
            print(f"📤 TX: {event_type}")
            return True
        return False

    async def send_session_update(self):
        """Invia session update per configurare voice"""
        print(f"\n{'='*60}")
        print(f"⚙️  SESSION UPDATE")
        print(f"{'='*60}")

        await self.send_message(
            "session.update",
            session={
                "voice": self.voice,
                "instructions": "You are a helpful voice assistant.",
                "turn_detection": None  # Disable auto turn detection
            }
        )

    def generate_fake_audio(self, duration_sec: float = 0.04) -> bytes:
        """Genera audio finto (sine wave) per testing"""
        samples = int(SAMPLE_RATE * duration_sec)
        t = np.linspace(0, duration_sec, samples, False)
        # Sine wave a 440Hz (nota A4)
        audio = np.sin(2 * np.pi * 440 * t)
        # Converti a int16
        audio_int16 = (audio * 32767).astype(np.int16)
        return audio_int16.tobytes()

    def encode_audio_to_opus_fake(self, pcm_data: bytes) -> bytes:
        """Simula encoding Opus (per test usiamo PCM diretto)"""
        # In produzione, qui dovrebbe esserci encoding Opus reale
        # Per test, restituiamo dati ridotti (simulazione compressione)
        return pcm_data[:OPUS_FRAME_SIZE]

    async def send_audio_chunk(self):
        """Invia chunk audio al server"""
        # Genera audio finto
        pcm_audio = self.generate_fake_audio()

        # Simula encoding Opus
        opus_audio = self.encode_audio_to_opus_fake(pcm_audio)

        # Base64 encode
        audio_b64 = base64.b64encode(opus_audio).decode('utf-8')

        # Invia
        await self.send_message(
            "input_audio_buffer.append",
            audio=audio_b64
        )

        self.audio_chunks_sent += 1

    async def send_cordino_tirato(self):
        """Invia evento cordino tirato (bottone premuto)"""
        print(f"\n{'='*60}")
        print(f"🎯 CORDINO TIRATO (Bottone Premuto)")
        print(f"{'='*60}")

        await self.send_message("unmute.bambola.cordino_tirato")

    async def send_cordino_rilasciato(self):
        """Invia evento cordino rilasciato (bottone rilasciato)"""
        print(f"\n{'='*60}")
        print(f"🎯 CORDINO RILASCIATO (Bottone Rilasciato)")
        print(f"{'='*60}")

        await self.send_message("unmute.bambola.cordino_rilasciato")

    async def handle_message(self, message_str: str):
        """Gestisce messaggi ricevuti dal server"""
        try:
            message = json.loads(message_str)
            event_type = message.get("type", "unknown")

            # Conta eventi
            self.events_received[event_type] = self.events_received.get(event_type, 0) + 1

            # Log eventi importanti
            if event_type == "session.updated":
                print(f"✅ RX: session.updated - Sessione configurata!")

            elif event_type == "response.created":
                print(f"✅ RX: response.created - Bot sta generando risposta")

            elif event_type == "response.text.delta":
                delta = message.get("delta", "")
                self.text_deltas_received += 1
                print(f"📝 RX: response.text.delta - '{delta}'", end="", flush=True)

            elif event_type == "response.text.done":
                text = message.get("text", "")
                print(f"\n✅ RX: response.text.done - Testo completo: '{text}'")

            elif event_type == "response.audio.delta":
                audio_b64 = message.get("delta", "")
                audio_bytes = base64.b64decode(audio_b64)
                self.audio_chunks_received += 1
                print(f"🔊 RX: response.audio.delta - {len(audio_bytes)} bytes")

            elif event_type == "response.audio.done":
                print(f"✅ RX: response.audio.done - Audio completo ricevuto")

            elif event_type == "unmute.bambola.buffer_ready":
                buffer_size = message.get("buffer_size", 0)
                print(f"✅ RX: unmute.bambola.buffer_ready - Buffer: {buffer_size} samples")

            elif event_type == "unmute.bambola.playback_started":
                print(f"🔊 RX: unmute.bambola.playback_started - Inizia riproduzione")

            elif event_type == "unmute.bambola.playback_completed":
                print(f"✅ RX: unmute.bambola.playback_completed - Riproduzione completata")

            elif event_type == "error":
                error_msg = message.get("error", {}).get("message", "unknown")
                print(f"❌ RX: error - {error_msg}")

            else:
                # Altri eventi (meno verbose)
                if event_type not in ["response.text.delta", "response.audio.delta"]:
                    print(f"📥 RX: {event_type}")

        except json.JSONDecodeError as e:
            print(f"❌ Errore parsing JSON: {e}")

    async def receive_loop(self):
        """Loop di ricezione messaggi"""
        try:
            async for message in self.ws:
                await self.handle_message(message)
        except websockets.exceptions.ConnectionClosed:
            print("\n⚠️  Connessione WebSocket chiusa")
        except Exception as e:
            print(f"\n❌ Errore receive loop: {e}")

    async def run_test_scenario(self):
        """Esegue scenario di test completo"""
        print(f"\n{'='*60}")
        print(f"🧪 INIZIO TEST SCENARIO")
        print(f"{'='*60}")

        # 1. Session update
        await self.send_session_update()
        await asyncio.sleep(1)

        # 2. Simula utente che parla (invia audio chunks)
        print(f"\n{'='*60}")
        print(f"🎤 SIMULAZIONE AUDIO INPUT (Utente Parla)")
        print(f"{'='*60}")

        for i in range(10):
            await self.send_audio_chunk()
            await asyncio.sleep(0.04)  # 40ms chunks
            if i % 5 == 0:
                print(f"📤 Inviati {self.audio_chunks_sent} chunks audio...")

        print(f"✅ Inviati {self.audio_chunks_sent} chunks audio totali")

        # 3. Attendi un po' per trascrizione
        await asyncio.sleep(2)

        # 4. Simula pressione bottone (cordino tirato)
        await self.send_cordino_tirato()
        await asyncio.sleep(0.5)

        # 5. Attendi generazione risposta (buffer)
        print(f"\n⏳ Attesa buffer risposta bot...")
        await asyncio.sleep(5)

        # 6. Simula rilascio bottone (cordino rilasciato)
        await self.send_cordino_rilasciato()

        # 7. Attendi playback audio
        print(f"\n⏳ Attesa playback audio bot...")
        await asyncio.sleep(5)

        # 8. Mostra statistiche
        await self.print_stats()

    async def print_stats(self):
        """Mostra statistiche test"""
        print(f"\n{'='*60}")
        print(f"📊 STATISTICHE TEST")
        print(f"{'='*60}")
        print(f"Audio chunks inviati:      {self.audio_chunks_sent}")
        print(f"Audio chunks ricevuti:     {self.audio_chunks_received}")
        print(f"Text deltas ricevuti:      {self.text_deltas_received}")
        print(f"\nEventi ricevuti:")
        for event_type, count in sorted(self.events_received.items()):
            print(f"  - {event_type}: {count}")
        print(f"{'='*60}")

    async def disconnect(self):
        """Disconnette dal server"""
        if self.ws:
            await self.ws.close()
            print(f"\n🔌 Disconnesso dal server")

    async def run(self):
        """Esegue test completo"""
        # Connetti
        if not await self.connect():
            return

        # Avvia receive loop in background
        receive_task = asyncio.create_task(self.receive_loop())

        # Esegui scenario test
        await self.run_test_scenario()

        # Cleanup
        receive_task.cancel()
        await self.disconnect()


async def test_connection_only(host: str, port: int = 443, use_ssl: bool = True):
    """Test solo connessione WebSocket"""
    tester = UnmuteAPITester(host, port, use_ssl)

    print(f"\n{'='*60}")
    print(f"🧪 TEST CONNESSIONE SEMPLICE")
    print(f"{'='*60}")

    if await tester.connect():
        print(f"✅ Connessione riuscita!")

        # Ricevi qualche messaggio
        try:
            message = await asyncio.wait_for(tester.ws.recv(), timeout=5.0)
            print(f"📥 Primo messaggio ricevuto: {message[:100]}...")
        except asyncio.TimeoutError:
            print(f"⚠️  Nessun messaggio ricevuto entro 5 secondi")

        await tester.disconnect()
    else:
        print(f"❌ Connessione fallita!")


async def main():
    parser = argparse.ArgumentParser(
        description="Test Unmute API - Simula M5Stack Atom Echo"
    )
    parser.add_argument(
        "--host",
        default="localhost",
        help="Hostname server Unmute (es: your-vm.lightning.ai)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=443,
        help="Porta WebSocket (default: 443)"
    )
    parser.add_argument(
        "--no-ssl",
        action="store_true",
        help="Disabilita SSL (usa ws:// invece di wss://)"
    )
    parser.add_argument(
        "--voice",
        default="Anne",
        help="Voice da usare (default: Anne)"
    )
    parser.add_argument(
        "--connection-test",
        action="store_true",
        help="Esegui solo test connessione"
    )

    args = parser.parse_args()

    print(f"\n{'='*60}")
    print(f"🎭 TEST UNMUTE API - M5Stack Atom Echo Simulator")
    print(f"{'='*60}")
    print(f"Host: {args.host}")
    print(f"Port: {args.port}")
    print(f"SSL:  {not args.no_ssl}")
    print(f"Voice: {args.voice}")
    print(f"{'='*60}\n")

    if args.connection_test:
        # Test solo connessione
        await test_connection_only(args.host, args.port, not args.no_ssl)
    else:
        # Test completo
        tester = UnmuteAPITester(args.host, args.port, not args.no_ssl, args.voice)
        await tester.run()

    print(f"\n✅ Test completato!\n")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print(f"\n\n⚠️  Test interrotto da utente")
    except Exception as e:
        print(f"\n\n❌ Errore fatale: {e}")
        import traceback
        traceback.print_exc()
