"""Compare the reconstructed Store transport entry points with modeled OS calls."""

import struct

from unicorn import x86_const as x86

from harness import OBJECTS

STORE = ["t6mp_msstore"]
MODE = OBJECTS
WORKER = OBJECTS + 0x100
REQUEST = OBJECTS + 0x800
BUFFER = OBJECTS + 0xB000
SLOT = 0x1441FCA10
TREE_HEAD = OBJECTS + 0xB300
TREE_NODE = OBJECTS + 0xB400


def value(mu, result):
    mu.reg_write(x86.UC_X86_REG_RAX, result & 0xFFFFFFFFFFFFFFFF)


def arg(mu, index):
    return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX,
                        x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[index])


def get(mu, address, fmt="<Q"):
    return struct.unpack(fmt, bytes(mu.mem_read(address, struct.calcsize(fmt))))[0]


def mode(mu):
    return get(mu, MODE, "<i")


def setup(value, extra=None):
    def prepare(writer):
        writer.u32(MODE, value)
        if extra:
            extra(writer)
    return prepare


def output(mu, address, result, fmt="<Q"):
    mu.mem_write(address, struct.pack(fmt, result))


def expect_connect(_, mu):
    family, port, ip, padding = struct.unpack("<H2sIQ", bytes(mu.mem_read(arg(mu, 1), 16)))
    assert family == 2 and padding == 0
    assert (port, ip) in ((b"\0{", 0x0100007F), (b"\x69\x87", 0x04030201))
    value(mu, -1 if mode(mu) else 0)


def probe_send(_, mu):
    packet = bytes(mu.mem_read(arg(mu, 1), 48))
    assert arg(mu, 2) == 48
    assert packet[:4] == b"\x1b\0\0\0"
    assert packet[40:] == struct.pack(">Q", (0x80000000 + 0x100) << 32)
    value(mu, 48)


def inet_parts(machine, mu):
    assert machine.read_cstr(arg(mu, 0)) == b"https://example.invalid/"
    assert arg(mu, 1) == len(b"https://example.invalid/")
    parts = arg(mu, 3)
    assert get(mu, parts, "<I") == 0x68
    assert get(mu, parts + 0x20, "<I") == 256
    assert get(mu, parts + 0x50, "<I") == 1024
    mu.mem_write(get(mu, parts + 0x18), b"example.invalid\0")
    output(mu, parts + 0x24, 443, "<H")
    value(mu, mode(mu) != 1)


def inet_cleanup(mu):
    assert arg(mu, 0) == SLOT
    return 0


def inet_setup(writer):
    writer.u8(0x1441FC943, 1)
    writer.u8(0x1441FCA04, 0)
    writer.u32(SLOT + 0x100, 0)


def dw_parts(_, mu):
    parts = arg(mu, 3)
    assert get(mu, parts + 0x20, "<I") == 0xFFFFFFFF
    assert get(mu, parts + 0x50, "<I") == 0xFFFFFFFF
    assert get(mu, parts + 0x60, "<I") == 0xFFFFFFFF
    if mode(mu):
        output(mu, parts + 0x14, 1 if mode(mu) == 1 else 2, "<I")
        output(mu, parts + 0x18, BUFFER)
        output(mu, parts + 0x20, 4, "<I")
        output(mu, parts + 0x24, 443, "<H")
        output(mu, parts + 0x48, BUFFER)
        output(mu, parts + 0x50, 4, "<I")
        output(mu, parts + 0x58, BUFFER)
        output(mu, parts + 0x60, 0, "<I")
    value(mu, mode(mu) != 0)


def wide_string(mu):
    mu.mem_write(arg(mu, 0), b"e\0x\0\0\0")
    output(mu, arg(mu, 0) + 16, 2)
    output(mu, arg(mu, 0) + 24, 7)
    return arg(mu, 0)


def init_http_request(mu):
    output(mu, arg(mu, 1), BUFFER)
    output(mu, arg(mu, 2), BUFFER)
    return 0x80004005 if mode(mu) == 0 else 0


def fake_count(mu):
    output(mu, arg(mu, 1), 1, "<I")
    return 0


