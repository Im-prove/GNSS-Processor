# Beginner Code Walkthrough

This is the detailed, source-oriented guide: it follows the implementation functions and explains their behavior and trade-offs. For a shorter flow-first explanation in pseudocode, see [`CODE_WALKTHROUGH.md`](CODE_WALKTHROUGH.md). The catalog at the end describes every file in the project.

This guide explains the implementation in small steps: what each part does, why it is here, and what it costs. Line references refer to the current source files; if code moves, search by the nearby function name.

## 1. Public data and class: `gnss_processor.h`

### Lines 1–3: header setup and standard types

- `#pragma once` asks the compiler to include this header only once per source file. It avoids duplicate struct/class declarations when headers are included along multiple paths.
- `<cstdint>` provides a fixed-width unsigned integer type for millisecond timestamps. `uint64_t` represents nonnegative values over a long span.
- `<mutex>` provides `std::mutex`, which protects the processor's private stream state during concurrent calls.

**Trade-off:** the lock has a small runtime cost and makes the class non-copyable by default. It prevents races; it does not order packets submitted concurrently by timestamp. The first call to acquire the lock is processed first.

### Lines 5–12: `GnssFrame`

This `struct` bundles one sensor sample. Its public fields make aggregate initialization simple, but it does not validate values itself; callers can create bad frames.

- `timestamp_ms`: source timestamp in milliseconds. All frames are assumed to use one clock and epoch.
- `lat`, `lon`: `double` values for geographic coordinates and useful fractional precision.
- `hdop`: horizontal dilution of precision. `float` is enough for the expected receiver metric.
- `satellites_used`: number used in the solution.
- `fix_type`: the prompt's integer code: 0 no fix, 1 standard GPS, 2 mixed.

Separating data from processing policy makes the frame easy to pass around. The downside is that every processor entry point must defend against invalid members.

### Lines 14–20: public processor interface

`GnssProcessor` owns stream history and its quality `Config`. `process(const GnssFrame&)` takes a const reference: it avoids copying the frame and promises not to alter the caller's data. It returns `true` for accepted frames, `false` for rejected frames, and prints only accepted positions to meet the assignment. `reset()` clears timestamp history at a known session boundary.

The interface comment records a state policy: a newer rejected packet advances received-stream time, but does not print a position.

### Private helpers and state

- `validate(...) const` checks fields that can be judged from one frame without history. The const qualifier says it does not modify the processor.
- `mutex_` protects all state in this object.
- `has_received_frame_` separates “no packet yet” from a legitimate timestamp of zero.
- `last_received_timestamp_ms_` is used for ordering and gap checks.
- `config_` stores this instance's HDOP limit, minimum satellite count, maximum timestamp gap, and whether to reject `(0, 0)`.

The processor intentionally does not store the last accepted frame or timestamp. That could support a freshness API, but no such API is required here; callers currently get a boolean and accepted coordinates are printed.

`Config` is useful when the same processing code serves different receivers or applications with different policies. `reject_null_island` defaults to true for the assumed land robot; set it to false for marine or other applications that can operate at `(0, 0)`. The example's expected period is not included because this processor does not reject jitter around the nominal 100 ms interval.

## 2. Processor logic: `gnss_processor.cpp`

### Lines 1–6: headers

The implementation includes its own interface, `<cmath>` for `std::isfinite`, `<iomanip>` for decimal formatting, `<iostream>` for required output, `<mutex>` for lock guards, and `<sstream>` to format a line separately before writing it.

**Trade-off:** direct `std::cout` output is convenient for the assignment, but a library tied to console output is awkward in a robot app and might block a real-time path. A production design should return a result or accept an injected logger.

### Constructors and output lock

The unnamed namespace gives `output_mutex` internal linkage, so it is private to this source file. It serializes output lines from different processor objects; each object's `mutex_` only protects that object's state. This shared output lock matters if separate instances can print at the same time. The acceptance thresholds are stored in each object's `Config`, allowing applications to use different limits.

- Default HDOP maximum 5 and satellite minimum 4 are engineering assumptions based on field meaning and starter expectations.
- Default maximum gap 1500 ms is 15 nominal periods at 10 Hz. It tolerates jitter and brief packet loss, but rejects a longer outage.

The default constructor delegates to a constructor that accepts a `Config`. `reset()` locks state and clears the received-time flag/value so the next packet starts a new timestamp session.

### Lines 15–21: `validate`

The function returns true only if every condition is true. `&&` short-circuits at the first failure.

