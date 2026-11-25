# XCurl original request engine — November 24, 2026

Source: exact local Store XCurl.dll, SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551. The original transport is WinHTTP, not libcurl or a separate TLS implementation.

16 disabled-ETW function bodies transcribed and tested locally, 160 cases each (2,560 total):
- B55C easy event and queue creation; A8F0 queue progress; CA60 first-failure preservation; B6B4 done predicate.
- 10AC4 request reset, A6B0 completion publication with nested recursive lock order, active/completed list movement and 40-byte message allocation; 16DE4 global counter decrement.
- 12124 read, 127A8 receive, 12D4C send (64-bit accounting, 32-bit WinHTTP lengths), 13038 callback/context registration; exact HRESULT conversion.
- 13C30 close, 1395C stop/cancellation transition, 13B14 final state, B90C notification, BA8C completion scheduling with synchronous fallback.

Test compares original and source callback sequence, return values, and entire normalized request/multi object bytes; original instruction execution occurs through Unicorn, with OS APIs and unported task queue / allocation / diagnostics modeled explicitly. Request reset connection-release (non-null COM pointer) branch is transcribed but not exercised by this fixture. The original unchecked completion-message allocation contract is retained and tested only for success. No ETW-enabled path, actual HTTP traffic, or complete callback dispatcher is claimed. All source joins t6::xcurl_research; no production DLL replacement is claimed.

Next: 13E14 WinHTTP status handler, 10748 connection/open-request, 131A8 option application, 18F68/19344 scheduler engine, task queue and actual Windows integration.

## WinHTTP notification dispatcher (13E14)

Reconstructed disabled-ETW status routing including send/write completion, header availability, data availability/read completion, redirect allocation, handle closure, partial-body and excess-length detection, and request-error/HRESULT mapping. Local differential: **3,840 passing cases**, 240 each across 16 status values (including inactive and unknown values). Full object bytes and callback order/arguments compared against original x64 execution; previously reconstructed cancel, close, finish, receive and notification routines execute directly. Unported sending, redirect-copy, upload, headers, publication and read-scheduling helpers are explicit modeled boundaries; ETW and real WinHTTP traffic are not tested. Combined with the 2,560 request-engine cases: 6,400 local comparisons.

## Core scheduler (0x18F68, 0x19344, 0x1925C, 0x19514, 0x19614)

1,280/1,280 local original-machine comparisons, 256 each: global admission reservation including wraparound, race with cancellation between reservation and admission, list movement, generation-based active progress, abort sweep, single-event wait and wakeup arbitration. Windows synchronization and actual request launch/progress remain explicit boundaries. Source joins the static target; this is not a completed network ABI.

## Request preparation (0xC6A0, 0xD174, 0x11780, 0xCF90)

1,024/1,024 local original-machine differential cases, 256 per function. Recovered transfer reset and tagged releases, URL/default/port preparation control flow, asynchronous task context construction and launch, cancellation/error behavior, and the top-level easy start chain. Original uninitialized aggregate padding is explicitly zeroed and excluded from equivalence; all other bytes and call traces compared. URL conversion/copy and task submission remain explicit boundaries; original start callback 0x11F10 is not yet translated. Next prioritize URL conversion 0x10EA4 and 0x94AC, then connection and request open, real task queue, header/body delivery and WinHTTP integration.

## HTTP setup (0x10748, 0x131A8, 0x137B8, 0x12FD8) and UTF-16 copy (0x94AC)

Actual local original-machine tests passed 1,536 setup cases and 256 copy cases. Original WinHTTP API order, method/port selection, session acquisition arguments, hostname temporary NUL and restoration, secure flag, streaming upload retry on 87, request options and timeout/error behavior verified. UTF-16 copy includes terminator, tag 7 allocation, failure output and original non-freeing ownership. The session acquisition 0x15F40, diagnostic and WinHTTP APIs are explicit boundaries; not end-to-end HTTP. Next: session cache 0x15F40 and complete request init 0x11478/0x11F10 plus header/body helpers.
## WinHTTP session cache and reference ownership

