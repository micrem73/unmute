# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Unmute is a real-time voice interaction system that wraps text LLMs with Kyutai's low-latency Speech-to-Text (STT) and Text-to-Speech (TTS) models. The system enables spoken conversations with any text-based LLM through WebSocket connections.

**Architecture**: Browser ↔ Backend (FastAPI) ↔ [STT, LLM, TTS services]

## Development Commands

### Backend Development

```bash
# Development mode with auto-reload
uv run fastapi dev unmute/main_websocket.py

# Production mode
uv run fastapi run unmute/main_websocket.py

# Run tests
uv run pytest -v

# Type checking
uv run pyright

# Linting and formatting
uv run ruff check --fix
uv run ruff format
```

### Frontend Development

```bash
cd frontend
pnpm install
pnpm run dev     # Development server
pnpm run build   # Production build
pnpm run lint    # Lint with max-warnings 0
```

### Pre-commit Hooks

Install pre-commit hooks before contributing:

```bash
pre-commit install --hook-type pre-commit
```

Hooks include: ruff (lint + format), pyright, trailing-whitespace, nbstripout, and frontend linting.

### Load Testing

```bash
uv run unmute/loadtest/loadtest_client.py --server-url ws://localhost:8000 --n-workers 16
```

### Docker Compose Deployment

```bash
# Start all services (requires HUGGING_FACE_HUB_TOKEN env var)
docker compose up --build

# Health check
curl http://localhost/api/v1/health
```

### Dockerless Deployment

Start each service in separate terminals:
```bash
./dockerless/start_frontend.sh  # Port 3000
./dockerless/start_backend.sh   # Port 8000
./dockerless/start_llm.sh        # 6.1GB VRAM
./dockerless/start_stt.sh        # 2.5GB VRAM
./dockerless/start_tts.sh        # 5.3GB VRAM
```

## High-Level Architecture

### Core Pipeline Flow

1. **User Audio Input** → Browser captures microphone via WebRTC
2. **STT Processing** → Backend forwards audio to STT server, receives real-time transcription
3. **LLM Generation** → Backend sends conversation history to LLM (VLLM/OpenAI-compatible), streams response
4. **TTS Synthesis** → Backend feeds LLM chunks to TTS server, receives audio
5. **Audio Output** → Backend streams audio back to browser via WebSocket

### Key Components

#### `unmute/main_websocket.py`
FastAPI entry point handling WebSocket connections at `/v1/realtime`. Routes client events to `UnmuteHandler`, manages health checks, voice endpoints, and Prometheus metrics. Enforces `MAX_CLIENTS=4` semaphore for concurrency control.

#### `unmute/unmute_handler.py`
Core orchestration class `UnmuteHandler` implementing `AsyncStreamHandler`:
- Manages conversation state machine: `waiting_for_user` → `user_speaking` → `bot_speaking`
- Coordinates STT, LLM, TTS via `QuestManager` (Quest-based task lifecycle)
- Handles VAD-based interruptions, pause detection (60% threshold), and user silence timeouts (7s)
- Implements audio buffering and synchronization via `output_queue`
- Special "bambola parlante" (talking doll) mode: buffers TTS response before playback

#### `unmute/llm/chatbot.py`
Manages conversation history and system prompts. Default system prompt is `BAMBOLA_INSTRUCTIONS` (talking doll personality). Handles message deltas, state tracking, and preprocessing for LLM API calls.

#### `unmute/stt/speech_to_text.py` and `unmute/stt/whisper_stt.py`
- `SpeechToText`: WebSocket client for Kyutai STT server, streams audio chunks, receives words + pause predictions
- `WhisperSTT`: Alternative local Whisper implementation using OpenAI API with 10-second sliding window, VAD filtering, and `bot_speaking` flag to prevent echo

#### `unmute/tts/text_to_speech.py`
WebSocket client for Kyutai TTS server. Streams LLM text chunks, receives PCM audio. Implements `RealtimeQueue` for audio synchronization with 4-frame buffer (`AUDIO_BUFFER_SEC`).

#### `unmute/openai_realtime_api_events.py`
Pydantic models for WebSocket protocol. Based on OpenAI Realtime API with Unmute-specific extensions:
- `UnmuteBambolaCordinoTirato` / `UnmuteBambolaCordinoRilasciato` - Talking doll trigger events
- `UnmuteBambolaBufferReady` / `UnmuteBambolaPlaybackStarted` - Buffer management events
- `UnmuteInterruptedByVAD` - Voice activity interruption signal

