# Assignment Checklist

| Assignment item | Evidence in this repository | Result |
| --- | --- | --- |
| Implement `GnssProcessor::process(const GnssFrame&)` | `gnss_processor.h/.cpp` | Implemented |
| Print valid positions | `process()` prints accepted frames; rejected frames are silent | Implemented |
| Handle expected 10 Hz input | 100 ms is treated as nominal; gaps above 1500 ms are rejected | Implemented with documented policy |
| Pass starter tests and add at least four cases | `test_gnss_processor.cpp` | All 29 checks passed; 22 extend the supplied seven |
| State assumptions and validation rules | `CANDIDATE_RESPONSE.md`, `README.md`, `docs/ASSUMPTION_TRACEABILITY.md` | Documented |
| Explain behavior after rejection | Candidate response and beginner walkthrough | Documented |
| Discuss changes needed for a robot | Candidate response and interview notes | Documented |
| Replay `gnss.csv` (extra credit) | `replay_gnss.cpp` | 500 rows: 459 accepted, 41 rejected |
| Provide a design slide | `docs/GNSS_Processor_Overview.pptx` | One-slide summary created and layout checked |
| Describe AI assistance | `RESPONSE.md` | Disclosure included |
| Provide a GitHub link, if requested | No Git remote or URL is configured in this workspace | Candidate must supply one only if the portal requires it |
| Build on Windows, Linux, and macOS | C++17 standard-library code and platform build commands in README | Commands documented |

## Remaining limits

- The candidate response filename is generic because the candidate’s name was not provided. Rename it to `<name>_RESPONSE.md` if the submission system requires that exact pattern.
- The CSV reader accepts six unquoted numeric columns; it is not a general quoted-CSV parser.
- Malformed-row replay was checked manually with CRLF input. There is no separate automated integration-test target for the replay executable.
- `GnssProcessor` does not expose last-fix age or structured rejection reasons. Thresholds and freshness rules need receiver- and robot-specific review.