1,536 original-machine comparisons pass locally: 768 acquisition cases (0x15F40), 256 cache teardown (0x15B64), 256 release (0x1AB90), 256 creation (0x1A9D0). Entry construction/destruction, UTF-16 ownership, default user agent XCURL/2403.2.0.0.amd64fre, last matching cache node, TLS/URL scheme key, WinHTTP flags, error-87 retry, tagged heap failures and reference decrement/handle close are executed through the original chain. Test normalizes native pointers; OS operations and allocator callbacks are modeled. The vtable identity is explicit and the translated source directly calls reference functions, not original pointers. Preserves original secure-option failure branch without a session close; this is a known original ownership behavior, not production certification. Next: native WinHTTP binding and complete request initialization.

## Native Windows OS binding and real HTTP acceptance slice

Windows-only winhttp_services.cpp binds the recovered session-cache, open/options, request send/receive/read and callback registration helpers directly to WinHTTP and original-style native heap and critical sections. Native reference table is provided; no original binary function addresses are used at runtime. The independent loopback test exercises a cached session, real async GET and POST body echo, callback sequence and teardown, and audits the x64 PE import table. This intentionally tests transport leaves via a test callback driver; it does not replace the actual XCurl task queue, header/body publication, full status dispatcher, or public ABI. Windows CI is required before claiming this acceptance slice passes.

## Response-buffer scheduling and publication

Original 0xAFFC, 0xBCAC, 0x11FA8, 0xB800, 0xBDB0 and 0xC4A0: 1,536 local differential cases pass (256 each). Compare full object state and allocation, lock, WinHTTP read, queue submission, notification, cancellation and delivery traces. Preserves the reserved bit on allocation failure and the no-output case when both buffers are in use. The body publisher has an explicit owned-buffer precondition: invalid pointers in the original lead to out-of-object writes and are not emulated. The queue and application delivery callbacks remain boundaries; symbolic callback identifiers are not original code pointers. Real Windows loopback transport slice at dc1d60ad passed workflow 45432492988 (GET, POST, async read, session reuse, PE import audit). Not a complete public XCurl replacement.

Application delivery 0xAB3C: 512 local original-machine comparisons of chunk boundaries, debug notifications, callback arguments and results, wrapping byte accounting, callback abort sentinel, short/oversized writes and status mappings. Actual invocation helper 0xB9B4 remains a boundary. Nonzero configured chunk limit is an input precondition for progress.

## Transfer callbacks and header publication

Original 0xB9B4, 0xC360, 0xBBA8, 0xBF6C and 0xB4C0/0xB18C: 1,280 local original-machine cases pass (256 each). Execute the original guard-dispatch trampoline into modeled app callbacks and compare callback argument widths, absent callback return rules, queued header/upload notifications, header ownership and line boundaries, fallback to body callback, abort and short-write cancellation, release order and full normalized object memory. Header input must be a valid allocated NUL-terminated buffer with length including NUL; the native guard avoids invalid null/zero pointer arithmetic. Unported upload continuation and real task submission remain explicit boundaries. No full API completion claimed.

## Header query and getinfo metadata publication

Original 0x14D04, 0x1ABB0, 0x14F84 and 0xCD4C: **2,048 local original-machine comparisons**, 512 per function. Exact two-pass WinHTTP query, error mapping (including E_UNEXPECTED if a failed call reports success), allocation and release, UTF-16/UTF-8 conversion and metadata ownership transfer under original lock. A verified original quirk writes NUL at the UTF-16 unit count instead of the UTF-8 byte count; valid non-ASCII cases preserve that behavior, while invalid UTF-16 that would write out of bounds is rejected deliberately. The reported header byte count excludes the terminator, superseding the earlier header-worker comment; the worker retains its original explicit bounded scan. Win32 query and conversion services bound directly; no full request header orchestration or ABI completion claimed.

## Full response header orchestration

Original 0x12228 now directly invokes reconstructed 0x14D04/0x14F84/0x1ABB0/0xCD4C, with 1,024 local original-machine comparisons. Numeric status and length, raw header/version detection, optional content type, effective URL conversion, scheme publication, error cleanup and allocation ownership compared across full object and buffer memory. WinHTTP, CRT strncmp and 0x14AC0 URL cracking are explicit boundaries. Extended Windows smoke checks actual header query, getinfo fields, header callback bytes and ownership cleanup on GET and POST; the smoke uses a test-only WinHttpCrackUrl adapter (not the original insecure URL reporting path). New Windows CI pending.

