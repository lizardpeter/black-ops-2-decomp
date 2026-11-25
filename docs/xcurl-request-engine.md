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


## Source-link API composition

The explicit ApiBinding connects the original global init/cleanup wrappers directly to the recovered engine bootstrap/destructor, and the public setopt wrapper to the owned option dispatcher (including wide-string conversion). Getinfo shares the same profiling and lock services. Host GDK, locks, telemetry, allocations and runtime shutdown remain injectable, with separate contexts for the API and engine. Calls to global init/cleanup must be serialized by the owner and all handles drained before shutdown. Deliberate integration-only safety deviations: skip null-engine destruction after initialization failure, and report initialization failure on repeated init while no engine exists, while retaining the original reference accounting. The original differential entrypoints are unchanged. Added composed source-link success, partial-initialization failure, allocation failure, runtime failure, retry, option ownership and active-request tests. No live Gaming Services or retail ABI completion claimed.


## Multi scheduler source-link composition

TransferBinding connects the original multi queue operations to the recovered admission scheduler, request preparation, queue progress, cancellation status, completion predicate and engine counter decrement. The engine-owned queue is read at admission; host wait/reset/mutex services remain explicit, as does original 0xCEFC after-cancel. EngineServices can reference the same scheduler for suspend/resume. The original request-completion callback and task queue still require host binding. Added a composed test with two admitted requests, a third removed before admission, a one-request engine limit, modeled asynchronous security-query failure, real completion publication, message ownership, poll/wait/wakeup and teardown. This is a source-link integration test, not an original-machine comparison or live HTTPS acceptance. Easy/multi allocation lifecycle and public ABI are not completed.


## GDK task queue source-link adapter

TaskQueueBinding uses the recovered GDK wrappers for the original manual easy queue modes (0,0), completion-port dispatch (port 1), XAsyncCancel and completion-port submission. It invokes the actual recovered A6B0 completion handler from the native callback; a small host-owned context carries the RequestServices pointer in addition to the easy object and is freed on callback or failed submit. The cancellation flag is intentionally ignored as in the original callback. Global decrement calls the recovered 16DE4 directly. WinHttpHost can opt into these bindings without changing the independent loopback harness; a native Win32 wait/reset/release-mutex adapter supplies the MultiHostServices primitives. The composed scheduler test now routes through the GDK interface slots, tests manual queue creation, cancellation, failed submission and completion with both cancellation flags. The GDK interface is modeled, not a live Gaming Services runtime; queue lifetime/drain and original 0xCEFC remain explicit owner responsibilities.


## Multi engine lifecycle and request-stop wrapper

The original hash-pinned instruction evidence run 45545280844 recovered 0x184A8 (multi engine initialization), 0x18110 (multi engine destruction), and 0xCEFC (a disabled-ETW wrapper around 0x1395C request stop). Multi creation now uses mutex, two initially nonsignaled auto-reset events, one initially signaled manual-reset idle event, intrusive engine registration and suspended-engine handling. Destruction detaches pending handles, cancels/drains active handles, detaches completed handles, unregisters under the engine mutex and lock, releases all completion messages and closes events/mutex. The original disabled-ETW routine does not delete the multi critical section; a future native ownership adapter must release it before freeing storage. Source-link tests cover successful creation, all four partial initialization failures, suspend at registration, real cancel/completion chain, message ownership and teardown. No original-machine differential pass claimed for these newly recovered functions yet.


## Easy destructor and bound handle lifecycle

The pinned A544 destructor, 113B8 active predicate (request states 1 through 5), FFA4 lock destruction, E544 option destruction and CB3C detach instructions were recovered from the two lifecycle evidence runs. Easy destruction now resets transfer metadata, terminates its GDK queue without waiting, drains both work/completion ports until neither dispatches, closes the queue and event, frees owned options and deletes all four critical sections in original order. The source-link HandleBinding directly invokes recovered easy/multi constructors and destructors, with original public accounting IDs 6/9/13/20. It uses the real engine bootstrap for implicit easy initialization; it refuses uninitialized multi creation and attached or active easy destruction as deliberate integration safety guards. Native multi critical sections are released before storage free. The remaining cancellation callback is now a direct CEFC translation, removing the host placeholder from TransferBinding. Added handle ownership, queue-drain, partial easy initialization failure and accounting tests. Live GDK and original-machine differential for these new bodies remain pending.


## Inline completion safety at admission

The original scheduler calls request start before inserting the easy into the active list. If GDK submission fails immediately, BA8C falls back to A6B0 synchronously and the completion cannot find the easy on that list. The bound scheduler now publishes the active list entry and count before invoking request start; the separately exposed faithful 0x18F68 function retains original ordering for differential tests. The regression forces security-query failure and subsequent completion-submit failure during admission, verifying the message appears and both active counters return to zero. Fixed a const-pointer typo in the new handle-binding fixture.


