# Store checksum reconstruction

The primary target is t6mp_msstore (x64), not the x86 dedicated server.
The original PC server source unit is named by its PDB and exported in
analysis/CoDMPServer_PC/pseudocode/live/live_anticheat.cpp.c. The Store MP
pseudocode and disassembly contain both CRC recurrence loops inlined
into the hash continuation body, and show hash initialization and
finalization logic. Do not assign fictitious standalone Store entries
to inlined functions.

## Authored code and scope

src/live/live_anticheat.cpp contains the CRC arithmetic and a bounded,
caller-owned buffer model of the CRC lifecycle. The record layout
matches the observed 32-byte Store MP context accesses and has
compile-time size and offset checks. Allocation, incremental processing,
completion and slot reuse are represented. The model uses buffer offsets,
rejects unsupported modes and invalid ranges, and rejects premature
completion. These are deliberate offline API constraints, not claims
about the original ABI or all its edge cases. No remote challenge
processing or runtime override is implemented.

## Verification

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude src/live/live_anticheat.cpp tests/live_checksum_test.cpp -o /tmp/t6checksum-test
/tmp/t6checksum-test
```

Executed in Linux x86-64: 1,542 arithmetic cases and 216 lifecycle
cases passed, plus pool exhaustion/reuse, invalid bounds and standard
CRC test vector checks. A separate isolated replay of the 237-byte
Store MP continuation body extracted from the repository's Ghidra
instruction listing passed 216 differential cases (360 original
calls) over owned input buffers. The replay supplied an independently
generated reflected CRC32 table; the original table bytes were not
available. Thus this is evidence for valid-input continuation behavior,
not complete binary parity, not a Windows build, and not validation
of startup, finalization or network/service logic.

SP and ZM have PE/unwind inventories only in the current repository,
without comparable full disassembly. No shared-code parity claim is
made. Fetching the Store executable from the configured R2 host failed
with a DNS error in the execution environment.

No generated analysis, runtime hooks, or function-status claims
were changed.
