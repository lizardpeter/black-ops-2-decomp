# Microsoft Store BO2: true external-DLL reconstruction

## Goal and constraints

Reconstruct the **actual code** and reachable algorithms of the original Microsoft Store x64 `XCurl.dll` and `binkw64.dll`, not compatibility wrappers around FFmpeg, libcurl, or other implementations. Preserve relevant ABI, state, error behavior, threading, and multimedia semantics. Differential-test the reconstructed source against the original binaries before replacing either dependency.

This is **not done yet**. The shipped Store EXEs still import both DLLs at process startup. Reconstructing selected functions does not remove those PE imports; the enhanced loader must eventually redirect/remove the imports, or the fully rebuilt EXE must link replacements statically.

## Original files (never commit them)

| R2 key | Size | SHA-256 |
|---|---:|---|
| `bo2/DUMP/XCurl.dll` | 198,248 B | `65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551` |
| `bo2/DUMP/binkw64.dll` | 310,784 B | `10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17` |

Tools:

```sh
python tools/dlls/fetch.py
python tools/dlls/inventory.py originals/dlls/XCurl.dll originals/dlls/binkw64.dll --json originals/dlls/inventory.json
# Optional: add --clients originals/t6mp_msstore.exe originals/t6zm_msstore.exe originals/t6sp_msstore.exe
```

Fetch verifies SHA-256 and does **not execute** any file. Inventory uses the repository's `pefile` dependency and scans import names, exports and .pdata as data. Original binaries and generated inventories remain under ignored `originals/`.

## Initial measured scope

| | XCurl | Bink |
|---|---:|---:|
| Functions imported by each Store EXE | 16 | 19 |
| Named exported entry points | 61 | 77 |
| x64 unwind entries | 431 | 676 |

Unwind entries are **not** a complete unique-function count: inlined routines, leaf functions, thunks, code sections without unwind records, and chained unwind regions require analysis.

The 16 imports: `curl_global_init`, `curl_global_cleanup`, `curl_easy_init`, `curl_easy_cleanup`, `curl_easy_setopt`, `curl_easy_getinfo`, `curl_easy_strerror`, `curl_slist_append`, `curl_slist_free_all`, `curl_multi_init`, `curl_multi_cleanup`, `curl_multi_add_handle`, `curl_multi_remove_handle`, `curl_multi_perform`, `curl_multi_poll`, `curl_multi_info_read`.

The 19 imports: `BinkOpen`, `BinkClose`, `BinkDoFrame`, `BinkDoFrameAsync`, `BinkDoFrameAsyncWait`, `BinkNextFrame`, `BinkPause`, `BinkGetRects`, `BinkGetFrameBuffersInfo`, `BinkRegisterFrameBuffers`, `BinkGetError`, `BinkGetRealtime`, `BinkSetSoundTrack`, `BinkSetMemory`, `BinkSetIOSize`, `BinkControlBackgroundIO`, `BinkStartAsyncThread`, `BinkRequestStopAsyncThread`, `BinkWaitStopAsyncThread`.

## First transcribed source bodies

`reconstructed_dlls/bink/bink_small.cpp` contains four independently decoded **export function bodies** from the original machine code. These are *not yet differential-tested or integrated*.

| Function | Original RVA | Behavior observed in original bytes |
|---|---|---|
| `BinkSetMemory` | `0x1DA30` | Store allocator and deallocator pointers at RVAs `0x542A8` and `0x542B0` |
| `BinkSetIOSize` | `0x16840` | Store unsigned 32-bit argument at global RVA `0x570E0` |
| `BinkSetSoundTrack` | `0x16870` | Clamp unsigned count to 32, store it at `0x570B0`, copy that many 32-bit IDs to `0x57250` |
| `BinkRegisterFrameBuffers` | `0x17950` | If handle and buffer pointer are non-null, store the latter at handle offset `0xD0` |

Other initial observations: `BinkGetError` at `0x14C40` returns a pointer to a global at `0x31860` (bounds/writers still unknown). `BinkDoFrame` at `0x1B0C0` is a small dispatch stub, **not** the underlying frame decoder.