## Native asynchronous entrypoints and transfer workers

The 0x14A00 instruction evidence confirms the disabled-ETW WinHTTP callback forwards request context, notification, information and length directly to 0x13E14. CallbackBinding installs one process-wide native callback owner (matching the original global engine), supplies the 0x11F10 async-start token and Win32 status callback, and submits header, upload, deferred-read and body workers through the real recovered GDK task-queue wrappers. Generalized TaskQueueBinding keeps callback contexts alive until execution or failed submission. The owner must drain all callbacks before deactivation, and the source-link test models GDK queue delivery while exercising the actual worker routines, callback forwarding and ownership. The remaining request setup/status service composition and full live runtime acceptance are not complete.


## Native WinHTTP pipeline composition

WinHttpPipeline connects the recovered asynchronous request startup to URL cracking, engine session cache, native WinHTTP connection/open/registration/options/outgoing headers, and optional client certificate selection. Status callbacks now directly compose upload continuation, response-header query/publication, read scheduling, body publication, callback delivery and WinHttpQueryDataAvailable. The bridge uses recovered GDK worker submission and the native WinHTTP callback rather than manually driving individual leaves. Non-owned read buffers are rejected at the binding boundary. A requested client certificate or custom trailer without a verified service returns failure rather than silently disabling security/features. The host still supplies the URL warning/report services and original debug callback; original URL parsing, native engine notification registration and full end-to-end acceptance remain open. Windows smoke checks pipeline activation and service ownership in addition to the existing real loopback leaf tests.

### URL preparation coordinator (0x10EA4)
The pinned instruction and data evidence runs 45545280844 and 45546001831 establish the complete URL preparation branch structure, including parser error 3, default scheme literal `https`, setting part 5, rebuilding part 0 and removing its final byte. The source composes the existing owned wide conversion and WinHTTP crack routine; CURLU create/set/get/destroy remain explicit injectable boundaries, not a substitute parser. Failure clears the output and releases temporary strings; null/empty successful parser output is rejected to avoid the original underflow. A ten-path source-link test exercises parser, allocation and crack failures and successful fallback. This test is not a machine differential. The remaining lifecycle differential run 45545729574 passed 960/960 cases; source-link and native Windows loopback run 45545964487/45545964509 passed after pipeline integration.


## Store game-facing C ABI source-link facade
GameApi exports the 13 remaining stateful symbols alongside the three existing leaves, matching the 16 Store imports. It requires a deliberately installed, externally serialized owner; it is not a replacement DLL or a fully composed default runtime. Handle membership and a final-global-cleanup guard prevent common use-after-free errors; request state 6 is still in flight until completion resets it. Multi wrapper IDs 14/15/17/19/21 use original profiling, with telemetry flush still unported. The variadic setopt adapter covers the recovered supported option classes, and getinfo writes the original Windows-width fields. Extra poll descriptors are ignored as in the original 19C54 wrapper. Tests extend the handle fixture through actual C symbols, pointer/integer/callback arguments, pending removal and message ownership. No original-machine public-wrapper equivalence claimed for this composition.


## Debug callback and URL object verification
The original URL object differential run 45548745781 passed 1,920/1,920 cases across eight functions/subsets. The game-facing source-link ABI passed Windows and Linux run 45548509994. Original debug dispatch 0xA128 now invokes the registered callback with the original easy/type/data/count/userdata arguments, or emits the original text/header prefixes through native CRT stream and debugger output. The debugger-only fallback deliberately bounds the read to the supplied count instead of treating a raw network buffer as a NUL-terminated string. WinHTTP RequestServices now installs this callback by default, preventing the former null callback call when verbose transfers are enabled. This new debug adapter is source-tested, not yet machine differential.


## URL component mutation and formatting
The original descriptor table recovered in run 45550472340 contains only HTTPS (443) and HTTP (80), both with options bit 0x400 clear. The flag-zero component setter and URL formatter implement owned component replacement, percent escape lowercasing, port bounds, default port/scheme flags, path insertion, user/password, query and fragment formatting, and bracketed IPv6 zone handling from the original instructions. Whole-URL parsing and percent decode/encode flag paths remain separate. Source-link tests cover replacement/failed allocation, zone ownership, nondefault/default ports, fallback scheme and exact output strings; machine differential validation is pending.


