# XCurl original request engine — November 24, 2026

Source: exact local Store XCurl.dll, SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551. The original transport is WinHTTP, not libcurl or a separate TLS implementation.

16 disabled-ETW function bodies transcribed and tested locally, 160 cases each (2,560 total):
- B55C easy event and queue creation; A8F0 queue progress; CA60 first-failure preservation; B6B4 done predicate.
- 10AC4 request reset, A6B0 completion publication with nested recursive lock order, active/completed list movement and 40-byte message allocation; 16DE4 global counter decrement.
- 12124 read, 127A8 receive, 12D4C send (64-bit accounting, 32-bit WinHTTP lengths), 13038 callback/context registration; exact HRESULT conversion.
- 13C30 close, 1395C stop/cancellation transition, 13B14 final state, B90C notification, BA8C completion scheduling with synchronous fallback.

Test compares original and source callback sequence, return values, and entire normalized request/multi object bytes; original instruction execution occurs through Unicorn, with OS APIs and unported task queue / allocation / diagnostics modeled explicitly. Request reset connection-release (non-null COM pointer) branch is transcribed but not exercised by this fixture. The original unchecked completion-message allocation contract is retained and tested only for success. No ETW-enabled path, actual HTTP traffic, or complete callback dispatcher is claimed. All source joins t6::xcurl_research; no production DLL replacement is claimed.

Next: 13E14 WinHTTP status handler, 10748 connection/open-request, 131A8 option application, 18F68/19344 scheduler engine, task queue and actual Windows integration.
