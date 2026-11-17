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
| Provide a GitHub link | [GNSS-Processor repository](https://github.com/Im-prove/GNSS-Processor) | Link supplied; clone command documented in README, but not verified from this environment |
| Build on Windows, Linux, and macOS | C++17 standard-library code and platform build commands in README | Commands documented |

