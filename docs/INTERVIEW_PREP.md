# Interview Notes: GNSS Processor

Use the notes to explain the actual implementation. The cutoffs below are project choices; do not describe them as universal GNSS requirements.

## Short design summary

The program accepts a frame only when its timestamp is new enough and its values meet the configurable quality checks. It tracks the latest received timestamp; rejected newer packets still advance that baseline. `Config::reject_null_island` defaults to true for land-based robots and can be turned off for marine use. The CSV replay tool checks file shape and numeric fields before passing frames to the processor. All 29 processor checks passed with MSYS2 GCC and C++17 warning flags. Before deploying this on a robot, the thresholds, freshness policy, and integrity checks need receiver- and vehicle-specific review.

## Questions an interviewer may ask

### Why split `validate()` and `process()`?

`validate()` checks values in one frame. `process()` handles rules that depend on earlier frames, such as timestamp order and gaps. This keeps the two kinds of checks separate. The current `bool` result does not identify which check rejected a frame.

### Why check `isfinite()` as well as numeric ranges?

NaN and infinity are not ordinary numbers. NaN comparisons return false, so range checks alone are not a clear way to reject it. `std::isfinite()` explicitly rejects both NaN and infinity before the range checks are applied.

### Why allow latitude from -90 to 90 and longitude from -180 to 180?

Those are geographic latitude/longitude domains: latitude runs from pole to pole, and longitude wraps at the antimeridian. They are not accuracy limits. A coordinate can be inside those ranges but still be wrong or nowhere near the robot. See the source links and explanation in `ASSUMPTION_TRACEABILITY.md`.

### Why is the HDOP cutoff 5?

The assignment does not set a numeric cutoff. The starter rejects 6.5, and this implementation chooses 5.0 as the upper bound; the test suite checks that 5.0 passes. The value should be confirmed against the receiver and the application’s error tolerance.

### Why require four satellites?

The starter rejects three. Four measurements are commonly used to solve a standard GPS 3D position and receiver clock offset. Some 2D, aided, or constrained modes can work differently, so the receiver’s mode and definition of `satellites_used` matter.

### Why accept fix type 2?

The prompt defines 2 as “mixed” but does not say whether it is usable. The implementation accepts the two named nonzero types and rejects no-fix and unknown codes. That mapping must be checked against the receiver documentation.

### Why not require exactly 100 ms between frames?

The assignment says 10 Hz, so 100 ms is the nominal period. It does not specify an allowed timing window. The implementation allows jitter and only rejects a long gap. If cadence itself matters, add a separate, configurable min/max interval rule and test its boundaries.

### Why does the gap cutoff equal 1500 ms?

The supplied starter rejects a gap of 1600 ms (1000 to 2600). It does not establish that 1500 ms is the uniquely correct cutoff. This implementation chooses 1500 ms and tests both 1500 and a larger gap. It is a recovery policy to revisit for a real sensor.

### Why advance received time after a poor-quality frame?

Ordering belongs to the stream, while quality belongs to each measurement. If a bad fix at 1100 ms did not advance receive time, a good frame at 1200 ms would be compared with older stream state. The current policy lets it recover. Duplicate or older packets do not change state.

### Why reject the first frame after a long gap?

The code marks a gap over 1500 ms as a discontinuity and uses that frame as a new time baseline. It rejects that first frame, then a normally spaced next frame can pass. This avoids repeatedly comparing against a stale timestamp, but it also discards one possible good fix. Another system might accept the first fix as a new epoch or require several good frames.

### Why use a mutex?

State fields change together, so the per-instance mutex prevents data races. The output mutex keeps this class’s messages from interleaving across instances. The costs are locking and a non-copyable default class. Concurrent calls are handled in lock-acquisition order; the mutex does not reorder them by timestamp.

### Why print inside the processor?

The assignment requires valid positions to print. Formatting is done in a local stream so `std::cout` formatting elsewhere is unaffected. For an application, a return object or logger callback would separate processing from console I/O and make output easier to test.

### What does six decimal places mean?

It is a display format only. More printed digits do not make the receiver more accurate. Do not use the output precision as an estimate of position error.

### Why does the CSV parser check syntax before conversion?

It keeps the accepted file format predictable. For example, a longitude containing `lon=as` fails before numeric conversion. The conversion step then handles values too large for the destination type. The parser supports the documented six numeric columns; it is not a full quoted-CSV parser.

### Why do malformed rows not change processor state?

A malformed row never becomes a complete `GnssFrame`, so it is not passed to `process()`. A row with valid syntax but poor GNSS quality does reach `process()` and advances receive time if its timestamp is newer.

## Current limitations

| Area | Current behavior | Before robot deployment |
| --- | --- | --- |
| Rejection reason | Returns only `bool` | Return a reason code and diagnostic details |
| Limits | `Config` exposes HDOP, satellite, and gap limits; constructor does not validate them | Validate and configure for the receiver/application |
| Last accepted fix | Not retained | Expose fix data and age; enforce freshness downstream |
| Accuracy/integrity | Checks basic fields and HDOP | Use receiver integrity/uncertainty and motion plausibility |
| Output | Prints to `std::cout` | Inject a logger or return the accepted result |
| State reset | `reset()` clears timestamp history | Caller invokes it when a new stream session starts |
| CSV syntax | Six unquoted numeric columns | Add a standard CSV library if the schema grows |
| Concurrent producers | Race-safe state, lock-acquisition order | Serialize or reorder frames before processing |

## Test coverage

The supplied starter has 7 cases; the suite now has 29. The 22 added checks cover boundaries, invalid numeric values, timestamp ordering/gaps, recovery, `(0, 0)` acceptance/rejection configuration, valid zero-axis coordinates, custom limits, and reset. All 29 processor checks passed. The replay parser has been checked separately with malformed fields and CRLF input; it does not have a dedicated automated integration-test target.
