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