## Flag-zero HTTP/HTTPS URL parsing
Recovered the main 1C780 HTTP/HTTPS branch and the 1C744 cleanup wrapper, control character scan 1C5D4, dot-segment normalization 1AE40, port parsing 1BFB4 and username/password extraction 1C678/1BD5C. The source uses original object slots and tagged allocations, and preserves the original slash-count check and URL split ordering. The initial/full absolute setter stages the parsed object and only replaces the destination on success. Explicitly unsupported here: file://, nonzero encoding flags and relative resolution into a populated CURLU. The request coordinator uses a fresh object, flag zero, and the separate original HTTPS fallback. Source tests include valid/invalid ports, IPv6 zone, dot segments, cleanup on allocation failure and transaction preservation; original-machine differential testing of these bodies remains pending.


## Native Win32 URL preparation binding
Win32UrlBinding owns the reconstructed CURLU object/parse/format service composition, original 10EA4 coordinator, native WinHttpCrackUrl, UTF-16 copy and callback token. It supplies PrepareServices directly to TransferBinding, including default port retrieval and original insecure-HTTP warning reporting through the recovered GDK error interface. The Windows smoke test now exercises a real absolute URL, missing-scheme HTTPS fallback and rejection before dispatch. This is not live GDK acceptance; unsupported parser branches remain explicitly documented.


## Native engine platform services
Win32EngineBinding supplies real critical sections, events, mutexes, performance counters, tagged allocation, GDK runtime initialization/shutdown, native connectivity callback 16960 and app suspend/resume callback 16B60. It dynamically resolves the same original app-notification API set and keeps its module alive until unregistration. Positive Win32 registration errors are converted into HRESULTs so the recovered engine cannot incorrectly advertise success. Telemetry object construction/destruction remains explicitly injected; actual registration and live GDK are not tested by the smoke fixture, which verifies native primitive and binding ownership only.


## Full game-facing loopback acceptance harness
Added a second native Windows executable that drives the 16-import C facade instead of manually invoking transport leaves. It uses real WinHTTP callbacks, native locks/events/mutexes, reconstructed URL preparation, engine lifecycle, scheduling, header/body delivery and cleanup. Its in-process GDK fixture is strictly restricted to localhost; only Gaming Services and app registration are modeled. It tests GET, direct POST, streamed 24,579-byte POST, completion messages, callback bytes, HTTP status and resource balancing; CI additionally scans the test PE for forbidden XCurl/libcurl dependencies. This is a test harness, not a production security substitute. Results pending.


## Single native runtime owner
Win32Runtime composes the real GDK loader, URL coordinator, async WinHTTP pipeline, scheduler, native engine, handle lifecycle and all sixteen C ABI entrypoints. Installation is transactional and exclusive; failed competing installations roll back only their own bindings. Uninstallation refuses while an engine, initialization reference or handle is live. The enclosing application must serialize public calls and finish cleanup before destroying the owner. Telemetry remains an explicitly supplied service and optional certificate/trailer handlers remain explicit. The native owner does not install a test security adapter or issue networking calls at installation. Windows smoke coverage exercises failed activation, idempotency, competing owners and reinstallation. The full game C ABI loopback run 45551082177 passed GET, direct POST, streamed POST, callback/message/cleanup and PE import verification with modeled GDK. Original URL parser run 45551806328 passed 332/332 comparisons after adding the missing CRT strcmp model; 1,920 previous URL-object cases remain green.


## Original telemetry object lifetime and concurrency default
Evidence run 45551949569 shows the initial 32-bit limit at 282BC is 12 (0x0c), now the Win32Runtime default rather than the provisional 64. The native local telemetry policy reconstructs the 0x40-byte object (allocation tag 3, self-linked list at +0x28, initial GetTickCount value at +0x38), record unlink/free (tag 2), owned destruction and GUID formatter with original byte permutation at 23530. The default owner no longer requires injected telemetry callbacks. This is intentionally a local non-publishing policy: original EventRegister/EventUnregister, provider enablement, event payload formatting/flush and global GUID state are not implemented. An application may supply both telemetry lifecycle callbacks for a different policy. Tests cover allocation failure, list ownership, tags and exact GUID format; no original-machine differential for this new lifecycle has yet run.


## Expanded full-pipeline stress and error cases
The WinHTTP game C ABI fixture now also admits 13 concurrent GETs with an 8-request cap, checks duplicate-add rejection, exact completion queue counts, reuses an easy, verifies malformed URL error 3 and its original HRESULT via getinfo, tests HTTP 404 fail-on-error result 22, and actively cancels an open slow WinHTTP request. It uses only loopback and keeps the modeled GDK isolated to tests. Expected connection resets during handle closure no longer produce misleading server tracebacks. The original UTF-16 hex table at 23500 is uppercase, correcting the telemetry GUID formatter accordingly. New acceptance results pending.


