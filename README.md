# GNSS Processor

C++17 implementation of a small GNSS frame validator and CSV replay tool. It uses the C++ standard library and has build instructions for Windows, Linux, and macOS.

## Project files

| File | Purpose |
| --- | --- |
| `gnss_processor.h/.cpp` | Frame type, configurable limits, validation, timestamp handling, and accepted-position output |
| `test_gnss_processor.cpp` | 29 processor behavior checks |
| `replay_gnss.cpp` | Six-column CSV reader and replay summary |
| `gnss.csv` | Sample recording |
| `ASSIGNMENT.md` | Original assignment prompt |
| `CANDIDATE_RESPONSE.md` | Answers to the four assignment questions |
| `RESPONSE.md` | Record of AI assistance used for this submission |
| `docs/CODE_WALKTHROUGH.md` | High-level flow and pseudocode |
| `docs/BEGINNER_CODE_WALKTHROUGH.md` | Source-oriented explanations and trade-offs |
| `docs/INTERVIEW_PREP.md` | Technical questions and design limitations |
| `docs/ASSUMPTION_TRACEABILITY.md` | Rationale and source for each implementation decision |
| `docs/SUBMISSION_CHECKLIST.md` | Assignment coverage and outstanding submission details |
| `docs/GNSS_Processor_Overview.pptx` | One-slide design summary |

## Behavior

`GnssProcessor::process()` returns `true` and prints a position for an accepted frame. It returns `false` and prints nothing for a rejected frame. The processor stores only the latest received timestamp; it does not retain the last accepted position. The caller must retain accepted results if it needs a last-known-good fix or freshness check.

The default configuration is:

| Limit | Default | Meaning |
| --- | ---: | --- |
| `max_hdop` | `5.0` | Reject HDOP above this value |
| `min_satellites` | `4` | Reject frames using fewer satellites |
| `max_gap_ms` | `1500` | Reject newer frames with a larger time gap |
| `reject_null_island` | `true` | Reject the exact `(0, 0)` coordinate |

Pass a `GnssProcessor::Config` to tune these policies. The Null Island check defaults to enabled for land-based robots; set `reject_null_island = false` for a marine robot or another system that may legitimately operate at `(0, 0)`. For example: `GnssProcessor::Config config; config.reject_null_island = false; GnssProcessor processor(config);`. The configuration constructor does not validate its values; callers should provide finite, nonnegative HDOP limits, a sensible satellite minimum, and a positive gap limit. `reset()` clears timestamp history so the next frame starts a new stream session.

## Validation rules and assumptions

