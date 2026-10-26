# C++ Audio Transcription Pipeline

A Windows C++17 command-line application that converts audio files into
timestamped JSON transcripts using whisper.cpp.

## Features

- Configurable input extensions: WAV, MP3, M4A, and FLAC.
- FFmpeg conversion to 16 kHz, mono, signed 16-bit PCM.
- Configurable chunk duration for processing longer recordings.
- Segment timestamps relative to the original recording.
- Combined transcript text for downstream processing.
- JSON configuration with validation.
- Console and rotating file logs.
- Input validation and nonzero exit codes on failure.
- Protection against overwriting an existing output file.
- Automatic temporary-audio cleanup on normal exit and handled exceptions.

## Technology

- C++17 and CMake
- whisper.cpp v1.7.6
- FFmpeg executable
- nlohmann/json v3.11.3
- spdlog v1.14.1

CMake fetches the C++ dependencies. FFmpeg and the Whisper model are
installed separately.

## Requirements

- Windows x64
- Visual Studio with the Desktop development with C++ workload
- CMake and Git
- FFmpeg
- Whisper model: ggml-base.en.bin
- Internet access during the first CMake configuration

The tested build environment uses Visual Studio 18 2026.

## Setup and Build

Clone this repository and open Developer Command Prompt:

```cmd
git clone https://github.com/nomanjaved-92/cpp-transcription-pipeline.git
cd cpp-transcription-pipeline
```

Place `ggml-base.en.bin` in the `models` directory. The model is not
included in Git.

Set `ffmpeg_path` in `config/config.json` to your installed FFmpeg
executable.

Build:

```cmd
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --parallel
```

If using another Visual Studio version, select its corresponding CMake
generator.

### Download the English Model

From the repository root, run:

```cmd
if not exist models mkdir models
curl.exe --fail --location --output models\ggml-base.en.bin "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"
```

Download this once before running the application.
The model is excluded from Git.

### Configure FFmpeg

Install FFmpeg separately and set `ffmpeg_path` in
`config/config.json` to the full path of `ffmpeg.exe`.

Verify that executable, for example:

```cmd
"D:\Tools\ffmpeg\bin\ffmpeg.exe" -version
```

## Configuration

Example `config/config.json`:

```json
{
  "language": "en",
  "threads": 4,
  "model_path": "models/ggml-base.en.bin",
  "ffmpeg_path": "D:/Tools/ffmpeg/bin/ffmpeg.exe",
  "allowed_extensions": [".wav", ".mp3", ".m4a", ".flac"],
  "chunk_seconds": 30,
  "logging": {
    "level": "info",
    "file": "logs/transcription.log",
    "max_size_mb": 5,
    "max_files": 3
  }
}
```

Paths are resolved relative to the current working directory.
Run the application from the repository root.

Configuration changes do not require rebuilding.
The base.en model supports English; another language requires a
compatible multilingual model.

## Usage

```cmd
build\Release\transcription_app.exe "examples\demo.mp3" "output\demo.json"
```

Optional configuration file:

```cmd
build\Release\transcription_app.exe "examples\demo.mp3" "output\custom.json" "config\config.json"
```

Choose a new output filename for each run.

Exit codes:

- `0`: success
- `1`: failure

## Output

Illustrative output, not a measured transcription:

```json
{
  "language": "en",
  "duration_seconds": 4.5,
  "chunks_processed": 1,
  "text": "Hello, this is a transcription demo.",
  "segments": [
    {
      "start": 0.0,
      "end": 4.0,
      "text": "Hello, this is a transcription demo."
    }
  ]
}
```

Timestamps are in seconds. Each segment is offset to its position in
the original recording.

Downstream applications can consume `text` for text processing and
`segments` for timestamp-based navigation.

## Included Example

- `examples/demo.mp3`: demonstration recording.
- `examples/demo.transcript.json`: actual output from the application.

Recognition errors are retained in the example output.
Results can vary with the model, configuration, and runtime environment.

To reproduce the example, use the supplied configuration with
`chunk_seconds` set to 30. Write the new result to a different filename
because the application rejects existing output paths.

## Processing Flow

1. Load and validate configuration.
2. Validate the input file and extension.
3. Load the Whisper model once.
4. Convert the input into temporary normalized PCM using FFmpeg.
5. Read and transcribe one configured chunk at a time.
6. Trim segment whitespace and assemble the combined transcript.
7. Apply global timestamp offsets and save JSON.
8. Clean up temporary audio.

WASAPI is not needed for this file-based implementation. It could be
added later to capture microphone or system audio.

## Long Audio and Tradeoffs

Audio buffers are bounded by chunk duration rather than recording length.
The model is reused across chunks.

The entire normalized recording is stored temporarily on disk.
Transcript text and segments remain in memory until JSON is written.

Chunks use fixed boundaries without overlap or silence detection.
Words crossing boundaries may lose context. Recognition can also be
inaccurate for names, accents, noise, and specialized vocabulary.

The current application processes one file per invocation. It does not
implement checkpoints, automatic retries, process timeouts, live capture,
or a concurrent upload API.

## Manual Validation

The following tests passed locally:

- MP3 transcription to timestamped JSON.
- A 26.28-second recording processed as three 10-second chunks.
- Global timestamps across chunk boundaries.
- Missing input rejected with exit code 1.
- Unsupported extension rejected with exit code 1.
- Corrupt MP3 rejected with exit code 1.
- Existing output rejected with exit code 1.

This is manual validation, not an automated test suite.

## Further Documentation

- [Service design](docs/design.md)