def http_connect(_, mu):
    assert arg(mu, 2) & 0xFFFF == 443
    value(mu, 0 if mode(mu) == 2 else 0x7777)


def http_open(_, mu):
    assert arg(mu, 0) == 0x7777
    flags = get(mu, mu.reg_read(x86.UC_X86_REG_RSP) + 0x38, "<I")
    assert flags == 0x800040
    value(mu, 0 if mode(mu) == 3 else 0x8888)


def xc_setup(writer, existing):
    writer.u64(WORKER + 0x60, TREE_HEAD)
    writer.u64(TREE_HEAD + 8, TREE_NODE if existing else TREE_HEAD)
    writer.u8(TREE_HEAD + 0x19, 1)
    writer.u64(TREE_NODE, TREE_HEAD)
    writer.u64(TREE_NODE + 16, TREE_HEAD)
    writer.u64(TREE_NODE + 32, 0x1111)
    writer.u8(REQUEST, 1)
    writer.u64(REQUEST + 8, BUFFER)


def xc_insert(mu):
    assert arg(mu, 0) == WORKER + 0x60
    node = arg(mu, 2)
    assert get(mu, node + 0x20) == 0x1111
    assert get(mu, node + 0x28, "<B") == 1
    assert get(mu, node + 0x30) == BUFFER
    return 0


