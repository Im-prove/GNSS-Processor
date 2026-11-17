# AI Assistance Disclosure

This file records how AI assistance was used on the GNSS Processor take-home. The candidate supplied the assignment and original seven-test starter, used AI throughout the work, and reviewed the resulting changes. AI helped generate and revise code, challenge the design, explore alternatives and failure paths, develop tests, and prepare supporting documents. The candidate is responsible for the final submission and for understanding and explaining its behavior.

## Work assisted by AI

- Generated and revised implementation code, including validation rules, timestamp handling, configurable limits, the optional `(0, 0)` rejection policy, reset behavior, and CSV parsing helpers. The candidate reviewed and adjusted the resulting code.
- Acted as a devil's advocate for both the candidate's choices and the implementation. This included challenging assumptions, comparing design options, and considering how legitimate input or operating conditions could be rejected.
- Explored failure paths and potential bug/security areas in numeric parsing, non-finite values, malformed CSV rows, timestamp order and gaps, state recovery, concurrent calls/output, and configuration choices. This was a focused code review for this exercise, not a formal security audit.
- Helped expand the original seven processor checks to 29 cases. The additional cases cover boundaries, malformed numeric values, timestamp behavior and recovery, configurable limits, both settings of the `(0, 0)` policy, and reset.
- Helped manually examine CSV replay behavior with malformed values and CRLF line endings. The replay parser does not have a separate automated integration-test target.
- Generated and edited project materials, including the README, candidate response, pseudocode and beginner walkthroughs, interview notes, assumption traceability, submission checklist, AI disclosure, and one-slide overview.
- Researched coordinate conventions and the four-measurement assumption for standard GPS 3D fixes; relevant references are recorded in the assumption traceability document.

## Verification performed

The 29-case suite passed with MSYS2 GCC using C++17, `-pthread`, `-Wall`, `-Wextra`, and `-Wpedantic`. Replaying the supplied CSV processed 500 rows: 459 accepted and 41 rejected. A CRLF input check confirmed malformed fields were reported and replay continued. These results verify the described checks, but do not establish that the design is production-ready or free of defects.