## URL cracking checkpoint

Translated 0x14AC0, including the complete 0x68-byte x64 URL_COMPONENTS initialization, HRESULT conversion, HTTP warning/report ownership and unsupported scheme rejection. The formatter 0x1ADA4, platform reporting 0x54F4 and WinHTTP remain explicit service boundaries. All 1,024 original-machine cases passed locally after switching to GCC following the Clang execution timeout; CI differential run 45435399365 also passed. The native URL reporting services and formatter still require integration. The full response-header chain and Windows loopback acceptance at fc32dd67 passed CI (45434669548, 45434669794).

## Streamed upload continuation

Original 0x12868 and 0xC9C0, disabled ETW: 1,024 local original-machine comparisons (512 per entry). Active-state guard, tag-16 buffer allocation, real original read callback dispatch, fixed-length byte accounting, hexadecimal chunk framing and terminal chunk, EOF, custom-trailer delegation, callback abort/overflow, debug notification and WinHTTP write error cancellation. Comparison includes normalized object bytes and exact written buffer contents. The CRT formatter is modeled with the verified original `%zx%s` format; custom trailer generation 0x10420 and WinHTTP are explicit boundaries. Local GCC test passed; CI pending. No public ABI replacement claimed.

## Buffered upload allocation and callback loop

Original 0x102FC and 0xC070, ETW disabled: 1,024/1,024 local machine-code comparisons in the valid original domain. Compare tag-16 allocation/reallocation, callback invocation and exact writes, error/cancellation traces, lengths and ownership. The original resets capacity to block size per iteration and uses unsigned subtraction; beyond one block this can underflow and skip a required growth. Deliberate safety correction tracks real allocation capacity and rejects invalid oversized callback returns, instead of reproducing out-of-bounds writes. A separate real-host allocation regression verifies five full blocks plus EOF and capacity before every callback, exact data and cleanup. Streamed upload differential CI at cba4e859 passed (45436675918). Next integrate original 0x11978 request startup using these recovered helpers; trailer generation, task queue and native public ABI remain unclosed.

## Asynchronous request startup coordinator

Original 0x11978 and 0x11F10: 1,024 passing original-machine comparisons. Recovered state guard, metadata allocation/ownership and type-2 scan, URL/open/registration/options/header/certificate order, per-stage cancellation codes and close/finish selection, direct POST data, fixed-length callback and buffered callback upload modes. Startup now calls the reconstructed XAsyncGetStatus and XNetworking security-result-size/result wrappers instead of three generic result callbacks. The GDK QueryApiImpl resolver remains an explicit host boundary; the Windows GDK loader/initialization adapter is not yet complete. Oversized fixed read returns are rejected rather than permitting the original unsafe send length. The 1,024 startup cases pass CI at 45444003768.

## Outgoing header list

Original 0x10038, disabled ETW: 1,024 passing local original-machine comparisons. Reuses the recovered 0x1AC88 converter directly, preserves the original UTF-16 buffer reuse, case-sensitive Transfer-Encoding/chunked detection, POST data exception, debug bytes, WinHttpAddRequestHeaders flags 0xA0000000, ignored 12150 error and first-error break. Original header list is valid non-null string nodes; an invalid null node is not a supported input. Windows I/O and the allocation callbacks are modeled during the differential. Startup coordinator CI passed (45437375067). Next wire actual WinHTTP add/write and run streamed loopback.

## Native upload/write and outgoing header acceptance

The Windows adapter now binds the recovered outgoing header and streamed upload routines to WinHttpAddRequestHeaders and WinHttpWriteData. Expanded independent loopback smoke verifies the server observes the outgoing header on every request, a 24,579-byte fixed-length streamed POST uses multiple original upload callbacks/writes, the server echoes exact bytes, and all response bodies pass the recovered delivery callback/accounting path. The harness still drives the callback sequence and uses test-only URL cracking; this is an acceptance slice, not the finished task queue/public ABI. Windows CI pending.

## Certificate-store selector and lookup