- Timestamps are milliseconds from one source clock. After the first frame, they must strictly increase.
- At a nominal 10 Hz, the expected interval is 100 ms. The processor allows normal jitter; it rejects gaps above the configured limit. A gap exactly equal to the default 1500 ms is accepted.
- Latitude and longitude must be finite and within the general geographic bounds `[-90, 90]` and `[-180, 180]`. These are coordinate-domain checks, not GNSS accuracy limits.
- When `reject_null_island` is true, the exact `(0, 0)` coordinate is rejected as a presumed uninitialized placeholder. The default assumes a land-based mobile robot: this ocean location is outside its expected operating area, and all-zero coordinates are a common default before a receiver supplies a fix. This is an application heuristic, not a GNSS rule; `(0, 0)` is a real geographic location ([map](https://www.openstreetmap.org/?mlat=0&mlon=0#map=5/0/0)). Marine and other applications can disable the check in `Config`.
- HDOP must be finite, nonnegative, and no greater than `max_hdop`.
- `satellites_used` must be at least `min_satellites`.
- Fix types 1 and 2 are accepted. Fix type 0 and unknown values are rejected.

The original starter test rejects a 1600 ms gap; the 1500 ms cutoff is the implementation's chosen precise boundary. A four-satellite minimum is common for a standard 3D GPS solution, but receiver modes and aided solutions may differ. These assumptions should be checked against the target receiver and robot.

## Timestamp handling

For each newer frame, the processor calculates the gap and records the frame timestamp as the new received-time baseline before checking quality. A quality-rejected frame therefore advances stream time. A gap-rejected frame also becomes the new baseline, allowing a following normally spaced frame to recover. Duplicate and older frames do not change state.

Example: a valid frame at 1000 ms, a poor-quality frame at 1100 ms, and a valid frame at 1200 ms produce accept, reject, accept. After a long outage, the first frame is rejected; a regular frame after it may pass. Calling `reset()` clears the received-time baseline when the caller starts a new receiver session. The processor does not track an accepted-time value.

## CSV input

The replay tool expects this header and column order:

```csv
timestamp_ms,lat,lon,hdop,satellites_used,fix_type
```

Rows must contain exactly six unquoted numeric fields. Surrounding whitespace is trimmed. Timestamps must be unsigned decimal digits; coordinates and HDOP accept decimal notation with an optional sign and exponent; satellite count and fix type must be integers. Text such as `lon=as`, NaN/Inf spellings, missing fields, extra columns, invalid syntax, and conversion overflow are rejected as malformed rows. The tool reports the source line number and continues. A syntactically valid number can still fail GNSS validation.

The parser supports LF and CRLF line endings. It is not a general quoted-CSV parser. Missing or incorrect headers, file-open failures, and read failures stop replay with status 2.

## Build and run

Run from the project directory with a C++17 compiler. The examples use GCC or a compatible Clang. `-pthread` enables mutex support on toolchains that require it.

### Linux

```sh
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic test_gnss_processor.cpp gnss_processor.cpp -o test_gnss
./test_gnss
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic replay_gnss.cpp gnss_processor.cpp -o replay_gnss
./replay_gnss gnss.csv
```

### macOS

```sh
clang++ -std=c++17 -pthread -Wall -Wextra -Wpedantic test_gnss_processor.cpp gnss_processor.cpp -o test_gnss
./test_gnss
clang++ -std=c++17 -pthread -Wall -Wextra -Wpedantic replay_gnss.cpp gnss_processor.cpp -o replay_gnss
./replay_gnss gnss.csv
```

### Windows with MSYS2 UCRT64 GCC

Add `C:\msys64\ucrt64\bin` to `PATH` so GCC and the generated programs can find their runtime DLLs. In PowerShell:

```powershell
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic test_gnss_processor.cpp gnss_processor.cpp -o test_gnss.exe
.\test_gnss.exe
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic replay_gnss.cpp gnss_processor.cpp -o replay_gnss.exe
.\replay_gnss.exe gnss.csv
```

## Tests and replay output

The test program has 29 cases covering normal frames, quality/fix rules, coordinate bounds, `(0,0)` handling with the default and disabled setting, custom limits, timestamp boundaries, rejected-frame recovery, and `reset()`. A `[PASS]` on a rejection test means `process()` returned `false` as expected. Accepted-position output is separate; rejected positions are intentionally silent.

The supplied CSV contains 500 rows. Replay accepted 459 and rejected 41. Malformed-row handling was also checked with CRLF input containing `lon=as` and a text satellite count; the tool reported those rows and continued.

## Limits before robot use

- The `(0,0)` rejection can discard a legitimate coordinate; disable it in `Config` for marine or other applications where the point is in the operating area.
- `Config` values are not validated by the constructor.
- The processor returns only `bool`, does not retain the last accepted fix, and has no freshness API.
- HDOP and satellite count do not guarantee position integrity or accuracy. Add receiver integrity data and motion checks based on robot requirements.
- Output is written to `std::cout` to meet the assignment. Production code should return a result or use a logger callback.
- The per-instance mutex protects state but makes `GnssProcessor` non-copyable by default. It does not reorder concurrent input by timestamp.
- CSV parsing supports six unquoted numeric fields only.

For decision rationale, see `docs/ASSUMPTION_TRACEABILITY.md`. For source explanations, see `docs/CODE_WALKTHROUGH.md` and `docs/BEGINNER_CODE_WALKTHROUGH.md`. The AI assistance record is in `RESPONSE.md`.
