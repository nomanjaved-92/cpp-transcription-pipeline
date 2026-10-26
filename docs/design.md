# Part 2: Proposed Transcription Service

This document describes an extension of the implemented CLI.
The API, queue, storage service, and worker orchestration are design only.

## API

- POST /jobs: accept an audio upload and transcription settings.
  Return HTTP 202 with a job ID after durable storage and job registration.
- GET /jobs/{id}: return queued, running, succeeded, or failed status.
- GET /jobs/{id}/result: return the completed transcript or a result URL.

Authenticate requests and check job ownership before exposing status,
audio, or transcripts. Enforce upload size and duration limits.

## Concurrent Uploads

The API stores audio in object storage and creates a job in a database.
A durable queue carries job IDs and storage references.

Separate worker processes consume jobs and run transcription.
Start with one active transcription per worker to control memory usage.
Limit worker count and inference threads to avoid CPU oversubscription.

Workers may keep models loaded between jobs to reduce startup latency.

## Storage

Object storage:
- Original audio
- Completed transcript JSON
- Optional chunk results for resumable processing

Database:
- Job ID and owner
- Input and output object keys
- Model version and configuration snapshot
- Status, attempt count, timestamps, and error details

Workers use local temporary storage for normalized audio.
Retention policies remove expired audio, results, and abandoned uploads.

## Reliable Job Submission

Create the job record and an outbox event in one database transaction.
A dispatcher publishes outbox events to the queue.

This avoids losing a job if the API stops between saving the database
record and publishing a queue message.

Duplicate publication is possible, so workers must handle duplicate jobs.

## Retries and Recovery

Retry transient failures, such as temporary storage or network outages,
with exponential backoff and a maximum attempt count.

Do not automatically retry invalid audio or unsupported configuration.

Use worker heartbeats and expiring job leases so another worker can
recover work after a crash.

Assume queue delivery is at least once. Use conditional job updates and
attempt-specific output locations to prevent stale workers from
overwriting successful results.

Only mark a job successful after its result has been stored durably.
Move repeatedly failing jobs to a dead-letter queue for investigation.

## Long Recordings

The current CLI restarts the entire recording after failure.

A future worker could persist completed chunk results and resume from
the last checkpoint. Each checkpoint must identify the input, model,
configuration, chunk boundaries, and timestamp offset.

Overlapping chunks or silence-aware boundaries could improve continuity.
Overlap would require duplicate-text reconciliation when merging results.

## Observability

Include job ID, attempt, stage, and chunk index in structured logs.

Track:
- Queue depth and waiting time
- Processing time and audio duration
- Success, failure, and retry counts
- Worker memory, CPU, and temporary disk usage

Avoid recording raw audio or complete transcripts in routine logs.

## Implementation Boundary

Implemented:
File-based CLI, configuration validation, logging, FFmpeg conversion,
bounded audio chunks, timestamped JSON, and basic error handling.

Proposed:
Upload API, concurrent workers, durable queue, object storage,
job database, retries, leases, checkpoint recovery, and service metrics.
