# Store checksum reconstruction

The primary target is t6mp_msstore (x64), not the x86 dedicated server.
The original PC server source unit is named by its PDB and exported in
analysis/CoDMPServer_PC/pseudocode/live/live_anticheat.cpp.c. The Store MP
export in analysis/t6mp_msstore/pseudocode/_unattributed/14050000.c
contains the two CRC recurrence loops within the hash continuation
body. This is consistent with LTCG inlining; do not assign the helper
functions fictitious standalone Store entry points.

Authored buffer arithmetic is in src/live/live_anticheat.cpp. The
generated IEEE reflected CRC table follows the reference implementation;
the Store table bytes still need independent binary comparison. The
source does not replace any live integrity or challenge service and is
not registered as a Store runtime override.

Standalone arithmetic test:
```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude src/live/live_anticheat.cpp tests/live_checksum_test.cpp -o /tmp/t6checksum-test
/tmp/t6checksum-test
```

1,542 cases (two alignments, lengths 0–256, three seeds), plus a
standard CRC test vector, passed on Linux x86-64. This is not a
Windows build, an original-binary differential pass, or a claim of
Store SP/ZM parity. Hash lifecycle and service integration remain
unreconstructed. No generated analysis or runtime hooks changed.