CASES = [
    {"name": "BdConnectStream", "builds": STORE, "ret": "int",
     "imports": {"ws2_32.dll!htons": (lambda _, mu: value(mu, 0x8769), 0),
                 "ws2_32.dll!connect": (expect_connect, 0),
                 "ws2_32.dll!WSAGetLastError": (lambda _, mu: value(mu, mode(mu)), 0)},
     "inputs": [{"args": [9, bytes(range(1, 5)), 27015], "setup": setup(error)}
                for error in (0, 10004, 10022, 10035, 10040, 10051, 10099)]},
    {"name": "Sys_ProbeTimeServer", "builds": STORE, "ret": "bool",
     "imports": {"api-ms-win-crt-time-l1-1-0.dll!_time64": (lambda _, mu: value(mu, 0xFC558280), 0),
                 "winmm.dll!timeGetTime": (lambda _, mu: value(mu, 5000), 0),
                 "ws2_32.dll!htons": (lambda _, mu: value(mu, 0x7B00), 0),
                 "ws2_32.dll!inet_addr": (lambda _, mu: value(mu, 0x0100007F), 0),
                 "ws2_32.dll!socket": (lambda _, mu: value(mu, -1 if mode(mu) == 2 else 7), 0),
                 "ws2_32.dll!connect": (expect_connect, 0),
                 "ws2_32.dll!htonl": (lambda _, mu: value(mu, int.from_bytes(
                     (arg(mu, 0) & 0xFFFFFFFF).to_bytes(4, "little"), "big")), 0),
                 "ws2_32.dll!sendto": (probe_send, 0),
                 "ws2_32.dll!select": (lambda _, mu: value(mu, 1), 0),
                 "ws2_32.dll!recvfrom": (lambda _, mu: value(mu, 48), 0),
                 "ws2_32.dll!closesocket": (lambda _, mu: value(mu, 0), 0)},
     "inputs": [{"args": [b"127.0.0.1\0"], "setup": setup(case)} for case in (0, 1, 2)]},
    {"name": "WinInetOpenConnection", "builds": STORE, "ret": "uint64",
     "imports": {"wininet.dll!InternetCrackUrlA": (inet_parts, 0),
                 "wininet.dll!InternetOpenA": (lambda _, mu: value(mu, 0 if mode(mu) == 2 else 0x1111), 0),
                 "wininet.dll!InternetConnectA": (lambda _, mu: value(mu, 0 if mode(mu) == 3 else 0x2222), 0)},
     "fakes": {"WinInetReleaseConnection": inet_cleanup}, "observe": {"WinInetReleaseConnection": "q"},
     "inputs": [{"args": [b"https://example.invalid/\0"], "setup": setup(case, inet_setup)}
                for case in (0, 1, 2, 3)]},
    {"name": "BdBeginWinHttpRequest", "builds": STORE, "ret": "void",
     "imports": {"winhttp.dll!WinHttpCrackUrl": (dw_parts, 0),
                 "winhttp.dll!WinHttpConnect": (http_connect, 0),
                 "kernel32.dll!GetLastError": (lambda _, mu: value(mu, 12029), 0)},
     "fakes": {"BdPlatformLog": lambda mu: 0, "BdCompleteWinHttpRequest": lambda mu: 0,
               "BdAllocate": lambda mu: BUFFER + 0x100, "BdFormatHttpHost": lambda mu: 0,
               "BdFormatHttpPath": lambda mu: 0, "BdSendWinHttpRequest": lambda mu: 0},
     "observe": {"BdCompleteWinHttpRequest": "qi", "BdSendWinHttpRequest": "q"},
     "inputs": [{"args": [WORKER, REQUEST], "setup": setup(case)} for case in (0, 1, 2, 4)]},
    {"name": "XCurlAddRequest", "builds": STORE, "ret": "int",
     "imports": {"xcurl.dll!curl_multi_add_handle": (lambda _, mu: value(mu, 4 if mode(mu) == 1 else 0), 0)},
     "fakes": {"HttpMutexLock": lambda mu: 0, "XCurlEasyHandle": lambda mu: 0x1111,
               "HttpClientLog": lambda mu: 0, "XCurlResult": lambda mu: 0x80004005,
               "HttpMutexUnlock": lambda mu: 0, "HttpDestroyEasyRequest": lambda mu: 0,
               "HttpFree": lambda mu: 0, "HttpQueueCallback": lambda mu: 0,
               "HttpAllocate": lambda mu: TREE_NODE + 0x100, "XCurlInsertNode": xc_insert},
     "observe": {"XCurlResult": "i", "HttpDestroyEasyRequest": "q", "HttpFree": "q"},
     "inputs": [{"args": [WORKER, REQUEST], "setup": setup(case, lambda w, c=case: xc_setup(w, c == 2))}
                for case in (0, 1, 2)]},
    {"name": "WinHttpInitializeRequest", "builds": STORE, "ret": "int",
     "imports": {"winhttp.dll!WinHttpConnect": (http_connect, 0),
                 "winhttp.dll!WinHttpOpenRequest": (http_open, 0),
                 "winhttp.dll!WinHttpSetTimeouts": (lambda _, mu: value(mu, mode(mu) != 4), 0),
                 "winhttp.dll!WinHttpSetOption": (lambda _, mu: value(mu, mode(mu) != 7), 0),
                 "winhttp.dll!WinHttpAddRequestHeaders": (lambda _, mu: value(mu, mode(mu) != 6), 0),
                 "kernel32.dll!GetLastError": (lambda _, mu: value(mu, 12029), 0),
                 "kernel32.dll!GetCurrentThreadId": (lambda _, mu: value(mu, 1), 0)},
     "fakes": {"HttpGetRequestMethodAndUrl": init_http_request, "HttpAssignNarrowString": lambda mu: 0,
               "HttpParseUrl": lambda mu: arg(mu, 0), "HttpMoveUrl": lambda mu: 0, "HttpDestroyUrl": lambda mu: 0,
               "HttpUrlHost": lambda mu: BUFFER + 0x200, "HttpUrlPort": lambda mu: 443, "HttpUrlSecure": lambda mu: 1,
               "HttpToWideString": wide_string, "HttpUrlResource": lambda mu: arg(mu, 1), "HttpWidenMethod": wide_string,
               "HttpGetTimeout": fake_count, "HttpGetBody": lambda mu: 0, "HttpGetHeaderCount": fake_count,
               "HttpFormatHeaders": wide_string, "HttpRequestId": lambda mu: 5,
               "HttpClientLog": lambda mu: 0, "HttpFree": lambda mu: 0},
     "inputs": [{"args": [WORKER], "setup": setup(case, lambda w: (
         w.u64(WORKER + 0x38, REQUEST), w.u64(BUFFER + 0x218, 15)))}
         for case in (0, 2, 3, 4, 5, 6, 7)]},
]
