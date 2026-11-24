# XCurl GDK interface map

Derived from the original Store XCurl.dll, SHA-256 pinned by `tools/dlls/probe_xcurl_gdk.py`. RVA is the original wrapper function entry. Vtable byte offsets and method names were verified against original callsites and cross-checked against the public interface layout documents.

| Original RVA | Interface provider / IID | Vtable byte offset | Method |
|---:|---|---:|---|
| 0x5424 | XThreading / XAsync | 0x28 | XAsyncCancel |
| 0x547c | XThreading / XAsync | 0x18 | XAsyncGetStatus |
| 0x5600 | XNetworking / XNetworking2 | 0x78 | XNetworkingGetConnectivityHint |
| 0x566c | XNetworking / XNetworking2 | 0x58 | XNetworkingQuerySecurityInformationForUrlUtf16Async |
| 0x56e8 | XNetworking / XNetworking2 | 0x68 | XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult |
| 0x5790 | XNetworking / XNetworking2 | 0x60 | XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize |
| 0x580c | XNetworking / XNetworking2 | 0x80 | XNetworkingRegisterConnectivityHintChanged |
| 0x58ac | XNetworking / XNetworking2 | 0x88 | XNetworkingUnregisterConnectivityHintChanged |
| 0x5934 | XNetworking / XNetworking2 | 0x70 | XNetworkingVerifyServerCertificate |
| 0x5ab4 | XThreading / XAsync | 0x88 | XTaskQueueCloseHandle |
| 0x5b10 | XThreading / XAsync | 0x60 | XTaskQueueCreate |
| 0x5b98 | XThreading / XAsync | 0x80 | XTaskQueueDispatch |
| 0x5c20 | XThreading / XAsync | 0x90 | XTaskQueueSubmitCallback |
| 0x5cbc | XThreading / XAsync | 0xb0 | XTaskQueueTerminate |
| 0x5d5c | XThreading / XAsync | 0xe8 | XThreadAssertNotTimeSensitive |

The GUIDs are stored at original data RVAs:

| RVA | GUID |
|---:|---|
| 0x20120 | `073b7dcb-1fcf-4030-94be-e3c9eb623428` |
| 0x20130 | `37e56907-2f10-41e8-b72f-36edb185331a` |
| 0x20140 | `bf2346b2-39af-4658-b5ea-44713c7e83b3` |

The four original wrappers that first invoke XThreadAssertNotTimeSensitive are 0x58ac, 0x5934, 0x5b10 and 0x5cbc. Interface pointers returned by QueryApiImpl are released via IUnknown::Release (vtable slot 2) after the method call.

The request-start path at 0x11978 uses:
- 0x547c: XAsyncGetStatus(asyncBlock, false)
- 0x5790: XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize(asyncBlock, &size)
- 0x56e8: XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult(asyncBlock, size, nullptr, buffer, &securityInformation)

The wrapper differential suite runs 96 original-machine comparisons for each of the 15 entries (1,440 total). The startup differential suite exercises the three used methods end-to-end (1,024 cases).

These wrappers accept a QueryApiImpl service callback. They do not yet load or initialize the actual Gaming Services runtime; that native loader/initialization path remains an explicit integration task.