The original allocators and full Bink handle type are unresolved; the standalone source does not yet reproduce original process initialization.

## Steps to completion

1. Import these exact hashes into Ghidra and export decompilation/disassembly/function tables, finding leaves in addition to .pdata-covered functions.
2. Trace the full transitive call graphs from the 35 imported entry points, following indirect calls and noting unresolved edges.
3. Recover all state and types. For Bink, this explicitly includes the **Bink container, video entropy/transform/decode pipeline, audio codec, frame buffers, timing, background worker threads, and error handling**. No third-party decoder or stub returns.
4. Reconstruct every reachable function in readable source and differential-test semantics against the original DLL on Windows. Implement full import-compatible exports only when tested.
5. Test complete playback of the **44 `.bik` assets** already in R2, comparing decoded frames, audio, timing, seeks, and failure behavior. The other 81 videos being `.webm` does not eliminate Bink decoding.
6. Reconstruct XCurl's HTTP/multi-handle semantics, callbacks, progress, errors, and TLS interactions. Separately determine which calls to decommissioned vendor services should be *intentionally removed from engine callers*, without confusing removal with faithful decompilation.
7. Only once the transitive set is fully matched, redirect/remove the Store executable's two DLL dependencies and verify cold startup, videos, menu flow, and all three game modes.

**Current status:** 13 Bink export bodies and two XCurl export caller bodies transcribed. The XCurl callers still need their original internal allocation callees. No end-to-end decoder, HTTP implementation, differential DLL harness, or DLL-free build exists.

## Reconstruction pass, 2026-11-23

These functions were decoded by disassembling bytes extracted from the SHA-256-pinned original DLLs; they are not copied from third-party library implementations.

### Additional Bink leaf bodies (`reconstructed_dlls/bink/bink_leaf.cpp`)

| Function | RVA | Machine-code behavior |
|---|---|---|
| `BinkSetError` | `14C00` | Copy up to 254 characters and NUL-terminate buffer at `31860` |
| `BinkGetError` | `14C40` | Return pointer to buffer at `31860` |
| `BinkSetFrameRate` | `16820` | Save two uint32 words at `570E4` and `311C0` |
| `BinkSetFileOffset` | `16830` | Save uint64 at `57128` |
| `BinkSetSimulate` | `16860` | Save uint32 at `570DC` |
| `BinkSetWillLoop` | `1DA40` | Toggle bit `0x80` of uint32 at handle+`0x20` |
| `BinkSetVideoOnOff` | `1D820` | Set handle+`0x134`, return input state |
| `BinkGetTrackID` | `1D320` | Read indexed uint32 from pointer at handle+`0x1D0` |
| `BinkGetTrackType` | `1D2E0` | Read indexed uint32 from pointer at handle+`0x1C8` |

Together with the first four setter bodies, **13 Bink exports** are transcribed. Five are BO2 imports. All 13 need differential verification; original global initialization and downstream consumers remain unresolved. Standalone sanitizer-enabled tests passed locally.

### XCurl caller transcriptions (`reconstructed_dlls/xcurl/slist.cpp`)

- `curl_slist_append` at `1BB80`: duplicates a string through an original function pointer at RVA `28020`, allocates a 16-byte node through internal function `932C` with tag 5, and appends.
- `curl_slist_free_all` at `1BC00`: walks nodes, frees the string if present, clears the pointer, then frees the node using internal function `9640` with tag 5.

**Important:** these two callers are *partial* source reconstructions because the original allocator functions and indirect target are not reconstructed. The isolated control-flow tests use test-double allocators only; they are not substitutes in the production implementation. Networking/TLS and every Bink decode routine remain unreconstructed.

### Analysis automation and tests

- [Manual Ghidra workflow](../.github/workflows/dll-ghidra.yml) exports full-DLL function inventories, disassembly, call graphs, and pseudocode for both SHA-256-pinned originals as CI artifacts. The workflow has been added, **not executed/verified**.
- [Standard CI](../.github/workflows/check.yml) now compiles and runs the Bink leaf and XCurl list tests using Clang with sanitizers.
- No original binaries are committed, and neither original EXE import table nor the enhanced loader was changed.