Original 0x10CA0 and 0x11478: 2,048 passing local original-machine comparisons (1,024 each), including all eight store-location names, original prefix-match and thumbprint length rules, malformed selectors, store-open error conversion, hash decode, lookup and cleanup, original ignored WinHTTP return and diagnostic ordering. Uses immutable borrowed string spans rather than temporary mutation and MSVC SSO; host string allocation failure is not original-equivalent. Cryptography calls are modeled. The original option 47 call uses the address of the certificate pointer with size 40; the native adapter is deliberately not implemented until that ABI can be validated on Windows. The expanded native streamed POST, outgoing header and response-delivery loopback passed (45437723076); outgoing-header differential passed (45437608735).


## GDK interface wrappers

The four interface data objects at original RVAs 0x20110, 0x20120, 0x20130 and 0x20140 were read directly from the hash-pinned Store XCurl.dll. Code references in the original binary establish the provider/IID pairs and byte offsets in the COM-style vtables. The method names/signatures are cross-checked against the public IXThreadingImpl and IXNetworkingImpl layouts; the original RVAs and vtable offsets are independently observed in XCurl machine code.

- Error-report provider and IID: `8ca467f7-22e8-4096-8456-bb8aa13f79d8`.
- XThreading / XAsync provider and IID: `073b7dcb-1fcf-4030-94be-e3c9eb623428`.
- XNetworking provider: `37e56907-2f10-41e8-b72f-36edb185331a`.
- XNetworking interface IID: `bf2346b2-39af-4658-b5ea-44713c7e83b3`.

All offsets include the three IUnknown entries; each successful QueryApiImpl result is released through vtable slot 2. Methods that the original calls only after XThreadAssertNotTimeSensitive retain that ordering.

The new `gdk_interfaces.cpp` implements all 16 wrappers in the table in `xcurl-gdk-interface-map.md`. Local differential tests compare all 16 wrappers against the original machine code: 3,072 passing cases (192 per entry), including query GUID bytes, interface lifetime, vtable slot, argument widths/order, stack arguments, return values and output writes. The original request-start functions also pass 1,024 cases with the actual three GDK wrappers wired in.

Important boundary: `GdkQueryServices.query` represents QueryApiImpl after runtime initialization. This work does not yet claim to load or initialize a real XGameRuntime module. The original loader, initialization, query and shutdown paths are now reconstructed and pass 2,048 local differential comparisons; a native Windows adapter binds the loader and lock operations. No XGameRuntime runtime was loaded in these differential tests.

Evidence generator: `tools/dlls/probe_xcurl_gdk.py`. Public interface-layout cross-checks: [IXThreadingImpl](https://github.com/xodus-gaming/xgameruntime-docs/blob/main/COM/XThreadingImpl/IXThreadingImpl.md) and [IXNetworkingImpl](https://github.com/xodus-gaming/xgameruntime-docs/blob/main/COM/XNetworkingImpl/IXNetworkingImpl.md).

## Original engine lifecycle

The 0xA8-byte engine constructor (0x164D4), initializer (0x16C04), destructor (0x165D0), connectivity handler (0x166EC), limit setter (0x17044), suspend (0x170F4), resume (0x16EC4), app state notification (0x16B60), allocator (0x169FC), bootstrap (0x9BA0), and certificate verification (0x13D50) are reconstructed in engine_lifecycle.cpp. Local original-machine differential testing passed 2,816 cases (256 per function), including allocation/initialization failures, nested lock order, GDK interface acquisition/release, session-cache cleanup, and insertion of a new multi during the suspend wait. The bootstrap deliberately rejects a null engine on positive allocation error 27 rather than reproducing the original null dereference. Telemetry creation/destruction, OS handles, app registration and callback entrypoints remain explicit service boundaries; this is not the completed public XCurl ABI.

Request preparation (0x11780 and its caller 0xCF90) now calls the reconstructed GDK security-query wrapper (0x566C) directly, including interface acquisition and release; the 1,024-case preparation suite passes locally. Remaining URL conversion and callback entrypoints are explicit service boundaries.

WinHTTP status dispatch (0x13E14) now calls the reconstructed certificate verification path (0x13D50) directly on SENDING_REQUEST, including absent metadata, thread-sensitivity assertion, real GDK interface slot and certificate rejection propagation. All 3,840 status dispatcher comparisons passed locally with this boundary removed.
