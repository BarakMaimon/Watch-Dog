# Watch-Dog

![C](https://img.shields.io/badge/C-ANSI%2FC99-A8B9CC?logo=c&logoColor=white)
![POSIX](https://img.shields.io/badge/POSIX-Signals%20%7C%20Semaphores%20%7C%20pthreads-0078D4)
![Platform](https://img.shields.io/badge/Platform-Linux-FCC624?logo=linux&logoColor=black)

A fault-tolerant process watchdog implemented in C using POSIX signals, named semaphores, and a task scheduler. Watch-Dog keeps your application alive — and keeps itself alive too.

Most watchdog systems have a single point of failure: if the watchdog crashes, nothing is watching the watcher. This implementation solves that with **bidirectional mutual monitoring** — the user process and the watchdog process each independently monitor the other using signal-based heartbeats. Either side can detect a crash and revive the other.

---

## How It Works

Two processes run side by side and exchange a `SIGUSR1` heartbeat every second. Each side counts incoming signals over a 5-second window. If the count hits zero, it means the other process stopped responding — and the survivor immediately relaunches it.

```
┌─────────────────────────────────────────────────────┐
│               User Process (your app)               │
│                                                     │
│  WDStart() ──► background scheduler thread          │
│                     │                               │
│          ┌──────────┼──────────┐                    │
│          ▼          ▼          ▼                    │
│     SendSignal  CheckCounter  CheckStopFlag         │
│     (1s tick)   (5s window)   (shutdown poll)       │
└──────────┼──────────┼──────────────────────────────┘
           │ SIGUSR1  │ detect crash → fork + exec
           │          │ wd_exec.out
           ▼          ▼
┌─────────────────────────────────────────────────────┐
│               Watchdog Process                      │
│                                                     │
│  WDStart() ──► background scheduler thread          │
│                     │                               │
│          ┌──────────┼──────────┐                    │
│          ▼          ▼          ▼                    │
│     SendSignal  CheckCounter  (no-op)               │
│     (1s tick)   (5s window)                         │
└──────────┼──────────┼──────────────────────────────┘
           │ SIGUSR1  │ detect crash → execv user argv
           └──────────┘
```

### Heartbeat cycle

| Time | Action |
|---|---|
| Every 1 s | Each process sends `SIGUSR1` to the other |
| `SIGUSR1` received | `global_counter++` in the signal handler |
| Every 5 s | Each process checks `global_counter` |
| Counter = 0 | Other process is dead → revive it |
| Counter 1–3 | Low responsiveness → log warning |
| Counter ≥ 4 | Healthy → reset counter and continue |

### Startup sequence

1. User calls `WDStart(argc, argv)`.
2. Checks the `wd_env` environment variable to determine its own role.
3. First run: spawns the watchdog via `fork()` + `exec("wd_exec.out")`.
4. Both sides create a scheduler and launch a background `pthread`.
5. A named POSIX semaphore (`sem_load`) synchronises the handshake — the user process blocks until the watchdog confirms it is ready.
6. Both schedulers enter their heartbeat loops; `WDStart` returns to the caller.

### Revival

- **Watchdog detects user crash** → calls `execv(argv[0], argv)` — replaces itself with the user program using the original arguments.
- **User detects watchdog crash** → `fork()` + `exec("wd_exec.out")` — spawns a fresh watchdog.
- Identity (who is "user" and who is "watchdog") is maintained across revivals through two environment variables: `usr_env` (user PID) and `wd_env` (watchdog PID).

### Graceful shutdown

1. User calls `WDStop()`.
2. The revival task is removed from the scheduler so the watchdog is not restarted.
3. `SIGUSR2` is sent to the watchdog up to 5 times (1 s apart).
4. The watchdog's `SIGUSR2` handler sets its stop flag; its scheduler exits cleanly.
5. The background thread joins, semaphores are unlinked, environment variables are cleared.

---

## API

Declared in `watchdog.h`. Link against `lib_wd.so`.

```c
/*
 * Start the watchdog. Spawns the WD process, establishes heartbeats, and
 * returns to the caller. The watchdog runs in a background thread for the
 * lifetime of the process.
 *
 * argc / argv  — pass main()'s argc and argv directly.
 */
int WDStart(int argc, char *argv[]);

/*
 * Stop the watchdog gracefully. Signals the WD process to exit, waits for
 * the background thread to finish, and cleans up all resources.
 */
void WDStop(void);
```

### Minimal integration

```c
#include "watchdog.h"

int main(int argc, char *argv[])
{
    WDStart(argc, argv);

    /* your application logic here — watchdog is now active */

    WDStop();
    return 0;
}
```

Compile and link:

```bash
gcc -o my_app my_app.c -L. -l_wd -Wl,-rpath,.
```

---

## Signals

| Signal | Direction | Meaning |
|---|---|---|
| `SIGUSR1` | User → WD, WD → User | Heartbeat — "I am alive" |
| `SIGUSR2` | User → WD | Graceful shutdown — "stop watching, exit cleanly" |

---

## Project Structure

```
Watch-Dog/
├── watchdog.h          — Public API (WDStart, WDStop)
├── watchdog.c          — Core logic: scheduler tasks, signal handlers, revival
├── wd_exec.c           — Entry point for the watchdog process
├── client_exec.c       — Example client (demonstrates WDStart / WDStop usage)
├── WDLogger.h          — Logger API
├── WDLogger.c          — Buffered logger with semaphore-guarded file flush
├── WDLogger_test.c     — Unit tests for the logger
├── WD_psudo.md         — Design pseudocode and notes
└── makefile
```

### Internal scheduler tasks

| Task | Interval | Role |
|---|---|---|
| `WDSendSignal` | 1 s | Send `SIGUSR1` to the monitored process |
| `WDCheckCounter` | 5 s window, checked at 6 s | Inspect heartbeat count; revive if zero |
| `WDCheckStopFlag` | 1 s | Poll the stop flag (user process only) |
| `WDLogWriteToFile` | 10 s | Flush in-memory log buffer to `logger.txt` |

---

## Building

The watchdog depends on a data-structures library (`scheduler`, `priority_queue`, `uid`, `task`, `dlist`, `sorted_list`, `dvector`). The makefile expects these at `../../ds/src/` relative to this directory.

```bash
# Build everything
make

# Build only the shared library
make lib_wd.so

# Build the watchdog executable
make wd_exec.out

# Build the example client
make client_exec.out

# Clean
make clean
```

Artifacts:

| File | Description |
|---|---|
| `lib_wd.so` | Shared library — link your application against this |
| `wd_exec.out` | Watchdog process binary — must be in the same directory as your app |
| `client_exec.out` | Example client to test the full setup |

---

## Running

```bash
# Run the example client — it starts the watchdog automatically
./client_exec.out

# Kill the watchdog mid-run to observe auto-revival
kill -9 <wd_pid>

# Kill the client mid-run to observe auto-revival
kill -9 <client_pid>
```

Both processes log their activity to `logger.txt` in the working directory.

---

## Logger

`WDLogger` is a lightweight, thread-safe logger used internally by the watchdog. It buffers messages in memory and flushes to disk every 10 seconds (or on demand). A named POSIX semaphore (`sem_write`) prevents concurrent writes from the two processes.

```c
/* Create a logger (capacity = initial buffer slots) */
WDLogger *WDLoggerCreate(const char *filename, size_t capacity);

/* Write a message to the buffer */
int WDLogWrite(WDLogger *logger, const char *msg);

/* Flush buffer to the log file */
int WDLogWriteToFile(WDLogger *logger);

/* Free all resources */
void WDLoggerDestroy(WDLogger *logger);
```

Log output (`logger.txt`) example:
```
[WD]  Sent SIGUSR1 to 18423
[USR] Sent SIGUSR1 to 18419
[WD]  Counter check: 5 signals received
[USR] Counter check: 4 signals received
[USR] WD unresponsive — reviving wd_exec.out
```

---

## Design Notes

**Why mutual monitoring?**
A traditional watchdog is a single point of failure. If the watcher crashes, nothing restarts it. By having each process watch the other, the system degrades gracefully: as long as one side is alive, the other gets revived.

**Why signals instead of shared memory?**
`SIGUSR1` delivers the heartbeat asynchronously with no shared buffer or locking required. The signal handler does only one thing — increment an integer — making it async-signal-safe.

**Why environment variables for identity?**
When a process is revived via `exec`, file descriptors and memory are replaced, but environment variables are inherited. Storing the PIDs in `usr_env` / `wd_env` lets each revived process immediately know its role and its partner's PID without any additional IPC.

**Why a task scheduler instead of `sleep` loops?**
The custom scheduler (built on a priority queue and UID system) allows multiple periodic tasks at different intervals to share one thread cleanly, and makes it trivial to remove a task (e.g. the revival check) during shutdown without stopping other tasks.

---

## License

MIT