## Static Store link contract audit
The source-link fixture emits an MSVC linker map and explicitly calls all sixteen Store import symbols, including the original strerror leaf. The new read-only audit verifies the source-linked code addresses lie in executable PE sections and originate in the native static archive, rejects any XCurl/libcurl import in the native executable, independently checks the import tables of all three SHA-pinned original Store EXEs, and recovers the literal setopt/getinfo argument at each of their 60 documented sites. Recovered values must exist in the ABI adapter and dispatch; unresolved literals are reported rather than guessed. Neither the original PE files nor loader are modified. Batch acceptance run 45552140261 passed all new concurrency/reuse/error/cancel cases. Telemetry run 45552221524 passed 1,200 lifecycle and 445 GUID format comparisons with provider publication modeled as a boundary. Static link contract results pending.


## Native HTTPS acceptance fixture
Added a Windows CI-only TLS 1.2 loopback acceptance test using separate ephemeral CAs. Only the trusted CA is temporarily added to the disposable elevated runner's LocalMachine root store and removed by exact thumbprint in finally; the CurrentUser store prompted interactively and could not be used in CI. WinHTTP/Schannel retains its normal certificate verification, and the GDK test substitute additionally queries the actual server certificate and applies Windows chain and SSL hostname policy. The native C ABI must accept a trusted IP-SAN certificate and reject a separate untrusted CA without delivering a response body. This is not live Gaming Services validation and does not change production trust policy. Also clear stale last-error before native WinHttpOpen so the original unconditional error-87 check cannot retry on a stale error. Results pending.


## Secure session failure ownership
The raw 15F40 differential body preserves the original unconditional GetLastError retry and secure-option failure leak. The production integration variant shares the exact cache selection and tagged object lifecycle but retries only a failed open, closes a session whose security option failed, and rejects inconsistent success codes from failed OS calls instead of dereferencing a missing entry. No security option is skipped. WinHttpPipeline uses the bound variant, with nine source-test paths for success, reuse, retry, failure, and allocation rollback. The original differential entrypoint is unchanged.


## Remaining original method selectors
The game-facing loopback fixture now exercises the actual method-related Store setopt cases 44 (HEAD), 46 (PUT), 14 (input size), and 10036 (custom PATCH). It also covers the original verbose callback, header-in-body fallback, and exact request/response debug bytes. Results pending. Original static-link audit run 45552373910 passed all sixteen symbols; improved analysis run 45552668548 proved 57 of 60 call sites, identifying the remaining three as conditional option selections. A finite value-set propagation pass is now under validation.


## Complete original Store option call-site evidence
Run 45552862959 independently recovered the same 18 setopt and two getinfo value sets in each of the three original Store clients. The previously unresolved method-selection site has the conservative set {44,46,47,80}, not a single literal. All four selectors are implemented in the native dispatcher and C varargs ABI. The manifest now records the set and independent SP/ZM numeric evidence. Future static-link audits reject unresolved sites or any difference from the manifest. Source-link and request-engine differential runs 45552730617 and 45552730567 passed the secure-session integration change. Full live Gaming Services and BO2 executable integration are still outstanding.


## Independent native executable consumer
The main game build is still hybrid-linked to the original EXE using tools/build_recon.py; it has no complete native executable target. Do not add the C++20 XCurl implementation to the freestanding C++17 hybrid compilation or imply that changing IAT slots after process start removes the vendor DLL's startup dependency. The new independent Windows x64 consumer uses add_subdirectory with XCurl tests disabled and links the production owner directly. It checks real GDK initialization when available, otherwise validates failure accounting, repeated initialization, blocked handle creation, teardown and reinstall without replacing Gaming Services or sending traffic. On an actual licensed GDK environment, run native_xcurl_consumer.exe --require-live-gdk to require successful real engine and handle initialization. It does not test online services or launch BO2. CI packages the executable and linker map, with no vendor XCurl/libcurl imports.


## Native client certificate selection
The default WinHTTP pipeline now uses native Crypt32 store open (existing/read-only), exact SHA-1 thumbprint decoding, certificate lookup and WinHttpSetOption(WINHTTP_OPTION_CLIENT_CERT_CONTEXT) with the actual 40-byte CERT_CONTEXT. The raw original-machine differential selector is unchanged. The bound variant checks the option result and fails closed instead of silently continuing on client-authentication failure; it never modifies trust settings. Windows smoke tests cover absent/malformed selectors and missing stores; a successful mutual TLS handshake with an actual client key still requires dedicated validation. Optional injected certificate services remain supported.


