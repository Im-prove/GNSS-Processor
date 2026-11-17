# Candidate Response — GNSS Processor

## 1. What assumptions did you make?

The assignment defines the fields and expected 10 Hz rate, but it does not define all quality thresholds or recovery behavior. I made these choices:

- The timestamp is milliseconds from one monotonic source clock. The first frame may have timestamp zero; later timestamps must increase.
- At 10 Hz, 100 ms is the nominal interval. I allow ordinary jitter. The original starter rejects a 1600 ms gap; I chose 1500 ms as the exact cutoff and added tests for the boundary.
- Latitude `[-90, 90]` and longitude `[-180, 180]` are general geographic-coordinate limits. They reject values outside the latitude/longitude domain; they do not measure GNSS accuracy or route plausibility.
- By default, the exact `(0, 0)` coordinate is rejected as a presumed uninitialized/default coordinate. This project assumes a land-based mobile robot: `(0, 0)` is in the ocean, outside its expected operating area, and an all-zero coordinate is a common pre-fix/default value. It is still a real geographic location ([map](https://www.openstreetmap.org/?mlat=0&mlon=0#map=5/0/0)), so this is an application heuristic, not a GNSS rule. The `reject_null_island` setting in `Config` defaults to `true`; marine or other applications that may legitimately operate there can set it to `false`.
- HDOP must be between 0 and 5. The original test rejects 6.5 but does not set an exact threshold; 5.0 is my chosen cutoff.
- At least four satellites are required, following the starter test and the usual assumption for a standard 3D GPS solution. A receiver operating in 2D or using aiding may have different requirements.
- HDOP, minimum satellite count, and maximum packet gap have per-processor `Config` values with the same defaults as the original fixed limits. The caller can tune them for its receiver/application. `reset()` clears stream timestamp history when starting a new session.
- Fix types 1 and 2 are accepted because the prompt defines them as standard and mixed fixes. Treating mixed as usable is an assumption; the receiver’s documentation should decide this for a real system.
- I did not add a position-jump limit because the assignment provides no robot speed or motion limits.

The sources and limits for these decisions are listed in [ASSUMPTION_TRACEABILITY.md](docs/ASSUMPTION_TRACEABILITY.md).

## 2. Which validation rules did you implement?

A frame is accepted only if:

- Its timestamp is newer than the last received timestamp and no more than the configured gap (1500 ms by default).
- Latitude and longitude are finite and within the geographic limits above.
- Unless `config.reject_null_island` is false, the coordinate is not the exact `(0, 0)` pair. The setting defaults to true for the assumed land robot; marine applications can disable it.
- HDOP is finite and in `[0, config.max_hdop]` (default maximum 5).
- At least `config.min_satellites` were used (default 4).
- Fix type is 1 or 2.

The CSV replay tool also checks the header, column count, numeric field syntax, and conversion range. A malformed row such as one with `lon=as` is reported and skipped; replay continues with the next row.

## 3. How should the processor behave after a rejected frame, and why?

The processor tracks the most recently received timestamp. A newer frame advances it even if the frame is rejected for quality, so the next normally spaced frame can recover without comparison to an old stream timestamp. Duplicate and older frames change no state. The processor does not store a last accepted position or timestamp.

A frame after a gap greater than 1500 ms is rejected and becomes the new timing baseline. The following normally spaced frame can then be accepted. This intentionally discards the first frame after a long outage; it is a simple recovery policy, not a universal GNSS rule.

Only accepted frames print a position. The processor does not retain the last accepted frame; rejected positions are not printed so they cannot be mistaken for usable fixes.

## 4. What would you change before using this on a real robot?

I would return a rejection reason instead of only `bool`, expose the last accepted fix and its age, replace console output with a logger or callback, and define how callers detect receiver clock changes. A motion-plausibility check could be added once the robot's speed/dynamics and receiver error model are known; this implementation intentionally has no kinematic speed check. The navigation system should stop using a fix after its freshness limit. HDOP and satellite count alone do not establish position accuracy.

## Verification

All 29 checks passed with MSYS2 GCC, C++17, `-pthread`, `-Wall`, `-Wextra`, and `-Wpedantic`. The new configuration test confirms `(0, 0)` is accepted when `reject_null_island` is false; the default rejection test also passes. Replaying `gnss.csv` processed 500 rows: 459 accepted and 41 rejected. A CRLF replay check rejected malformed values such as `lon=as` and a text satellite count, then continued with later rows.

See the [pseudocode overview](docs/CODE_WALKTHROUGH.md), [detailed code walkthrough](docs/BEGINNER_CODE_WALKTHROUGH.md), [interview guide](docs/INTERVIEW_PREP.md), and [one-slide overview](docs/GNSS_Processor_Overview.pptx) for more detail.
## Repository link

[GNSS-Processor repository](https://github.com/Im-prove/GNSS-Processor). The README includes the clone command for Command Prompt, PowerShell, and Linux. The command could not be run here: Git is not installed/on `PATH` in the Windows environment, and WSL/Linux is not installed. The command syntax is the standard `git clone <URL>` form; repository access itself remains unverified from this workspace.
