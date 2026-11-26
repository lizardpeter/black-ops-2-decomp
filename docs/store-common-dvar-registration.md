# Store MP common dvar registration

Replay-only candidate for the complete 14039E2B0..1403A002B body in `src/qcommon/common_register_dvars_store.cpp`. Source identity is provisional. Evidence: `analysis/t6mp_msstore/pseudocode/_unattributed/14030000.c` and matching x64 disassembly.

- 39 inline find/new-or-reregister sites, 35 bool wrapper sites, 7 float wrapper sites, 17 string wrapper sites (one iterates five times), and two enum sites (one conditional).
- 103 registration operations on the ordinary path; 104 when dedicated evaluates nonzero. 75 distinct result globals including the five war slots.
- Preserves the dedicated re-registration branch, debug_show_viewpos set with value 1/source 0, two subsidiary calls, dynamic g_loadScripts default, and ordered assignments.
- com_maxfps default 0, range 0..1000, flags 1; result pointer at 141F03820.
- Inline temporary padding normalized, as for the client-game candidate. No claim of raw padding parity.
- Enum contents remain an external original table boundary. The real core and setters are not linked into this isolated replay. No enhanced changes or status promotion.

Validation harness: `python tests/verify_store_common_registration.py originals/t6mp_msstore.exe` compares 256 original executions to the candidate, with both new/existing lookup paths, ten dedicated current-value cases, two script-load defaults, 145/146 ordered calls and 75 globals. The test masks unused value lanes and compares the enum table pointer, not its contents. Executed successfully in GitHub Actions run 45883561059 at commit b0b133758dc4ba1a3d37ea0134dec217fc935bfa: 256 complete common registration replays; 145/146 ordered events and 75 result slots per case. This is full enclosing-body differential replay with mocked callee boundaries, not a live game test. The full candidate is derived from the checked-in Store MP evidence; ZM/SP independent verification remains open.
