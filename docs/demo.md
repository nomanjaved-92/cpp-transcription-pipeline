# Interview Demo

Run commands from the repository root using Developer Command Prompt.

## 1. Explain the Pipeline

The application validates an audio file, converts it with FFmpeg,
transcribes it with whisper.cpp, and produces timestamped JSON.

Configuration and logging are separate components.
The model loads once per invocation.

## 2. Show Configuration

Open config/config.json.

Explain model_path, ffmpeg_path, threads, chunk_seconds,
allowed_extensions, and logging settings.

Use chunk_seconds = 10 to demonstrate multiple chunks with the
26.28-second sample. Restore 30 afterward.

## 3. Run Transcription

Choose an output filename that does not already exist:

```cmd
build\Release\transcription_app.exe "samples\demo.mp3" "output\interview_demo.json"
echo %ERRORLEVEL%
```

Expected exit code: 0.

Show the chunk progress in the console.

## 4. Inspect the Result

```cmd
notepad output\interview_demo.json
```

Explain:
- Combined text for downstream processing
- Timestamped segments
- Duration and number of chunks
- Global offsets across chunks

Acknowledge any recognition errors instead of editing the transcript
and presenting it as raw model output.

## 5. Demonstrate a Failure

```cmd
build\Release\transcription_app.exe "samples\missing.mp3" "output\missing_demo.json"
echo %ERRORLEVEL%
```

Expected: clear error and exit code 1.

## 6. Discuss Engineering Tradeoffs

- Audio buffers are bounded, while temporary disk usage grows with duration.
- Fixed chunk boundaries can reduce recognition continuity.
- Transcript results are accumulated in memory.
- The CLI processes one file per invocation.
- WASAPI would be an optional live-capture extension.
- The proposed concurrent service is documented separately in design.md.