1. Latitude is finite and in `[-90, 90]`.
2. Longitude is finite and in `[-180, 180]`.
3. When `config.reject_null_island` is true, the exact `(0, 0)` coordinate is rejected as a presumed default/uninitialized value; a coordinate with just one zero component remains valid. It defaults to true for land-based mobile robots because this ocean location is outside their expected operating area ([map](https://www.openstreetmap.org/?mlat=0&mlon=0#map=5/0/0)) and all-zero coordinates are common defaults. This is an application heuristic, not a GNSS rule. Set it to false for marine robots or applications that may operate there.
4. HDOP is finite and in `[0, config.max_hdop]` (default maximum 5).
5. At least `config.min_satellites` were used (four by default).
6. Fix type is 1 or 2.

`std::isfinite` is important because NaN comparisons are false in surprising ways; a range check alone is not a reliable NaN rejection. Boundaries are inclusive because geographic coordinate limits include the poles and antimeridian, and HDOP zero is allowed by this implementation's stated assumption.

**Trade-off:** these checks catch impossible values, the exact `(0,0)` default, and weak receiver quality. They do not detect plausible-but-wrong positions, spoofing, multipath error, or sudden motion that the robot cannot physically make. No kinematic speed check was added because the assignment provides no vehicle dynamics.

### Lines 23–27: locking and timestamp order

`process()` first locks `mutex_`. A lock guard unlocks automatically on every return path. When a received frame already exists, an equal or older timestamp is rejected. On the first call, any timestamp—including zero—is allowed because `has_received_frame_` is false.

**Why reject duplicates/older frames?** They may be repeated or delayed measurements. Accepting them could make downstream position appear to move backward in time.

### Lines 28–34: gap policy and received baseline

Only after confirming the new timestamp is greater does the code subtract the prior value. That avoids unsigned subtraction underflow. A gap strictly greater than `config.max_gap_ms` is flagged; exactly the configured limit passes (1500 ms by default).

The new timestamp is then recorded as the received baseline even for an excessive gap. This rejects the first frame after an outage, then allows a following packet roughly 100 ms later to recover.

**Trade-off:** dropping the first post-outage packet is deliberate. Alternatives include accepting it as a new epoch or requiring several consecutive good frames. The starter expectations motivated this policy.

### Quality decision

A long-gap frame has already been rejected. Otherwise `validate(frame)` decides quality. Quality failures return false without printing; the newer received timestamp remains the baseline, which allows recovery on the next regular packet.

The processor does not retain an accepted-time field. If a product needs freshness checks, add an explicit last-good-fix API rather than keeping unused internal state.

### Lines 39–46: printing and result

An `ostringstream` formats a position using fixed-point notation and six fractional digits. The code writes the resulting string to `cout` while holding a shared output lock. Formatting in a separate stream avoids leaving `std::cout` set to fixed notation or changing the caller's precision. It then returns `true`.

Six decimal places are a display choice, not an accuracy claim. Around the equator, a millionth of a latitude degree is roughly 0.11 m of coordinate granularity, while actual GNSS error can be much larger.
## 3. CSV replay: `replay_gnss.cpp`

### Lines 1–8: includes

The reader uses the public frame type, character classification, file and string streams, strings, and a vector. These are all C++ standard-library components, so no external CSV package is needed.

### Lines 10–16: `trim`

This helper removes whitespace from the beginning and end of a field. It accepts ordinary spaces around numbers and removes the carriage return on CRLF files. A field that is entirely whitespace becomes empty.

### Lines 18–34: integer syntax

`is_unsigned_integer` requires one or more digits and no sign; timestamps use this form so negative text cannot wrap to a huge unsigned value. `is_signed_integer` allows an optional plus/minus and then one or more digits; it is used for satellite count and fix type.

`std::isdigit` receives an `unsigned char`. Passing a negative signed `char` to C character-classification functions can be undefined for bytes outside ASCII.

### Lines 36–60: decimal syntax

`is_decimal` allows an optional sign, a decimal point, and an exponent like `e-3`, but requires at least one digit. It rejects strings such as `lon=as`, `NaN`, `inf`, hexadecimal numbers, and incomplete exponents before conversion.

### Lines 62–74: split and field validation

`parse_row` splits on commas and requires exactly six columns, trims every field, then checks its expected field type. Invalid longitude text never reaches `std::stod`.

**Limit:** this is not a general CSV reader. It does not implement quoted fields, escaped quotes, or commas inside quotes. Those are unnecessary for the documented scalar numeric fields; if the schema adds free text, use a tested CSV library.

### Lines 76–103: convert and catch errors

`std::stoull`, `std::stod`, `std::stof`, and `std::stoi` convert strings into the matching C++ types. The `consumed` index confirms the converter consumed the whole field. A `try`/`catch` turns invalid or out-of-range conversion exceptions into a malformed-row result rather than terminating replay.

Syntax validation makes accepted formats explicit; conversion handles representable range. Both layers matter: text may be syntactically numeric but too large for the target type.

### Lines 106–127: command line and header

Exactly one command-line argument is required: the CSV filename. Usage, file-open, missing-header, and invalid-header errors go to standard error and return status 2. The header is trimmed and compared with the one supported column order.

### Lines 129–143: replay rows

One `GnssProcessor` handles the whole file so timestamp history spans all rows. `rows` counts every data line after the header. A malformed row increments the rejected count, reports its source line number (first data row is line 2), and continues. A parsed frame is counted according to `process()`.

Malformed CSV rows never reach the processor and do not change its timestamp state. A syntactically valid but low-quality frame does reach `process()` and advances receive time when newer.

### Lines 145–152: read error and summary

An underlying stream read error returns status 2. Otherwise totals are printed and the program returns 0, even if some rows were rejected: bad rows are expected input handling, not a crash.

## 4. Tests: `test_gnss_processor.cpp`

### Lines 1–5: test includes

The tests need the processor, exit codes, standard streams, and `numeric_limits` for NaN and infinity values.

### Lines 7–15: `CHECK` macro

The macro evaluates an expression and prints `[PASS]` if it is true. If false, it prints `[FAIL]` to standard error and exits nonzero immediately. `do { ... } while (false)` makes a multi-statement macro behave syntactically like a single statement.

**Trade-off:** this avoids adding a test-framework dependency, but stops on the first failure and gives less diagnostic detail than named test functions in a full test framework.

### Lines 17–42: test cases

Each braced block creates a new processor so timestamp state from one test cannot leak into another. The cases cover normal acceptance; HDOP, fix type, and satellite rejection; both ends of latitude/longitude ranges; exact and excessive gaps; duplicate and out-of-order timestamps; NaN and infinity; negative HDOP; inclusive boundaries; zero as the initial timestamp; and recovery after quality or gap rejection.

`All 29 tests passed.` appears only after all 29 checks succeed. The expanded cases cover `(0, 0)` handling with the option enabled and disabled, valid zero-axis coordinates, custom limits, and reset. A `[PASS]` on a rejection test means `process()` correctly returned false. Accepted-position lines are emitted separately by valid frames inside the tests; rejected positions are intentionally silent.

## What this code does not promise

A `true` result does not prove a GNSS fix is safe for control. This exercise does not estimate accuracy, check age against wall-clock time, apply motion limits, detect spoofing, provide structured rejection reasons, automatically detect clock changes, or parse the full CSV standard. `reset()` is available for a caller that knows a new stream/session has begun. Those are production design decisions, not hidden guarantees.

## Project file catalog

### C++ implementation and input

- [`../gnss_processor.h`](../gnss_processor.h) is the public interface. It defines the measurement data structure and the processor class. The class exposes `process()` and `reset()`, accepts per-instance configuration, and keeps received-time state private behind a mutex.
- [`../gnss_processor.cpp`](../gnss_processor.cpp) implements the interface. The anonymous namespace holds the shared output mutex. `validate()` checks measurement values; `process()` enforces timestamp rules, updates state, and prints accepted coordinates.
- [`../replay_gnss.cpp`](../replay_gnss.cpp) is the standalone replay command. Small helpers trim whitespace, validate numeric spelling, parse six fields, and construct a frame. `main()` checks arguments and the header, processes each row, and prints counts.
- [`../test_gnss_processor.cpp`](../test_gnss_processor.cpp) is a dependency-free processor test program. It creates frames directly, checks expected return values, and exits at the first failed check.
- [`../gnss.csv`](../gnss.csv) is the recorded sample input for the replay program. Its header identifies the six expected fields; subsequent lines provide timestamp and measurement values.

### Assignment and submission documents

- [`../ASSIGNMENT.md`](../ASSIGNMENT.md) is the original prompt. It is the source of the requested features and any supplied starter-test expectations.
- [`../README.md`](../README.md) is the quick-start and project overview. It explains the behavior and assumptions and provides build/run commands for the supported command-line toolchains.
- [`../CANDIDATE_RESPONSE.md`](../CANDIDATE_RESPONSE.md) answers the assignment's design questions and reports implementation choices, verification, and limitations.
- [`../RESPONSE.md`](../RESPONSE.md) contains the AI assistance disclosure and verification statement requested for the submission.

### Guides and presentation material

- [`CODE_WALKTHROUGH.md`](CODE_WALKTHROUGH.md) explains the system flow and major algorithms using pseudocode. It also gives a quick description of every project file.
- [`ASSUMPTION_TRACEABILITY.md`](ASSUMPTION_TRACEABILITY.md) provides a review table connecting decisions to source locations, prompt/test evidence, rationale, and limits. Consult it when deciding whether a threshold is a requirement or an engineering choice.
- [`INTERVIEW_PREP.md`](INTERVIEW_PREP.md) offers likely discussion questions and answer outlines about the implementation, trade-offs, and next steps.
- [`SUBMISSION_CHECKLIST.md`](SUBMISSION_CHECKLIST.md) maps assignment items to their evidence and records status or remaining submission tasks.
- [`GNSS_Processor_Overview.pptx`](GNSS_Processor_Overview.pptx) is a single-slide visual summary intended for presentation or quick review.

Build executables from the C++ sources using the README commands.
