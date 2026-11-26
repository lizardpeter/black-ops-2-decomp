# Store MP dvar registration core

Authored replay-only source: `src/universal/dvar_registration_store.cpp`.
This continues the existing UI and client-game registration bodies. It is not a replacement registry and does not modify the enhanced runtime. Enable `T6_STORE_DVAR_CORE_REPLAY` only in isolated tests.

| Original MP entry | Candidate |
| --- | --- |
| 14046B830 | FindByHash |
| 14046B8B0 | FindByName |
| 14046C1A0 | RegisterNew |
| 14046C4C0 | RegisterBool |
| 14046C580 | RegisterInt |
| 14046C650 | RegisterFloat |
| 14046C760 | RegisterVec2 |
| 14046C840 | RegisterVec3 |
| 14046C930 | RegisterVec4 |
| 14046CA30 | RegisterString |
| 14046CAF0 | RegisterEnum |
| 14046CBD0 | RegisterColor |

Evidence: `analysis/t6mp_msstore/pseudocode/_unattributed/14040000.c` and matching disassembly. The original uses 4096 records of 0x68 bytes, 1024 hash buckets, full-hash equality without a string comparison, and a reader/writer counter protocol with Sleep(0). The disassembly corrects the incomplete Ghidra writer-acquisition model: atomic XADD, check for sole writer, recheck readers, decrement on retry. New registration stores current, latched and reset, interns external names and string values, invalidates the sorted list, and links the new entry into its hash bucket. Vector types preserve unused value lanes; vec3/vec4 also copy trailing components into the domain temporary as observed.

The nine typed wrappers all perform lookup, then either new allocation or re-registration and return the selected pointer. Description arguments are absent from the Store vector/string/enum/color ABIs; they supply the original empty string. Padding/uninitialized temporary lanes are normalized to zero in the source and are not yet proven byte-equivalent. Color constant 0.501f is taken from pseudocode and must be confirmed from original image data. The original capacity-error path releases the writer before calling the engine error routine; the candidate models it as nonreturning, pending boundary verification.

Remaining: complete 14046BF10 re-registration and its type/string conversion helpers, native string-pool binding, differential replay against the pinned MP PE, then independently recover ZM and SP counterparts. No records status promotion or FOV patch is made.

Source-body test (no PE required):

    g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/store_dvar_core_test.cpp -o /tmp/store_dvar_core
    /tmp/store_dvar_core

The test checks all nine typed wrappers, new/existing paths, value and domain fields, string storage, pointer publication and hash lookup. It does not validate real original execution or thread contention.