### Service Discovery and Scaling

`unmute/service_discovery.py` implements instance-finding logic for horizontal scaling:
- Redis-based service registry (when available)
- Fallback to direct URL connections
- `@async_ttl_cached` decorator for health check caching (500ms TTL)

### Recording and Debugging

- **Recorder**: `unmute/recorder.py` saves WebSocket events to `RECORDINGS_DIR` (if configured)
- **Dev Mode**: Press 'D' in UI (when `ALLOW_DEV_MODE=true` in `useKeyboardShortcuts.ts`)
- **Subtitles**: Press 'S' to toggle user/bot transcription display
- **Debug Dict**: `self.debug_dict` in `unmute_handler.py` - add metrics visible in dev mode

### Character/Voice Configuration

`voices.yaml` defines available voices with:
- `name`: Voice identifier
- `good`: Whether to expose in production
- `instructions`: System prompt configuration (types: `smalltalk`, `quiz_show`, `constant`, `news`, `unmute_explanation`)
- `source`: Audio sample metadata (Freesound, VCTK dataset, custom)

System prompts with dynamic elements (e.g., quiz questions) are in `unmute/llm/system_prompt.py`.

**Important**: Backend caches `voices.yaml` on startup - restart required after changes.

## LLM Configuration

Default setup uses external LLM (Together.ai). To switch LLMs, modify `docker-compose.yml` environment:

```yaml
backend:
  environment:
    - KYUTAI_LLM_URL=https://api.openai.com/v1
    - KYUTAI_LLM_MODEL=gpt-4o
    - KYUTAI_LLM_API_KEY=sk-...
```

For local VLLM, uncomment the `llm` service block in `docker-compose.yml`.

## WebSocket Protocol

Custom protocol based on OpenAI Realtime API (`/v1/realtime` endpoint). See `docs/browser_backend_communication.md` for detailed message specs.

**Client Events**: `SessionUpdate`, `InputAudioBufferAppend`, bambola trigger events
**Server Events**: `ResponseCreated`, `ResponseTextDelta`, `ResponseAudioDelta`, `InputAudioBufferSpeechStarted/Stopped`

Frontend reference: `frontend/src/`, Python client reference: `unmute/loadtest/loadtest_client.py`

## Code Style and Type Checking

- **Python**: Pyright strict mode with some relaxations (see `pyproject.toml`)
  - `reportUnknownX` disabled for MyPy-like behavior
  - Google docstring convention (`D417` enforced)
  - Ruff linter: bugbear, pep8, pyflakes, isort
- **Dependencies**: Managed via `uv` with `pyproject.toml` (Python 3.12 only)
- **Dev Dependencies**: jupyter, pytest, pytest-asyncio, pyright, ruff, pre-commit, pyinstrument

## Important Notes

- **Modified Files**: This repository has custom "bambola parlante" modifications:
  - `unmute/main_websocket.py`: Custom event handlers for doll trigger events
  - `unmute/unmute_handler.py`: `BambolaBufferState` class, audio buffering logic
  - `unmute/stt/whisper_stt.py`: Local Whisper STT implementation (switched from Kyutai STT)
  - `unmute/llm/chatbot.py`: Default `BAMBOLA_INSTRUCTIONS` system prompt
  - System prompt modification re-enabled in `update_session()` to allow voice-specific instructions

- **STT Configuration**: Switched to local Whisper STT implementation to support Italian language recognition:
  - Uses OpenAI Whisper API with 10-second sliding window
  - Implements VAD filtering to prevent audio feedback
  - Audio listening is disabled when bot is speaking (`bot_speaking` flag) to avoid feedback loops
  - Bot interruption capability has been disabled - users must wait for bot to finish speaking

- **LLM Configuration**: Currently using DeepSeek (deepseek-ai/DeepSeek-V3) for inference via Together.ai API

- **GPU Requirements**: 16GB+ VRAM recommended. Services can run on separate GPUs using `deploy.resources.reservations` in docker-compose.

- **Interruption Handling**: Bot interruption is currently disabled in this version. Users must wait for the bot to complete its response before speaking again. This prevents feedback issues when using Whisper STT with local audio processing.

- **Testing**: Unit tests use pytest + pytest-asyncio. CI runs pre-commit and backend tests via GitHub Actions.
