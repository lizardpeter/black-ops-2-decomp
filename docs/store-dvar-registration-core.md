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

## Re-registration and shutdown continuation

Six additional Store MP source-body candidates, compared with the checked-in x64 disassembly (14040000.asm):

| Address | Candidate |
| --- | --- |
| 14046A440 | CopyCurrentString |
| 14046A520 | CopyResetString |
| 14046B990 | Shutdown |
| 14046BAB0 | SetReset |
| 14046BBD0 | Reinterpret |
| 14046BF10 | Store_DvarReregister |

Ordinary re-registration merges flags and updates a non-null description, but does not replace the existing domain or reset. External-to-internal registration releases the owned name and reinterprets the value; an external type change also reinterprets. The original cheat-disabled and latched paths invoke setters in a specific order. This is preserved in the replay candidate without altering the original flags or enabling anything in the enhanced runtime.

Shutdown disables callbacks, clears owned string values with alias-aware release ordering, clears the count, cheats pointer, modified flags and hash buckets under the writer lock. It intentionally does not clear the sorted flag, pointer list or complete pool.

The original helper at 14046BBD0 writes the new type before checking it for release behavior; the source keeps this surprising order rather than applying an assumed correction. Its RDX argument is not consumed. The helper still depends on original formatter, parser, domain/constrain, setter and string-pool boundaries; those are not reconstructed or verified here.

Remaining: native string-pool binding, formatter/parse/constrain and setter bodies, callback registration and initialization, original-PE differential replay for the core, and independent ZM/SP recovery. No records status promotion or FOV patch is made.

The common engine routine at 14039E2B0 has an ordinary integer registration of com_maxfps with default 0, range 0–1000, flags 1, and result global 141F03820 (pseudocode shard 14030000.c). This is an original registration, not a proposal to remove the runtime frame limiter. The 14070E230 decompiler output still consists of a timeout marker; reconstruct from assembly rather than treating this as a recovered body.

Source-body test (no PE required):

    g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/store_dvar_core_test.cpp -o /tmp/store_dvar_core
    /tmp/store_dvar_core

The test exercises all nine typed wrappers, ordinary re-registration, flag/setter ordering with mocks, reset-string sharing and release, external string-to-integer conversion, shutdown and restart. It does not validate original execution, original helper semantics, or thread contention.