**Next:** run the Ghidra workflow, trace both `curl_slist` allocator callees and the call trees below `BinkOpen`/`BinkDoFrame`, then implement actual internal algorithms and differential test against pinned originals. No runtime replacement should be shipped until dependencies are fully reconstructed and matched.

## Full Ghidra artifacts and decoder-path triage (2026-11-23)

The [original-DLL Ghidra workflow](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45226208577) **succeeded for both pinned DLLs**. Its two artifacts contain disassembly, pseudocode, function tables and call graphs. Ghidra identified 515 Bink functions (486 non-thunk decompilation records) and 525 XCurl functions (501 decompilation records). These are *analyzed functions*, not reconstructed C++ matches.

The new `tools/dlls/reachability.py` reads the original artifacts' `functions.tsv` and `callgraph.tsv`. The measured static reachability union of **all game-imported entry points** is 189/515 Bink functions and 204/525 XCurl functions. These counts are *not upper bounds*: indirect calls, callbacks, dynamic import resolution, and missing leaf edges can increase the actual implementation scope.

Key direct edges from the real decoder:

| Source | Original direct callees | Static transitive size |
|---|---|---:|
| `BinkOpen` (`0x19B80`) | `FUN_180017970` (initializer, 8,714-byte body) | 98 |
| `BinkDoFrame` (`0x1B0C0`) | tail-jump to `BinkDoFramePlane` at `0x1A930` | 71 |
| `BinkDoFramePlane` | `FUN_1800162D0` (I/O); `FUN_18001A260` (frame state); `FUN_18001A850` (audio handling); `FUN_1800274A0` (video decompression); `FUN_18001A380`; `RADTimerRead` | 70 |
| `BinkDoFrameAsync` | `FUN_180021F70` (worker dispatch), `FUN_18001A260`, `FUN_180001010`, `BinkSetError`, `RADTimerRead` | 18 |
| `curl_easy_init` | `FUN_18000DC94` and trace/allocator wrappers | 92 |
| `curl_multi_perform` | `FUN_180019B8C` and timing/lock/tracing wrappers | 134 |

**Prioritized decoder targets:** `FUN_180017970` for Bink header/open state, then `FUN_1800274A0` and `FUN_1800252C0` for core video decoding, and `FUN_18001A850` for audio. The presence of names and pseudocode does not imply a matched decoder.

### Additional reconstructed source, tests and boundaries

- `reconstructed_dlls/bink/bink_frames.cpp`: full instruction-derived body for BO2-imported `BinkGetFrameBuffersInfo` at RVA `17870`, including aligned YUV dimensions, descriptor flags, pointer clearing, and multi-buffer metadata copy. The matching standalone C++ test, extended original-Unicorn oracle, and a new `tests/dlls/differential_frames.py` compare byte-for-byte source/original outputs on deterministic edge and random cases. Results must be checked in the latest CI before declaring a differential match.
- `reconstructed_dlls/bink/bink_dispatch.cpp`: `BinkOpen`'s original one-shot global option snapshot/reset and `BinkDoFrame`'s tail dispatch constant `0x303`. Both accurately expose **unresolved original decoder callees**, `FUN_180017970` and `BinkDoFramePlane`; they are not working video functions. The 4-byte uninitialized original stack gap in the BinkOpen options is explicitly documented.
- `reconstructed_dlls/xcurl/duplicate_string.cpp`: internal `FUN_180017450` scans NUL, allocates exactly length+1 with allocation tag 5, then copies the string. The original `0x28020` indirect duplication slot points to this function. Internal allocator `FUN_18000932C` and copy `FUN_180004AD0` remain unresolved; tests use declared test fixtures only.

**Do not remove** either import yet: `XCurl.dll` request engine and Bink's audio/video decoding algorithms still require substantial reconstruction. The goal remains original machine-code-derived behavior, without FFmpeg/libcurl substitutes or silent no-op stubs.