## Mutual TLS acceptance extension
The isolated Windows TLS fixture additionally creates an ephemeral client key/certificate signed by the test CA, imports only the client PFX into the runner's personal store, and starts a separate loopback server requiring that exact client certificate. The C ABI exercises original client-certificate selector option 10025, first with a missing thumbprint (must fail without response), then with the actual imported thumbprint (must complete). The server checks the SHA-256 peer fingerprint. The temporary personal certificate/key and trust root are removed in finally; no production trust policy is changed. Results pending.


## Recompiled game import-shaped ABI
The optional Windows x64 target t6::xcurl_import_abi defines the sixteen compiler-generated __imp_curl_* pointer symbols in a static archive. This lets authored/recompiled Store-compatible source that retains __declspec(dllimport) declarations link directly to the reconstructed C ABI without a vendor import library. It does not redirect or rewrite any retail PE and does not affect the existing hybrid loader. The independent consumer now has a separate translation unit compiled with dllimport declarations that calls every symbol before runtime installation, verifies closed-owner behavior, and runs the production initialization probe afterward. Native link and PE checks pending for this extension.


## Real GDK startup diagnostics
The independent consumer passed run 45555177309 and again with native certificate selection in 45555220766. Live GDK initialization returned CURLcode 2 on the hosted runner; failed references, blocked handles, cleanup and retry were validated. The new bound engine bootstrap records whether the failure occurred in runtime loading/initialization or engine creation, retaining the exact underlying status and frequency availability without altering the original CURLcode or reference counting. Both explicit global initialization and implicit easy initialization use that report; the consumer prints it and can require a live engine via --require-live-gdk. Original-machine bootstrap calls use the same body with no report. Source tests cover allocation, runtime and registration failures followed by successful retry.


## Native game target link recipe
For a future Windows x64 CMake executable target `t6_game` (not yet present), the native link contract is:

```cmake
set(T6_XCURL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(reconstructed_dlls/xcurl)
target_link_libraries(t6_game PRIVATE t6::xcurl_research)
# Use t6::xcurl_import_abi instead if authored source retains dllimport declarations.
```

Construct and install one `t6_xcurl::Win32Runtime` before any stateful curl call. Keep it alive until the game has removed/cleaned all easy and multi handles and balanced global initialization references. The consumer demonstrates the correct lifetime. Do not install either target together with a vendor XCurl import library. The optional import ABI is verified by a separate PE audit: all sixteen data slots must point at the exact reconstructed executable symbols, be in non-executable data sections, and have DIR64 base relocations for ASLR. This is source-link compatibility for a future native game, not an import-table rewrite of retail binaries. The current hybrid runtime is unchanged.


## Native request boundary guards
The bound start path rejects a missing URL before calling Gaming Services, and checks the GDK result buffer size, returned metadata pointer, and item-array bounds before dereferencing them. A failed second URL crack reports a transfer error instead of completing as success. These are explicit native guards; original-machine differential entrypoints preserve their prior behavior. Windows integration adds five malformed security-result cases and one missing URL case, requiring error messages and clean reuse without sending an HTTP request for these cases. The successful mutual-TLS handshake was confirmed in run 45555499568; its certificate cleanup command was unsupported on the runner and was replaced by exact-thumbprint .NET certificate/key cleanup. Relocation audit run 45555554670 verified all 16 static slots and live GDK absence 0x89240101.


## Additional native transport edges
The bound startup path refuses a 64-bit upload size that cannot fit WinHTTP's DWORD arguments, returning code 43 with HRESULT 0x80070057 rather than silently truncating. The raw original comparison path still uses its original narrowing. Loopback acceptance adds a 32,783-byte chunked response, relative HTTP redirect and effective-URL check, short write callback, header abort callback, receive timeout and oversized direct-upload rejection. These cases exercise actual WinHTTP and the C ABI; new acceptance results pending.


## Native upload trailer formatter
Recovered 10420 from the pinned original's instructions: call easy+140 with slist output and userdata +138, count 5 bytes plus each string and CRLF, allocate/reuse tag-16 request buffer +B0/+A8, emit the zero-chunk marker and trailer lines, and free the callback-owned list. Bound integration checks DWORD length, records allocation/callback failures, and reloads the request buffer after possible replacement instead of preserving the original stale pointer. Native trailer formatting now has a default implementation, with five source-test modes (reuse, no list, callback failure, growth and allocation failure). No claim of a full live chunked-upload trailer handshake or original-machine differential yet. TLS cleanup passed in run 45556231443, but its earlier HTTP test had failed and was masked by the final PowerShell command; the workflow now treats the separate HTTP and TLS steps independently.
