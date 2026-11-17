# GNSS Processor Test Cases

The test program is [`../test_gnss_processor.cpp`](../test_gnss_processor.cpp). It contains 29 checks of `GnssProcessor::process()`, covering valid frames, rejection rules, timestamp recovery, configuration, and reset.

The `CHECK` macro prints `[PASS]` when an expected result is observed. It prints `[FAIL]` and exits on the first failed check. Accepted frames also print their position, so those output lines are expected during a successful test run.

## Basic validity and coordinate checks

1. **normal frames** — Accepts two consecutive frames with valid measurements and increasing timestamps.
2. **high HDOP rejected** — Rejects HDOP `6.5`, above the default maximum of `5.0`.
3. **no fix rejected** — Rejects `fix_type = 0`.
4. **latitude above range** — Rejects latitude `91`, outside `[-90, 90]`.
5. **latitude below range** — Rejects latitude `-91`, outside `[-90, 90]`.
6. **longitude above range** — Rejects longitude `181`, outside `[-180, 180]`.
7. **longitude below range** — Rejects longitude `-181`, outside `[-180, 180]`.
8. **too few satellites** — Rejects three satellites when the default minimum is four.

## Timestamp checks

9. **gap over limit rejected** — Accepts the first frame, then rejects a frame after a `1601 ms` gap when the default maximum is `1500 ms`.
10. **exact gap limit accepted** — Accepts frames exactly `1500 ms` apart.
11. **duplicate timestamp rejected** — Rejects a second frame with the same timestamp as the previous frame.
12. **out of order rejected** — Rejects a frame whose timestamp is older than the previous frame.
13. **first frame can have zero timestamp** — Accepts a valid first frame with timestamp `0`.
14. **long-gap baseline recovery** — Rejects the first frame after an excessive gap, then accepts the next regularly timed frame.

## Invalid numeric values and quality rules

15. **NaN latitude rejected** — Rejects a latitude with a NaN value.
16. **infinite longitude rejected** — Rejects an infinite longitude.
17. **infinite HDOP rejected** — Rejects infinite HDOP.
18. **negative HDOP rejected** — Rejects HDOP below zero.
19. **unsupported fix rejected** — Rejects the unknown fix code `3`.

## Boundaries and recovery

20. **inclusive coordinate and HDOP boundaries** — Accepts coordinates at the geographic extremes and HDOP values `0` and `5.0`.
21. **invalid quality advances received timestamp** — A poor-quality frame advances the received-time baseline, allowing the next valid frame to pass.
22. **quality rejection does not poison recovery** — Rejects an invalid-latitude frame, then accepts a later valid frame.

## `(0, 0)` policy and configuration

23. **Null Island placeholder rejected** — Rejects exact `(0, 0)` with the default configuration.
24. **Null Island accepted when configured** — Accepts `(0, 0)` when `reject_null_island` is `false`, for example for a marine robot.
25. **equator and prime meridian coordinates remain valid** — Accepts coordinates where only latitude or only longitude is zero.

## Custom limits and reset

26. **custom HDOP limit** — With a configured maximum of `3.0`, rejects HDOP `4.0`.
27. **custom minimum satellite count** — With a configured minimum of six, rejects a frame using five satellites.
28. **custom maximum timestamp gap** — With a configured maximum gap of `500 ms`, rejects a `601 ms` gap.
29. **reset starts a fresh timestamp session** — After `reset()`, accepts a timestamp lower than the timestamp received before reset.
