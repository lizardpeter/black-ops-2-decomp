"""Exercise Store service dispatch, XUser handle operations and Demonware HTTP completion."""

import struct

from unicorn import x86_const as x86

from harness import OBJECTS

STORE = ["t6mp_msstore"]
DISPATCH = 0x140D45098
FLAGS = DISPATCH + 8
MODULE = DISPATCH + 0x10
TEST = OBJECTS + 0x12000
EXPORTS = {name: TEST + 0x100 + i * 16 for i, name in enumerate((
    "QueryApiImpl", "InitializeApiImpl", "InitializeApiImplEx", "XErrorReport", "UninitializeApiImpl"
))}
OBJECT = TEST + 0x300
VTABLE = TEST + 0x400
METHOD = TEST + 0x500
RELEASE = TEST + 0x510
RESPONSE = TEST + 0x600
REFERENCE = TEST + 0x700


def ret(mu, result):
    mu.reg_write(x86.UC_X86_REG_RAX, result & ((1 << 64) - 1))


def reg(mu, i):
    return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[i])


def get32(mu, address):
    return struct.unpack("<I", bytes(mu.mem_read(address, 4)))[0]


def lock_call(_, mu):
    assert reg(mu, 0) == DISPATCH
    ret(mu, 0)


def proc(machine, mu):
    assert reg(mu, 0) == 0x321
    name = machine.read_cstr(reg(mu, 1)).decode()
    mode = get32(mu, TEST)
    if (mode == 1 and name == "InitializeApiImplEx") or (mode == 2 and name == "UninitializeApiImpl"):
        ret(mu, 0)
    else:
        ret(mu, EXPORTS[name])


def load_runtime(mu):
    return 0x89240101


def initialize_ex(mu):
    assert reg(mu, 0) == 0x633600002711 and reg(mu, 1) == 0x633600000C6D
    return 0x80004005 if get32(mu, TEST) == 3 else 1


def dispatch_state(mode=0, initialized=0, implicit=0, module=0x321, cached=False):
    def setup(writer):
        mu = writer.m.mu
        if not any(start <= 0x7FFE0000 <= end for start, end, _ in mu.mem_regions()):
            mu.mem_map(0x7FFE0000, 0x1000)
        writer.u32(TEST, mode)
        writer.write(FLAGS, bytes((initialized, implicit)))
        writer.u64(MODULE, module)
        for i, address in enumerate(EXPORTS.values()):
            writer.u64(MODULE + 8 + i * 8, address if cached else 0)
    return setup


def query_user(mu):
    assert bytes(mu.mem_read(reg(mu, 0), 16)).hex() == "77d1ac01f9916347a38eccbb55ce32e0"
    assert bytes(mu.mem_read(reg(mu, 1), 16)).hex() == "e315940727677f438e9d8f8f9b2439f7"
    if not get32(mu, TEST):
        return 0x80004002
    mu.mem_write(reg(mu, 2), struct.pack("<Q", OBJECT))
    return 0


def user_setup(slot, available):
    def setup(writer):
        writer.u32(TEST, available)
        writer.u64(OBJECT, VTABLE)
        writer.u64(VTABLE + slot * 8, METHOD)
        writer.u64(VTABLE + 2 * 8, RELEASE)
    return setup


def raw_query(mu):
    return get32(mu, TEST)


def body_state(error, completion, release):
    def setup(writer):
        writer.u32(RESPONSE + 0x10, 12)
        writer.u32(RESPONSE + 0x14, 503)
        writer.u8(RESPONSE + 0x18, error)
        writer.u64(RESPONSE + 0x40, REFERENCE if release else 0)
        writer.u64(RESPONSE + 0x48, 0x1234 if completion else 0)
        writer.u32(RESPONSE + 0x78, 77)
        writer.u32(RESPONSE + 0x7c, 1)
        writer.u32(TEST, 1 if completion else 0)
    return setup


def release_reference(writer, count):
    writer.u64(REFERENCE + 8, OBJECT if count else 0)
    writer.u64(OBJECT, VTABLE)
    writer.u32(OBJECT + 8, count)
    writer.u64(VTABLE, METHOD)


LOCK_IMPORTS = {
    "kernel32.dll!AcquireSRWLockExclusive": (lock_call, 0),
    "kernel32.dll!ReleaseSRWLockExclusive": (lock_call, 0),
}
CASES = [
    {"name": "GdkInitializeRuntime", "ret": "uint", "builds": STORE,
     "imports": {**LOCK_IMPORTS, "kernel32.dll!GetProcAddress": (proc, 0)},
     "fakes": {"GdkLoadRuntime": load_runtime,
               EXPORTS["InitializeApiImpl"]: lambda mu: 0, EXPORTS["InitializeApiImplEx"]: initialize_ex,
               EXPORTS["XErrorReport"]: lambda mu: 0},
     "observe": {EXPORTS["InitializeApiImpl"]: "ii", EXPORTS["InitializeApiImplEx"]: "qqq",
                 EXPORTS["XErrorReport"]: "is"},
     "inputs": [
         {"args": [flags], "setup": dispatch_state(mode, initialized, implicit, module, cached)}
         for flags, mode, initialized, implicit, module, cached in (
             (0, 0, 0, 0, 0, False), (0, 0, 0, 0, 0x321, False), (1, 0, 0, 0, 0x321, False),
             (0, 1, 0, 0, 0x321, False), (1, 1, 0, 0, 0x321, False),
             (0, 2, 0, 0, 0x321, False), (0, 3, 0, 0, 0x321, False),
             (0, 0, 1, 1, 0x321, True), (1, 0, 1, 0, 0x321, True),
         )
     ]},
    {"name": "GdkQueryApi", "ret": "uint", "builds": STORE,
     "fakes": {"GdkInitializeRuntime": lambda mu: 0x80004005,
               EXPORTS["QueryApiImpl"]: raw_query, EXPORTS["XErrorReport"]: lambda mu: 0},
     "observe": {"GdkInitializeRuntime": "q", EXPORTS["XErrorReport"]: "is"},
     "inputs": [{"args": [0x1122, 0x3344, bytes(8)], "setup": dispatch_state(error, initialized, cached=True)}
                for initialized, error in ((0, 0), (1, 0), (1, 0x80004002), (1, 0x80040154))]},
    {"name": "GdkUninitializeRuntime", "ret": "void", "builds": STORE,
     "imports": {**LOCK_IMPORTS, "kernel32.dll!FreeLibrary": (lambda _, mu: ret(mu, 1), 0)},
     "fakes": {EXPORTS["UninitializeApiImpl"]: lambda mu: 0},
     "observe": {EXPORTS["UninitializeApiImpl"]: ""},
     "inputs": [{"args": [], "setup": dispatch_state(initialized=initialized, implicit=implicit, module=module, cached=cached)}
                for initialized, implicit, module, cached in ((1, 0, 0x321, True), (1, 1, 0x321, True), (0, 0, 0, False))]},
    {"name": "BdCompleteWinHttpRequest", "ret": "void", "builds": STORE,
     "fakes": {"BdStopwatchElapsed": lambda mu: 0.125 if get32(mu, TEST) else 0.001,
               "BdPlatformLog": lambda mu: 0, "BdReleaseReference": lambda mu: 0},
     "observe": {"BdPlatformLog": "issssis", "BdReleaseReference": "q"},
     "inputs": [{"args": [RESPONSE, result], "setup": body_state(error, completion, release)}
                for completion, release in ((False, False), (False, True), (True, False), (True, True))
                for error, result in ((0, 0), (0, 0x80004004), (0, 0x80072EFD), (2, 0x80004004), (7, 0))]},
    {"name": "BdHttpErrorName", "ret": "cstr", "builds": STORE, "inputs": [[i] for i in (*range(9), 255)]},
    {"name": "BdHttpStatusName", "ret": "cstr", "builds": STORE, "inputs": [[i] for i in range(-1, 5)]},
    {"name": "BdReleaseReference", "ret": "void", "builds": STORE,
     "fakes": {METHOD: lambda mu: 0}, "observe": {METHOD: "qi"},
     "inputs": [{"args": [REFERENCE], "setup": lambda w, c=count: release_reference(w, c)} for count in (0, 1, 2)]},
]

for name, slot, args, result in (
    ("GdkUserAddAsync", 7, [1, 0x1234], "uint"),
    ("GdkUserAddResult", 8, [0x1234, 0x5678], "uint"),
    ("GdkUserDuplicateHandle", 3, [0x1234, 0x5678], "uint"),
    ("GdkUserCloseHandle", 4, [0x1234], "void"),
):
    CASES.append({
        "name": name, "ret": result, "builds": STORE,
        "fakes": {"GdkQueryApi": query_user, METHOD: lambda mu: 7, RELEASE: lambda mu: 0},
        "observe": {METHOD: "qq" if result == "void" else "qqq", RELEASE: "q"},
        "inputs": [{"args": args, "setup": user_setup(slot, available)} for available in (False, True)],
    })


def send_state(stage, body, headers):
    def setup(writer):
        writer.u32(TEST, stage)
        writer.u32(RESPONSE + 0x10, 12)
        writer.u64(RESPONSE + 0xa0d0, 0xeeff)
        writer.u32(RESPONSE + 0x20b0, 700)
        writer.u32(RESPONSE + 0x38, 87)
        writer.u32(RESPONSE + 0x64, 5)
        writer.u64(RESPONSE + 0x30, body)
        writer.u64(RESPONSE + 0x48, 0x5678)
        writer.write(RESPONSE + 0x20d0, b"h\0\0\0" if headers else bytes(4))
    return setup


def stage(mu):
    return get32(mu, TEST)


def get_stack(mu, offset, fmt="<I"):
    return struct.unpack(fmt, bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RSP) + offset, struct.calcsize(fmt))))[0]


def open_http(_, mu):
    assert reg(mu, 0) == 0xeeff and reg(mu, 1) == RESPONSE + 0x1088
    assert get_stack(mu, 0x38) == 0x800000
    ret(mu, 0 if stage(mu) == 1 else 0x1122)


def send_http(_, mu):
    assert reg(mu, 0) == 0x1122
    assert get_stack(mu, 0x28) == (87 if reg(mu, 3) else 0)
    assert get_stack(mu, 0x30) == (0 if reg(mu, 3) else 87)
    assert get_stack(mu, 0x38, "<Q") == RESPONSE
    ret(mu, stage(mu) != 4)


def frequency(mu):
    return struct.unpack("<q", bytes(mu.mem_read(TEST, 8)))[0]


def counter(mu):
    return struct.unpack("<q", bytes(mu.mem_read(TEST + 8, 8)))[0]


def clock_state(hz, ticks):
    def setup(writer):
        writer.u64(TEST, hz)
        writer.u64(TEST + 8, ticks)
    return setup


CASES += [
    {"name": "BdSendWinHttpRequest", "ret": "void", "builds": STORE,
     "imports": {"winhttp.dll!WinHttpOpenRequest": (open_http, 0),
                 "winhttp.dll!WinHttpSetTimeouts": (lambda _, mu: ret(mu, stage(mu) != 2), 0),
                 "winhttp.dll!WinHttpAddRequestHeaders": (lambda _, mu: ret(mu, stage(mu) != 3), 0),
                 "winhttp.dll!WinHttpCloseHandle": (lambda _, mu: ret(mu, 1), 0),
                 "winhttp.dll!WinHttpSendRequest": (send_http, 0),
                 "kernel32.dll!GetLastError": (lambda _, mu: ret(mu, 12029), 0)},
     "fakes": {"BdPlatformLog": lambda mu: 0, "BdCompleteWinHttpRequest": lambda mu: 0},
     "observe": {"BdPlatformLog": "issssis", "BdCompleteWinHttpRequest": "qi"},
     "inputs": [{"args": [RESPONSE], "setup": send_state(state, body, headers)}
                for state, body, headers in ((1, 0, False), (2, 0, False), (3, 0, True),
                                              (4, 0, False), (4, 0x1234, True),
                                              (0, 0, False), (0, 0x1234, True))]},
    {"name": "BdClockMicroseconds", "ret": "uint64", "builds": STORE,
     "fakes": {"BdPerformanceFrequency": frequency, "BdPerformanceCounter": counter},
     "inputs": [{"args": [], "setup": clock_state(hz, ticks)}
                for hz, ticks in ((10000000, 123456789), (2400001, 100747211), (2400001, -100747211))]},
    {"name": "BdElapsedSeconds", "ret": "float", "builds": STORE,
     "inputs": [[12, 10012], [100, 12], [12, 12], [0, (1 << 64) - 1], [0, (1 << 63) + 1]]},
    {"name": "BdStopwatchElapsed", "ret": "float", "builds": STORE,
     "fakes": {"BdClockMicroseconds": lambda mu: 10_000_002},
     "inputs": [[struct.pack("<Q", start)] for start in (0, 1, 10_000_000, 10_000_003)]},
]


def query_threading(mu):
    expected = bytes.fromhex("cb7d3b07cf1f304094bee3c9eb623428")
    assert bytes(mu.mem_read(reg(mu, 0), 16)) == expected
    assert bytes(mu.mem_read(reg(mu, 1), 16)) == expected
    if not get32(mu, TEST):
        return 0x80004002
    mu.mem_write(reg(mu, 2), struct.pack("<Q", OBJECT))
    return 0


for name, slot, args, result, arguments in (
    ("GdkPrepareThreading", 29, [], "void", "q"),
    ("GdkTaskQueueCreate", 12, [1, 0, 0x5678], "uint", "qiiq"),
    ("GdkTaskQueueClose", 17, [0x1234], "void", "qq"),
    ("sub_1407893BC", 16, [0x1234, 1, 345], "bool", "qqii"),
    ("HttpQueueCallback", 18, [0x1234, 1, 0x5678, 0x9abc], "int", "qqiqq"),
):
    fakes = {"GdkQueryApi": query_threading, METHOD: lambda mu: 1, RELEASE: lambda mu: 0}
    observe = {METHOD: arguments, RELEASE: "q"}
    if name == "GdkTaskQueueCreate":
        fakes["GdkPrepareThreading"] = lambda mu: 0
        observe["GdkPrepareThreading"] = ""
    CASES.append({
        "name": name, "ret": result, "builds": STORE, "fakes": fakes, "observe": observe,
        "inputs": [{"args": args, "setup": user_setup(slot, available)} for available in (False, True)],
    })


CASES.append({
    "name": "XCurlProcessQueue", "ret": "void", "builds": STORE,
    "fakes": {"XCurlPerform": lambda mu: get32(mu, TEST),
              "HttpClientLog": lambda mu: 0, "XCurlFailAll": lambda mu: 0},
    "observe": {"XCurlPerform": "q", "XCurlFailAll": "qi", "HttpClientLog": "qissi"},
    "inputs": [{"args": [0x1122, cancelled], "setup": dispatch_state(mode)}
               for cancelled, mode in ((0, 0), (0, 0x80004005), (1, 0))],
})


MANAGER = TEST + 0xA000
NODE = MANAGER + 0x100
SECOND = MANAGER + 0x180
TASK = MANAGER + 0x200
REPLY = MANAGER + 0x300
PAYLOAD = MANAGER + 0x400
INPUT = MANAGER + 0x500
OUTPUT = MANAGER + 0x600


def reply_buffer(writer, size):
    writer.u64(REPLY, VTABLE)
    writer.u32(REPLY + 8, 1)
    writer.u64(REPLY + 0x10, PAYLOAD)
    writer.u32(REPLY + 0x18, size)
    writer.u64(REPLY + 0x20, PAYLOAD)
    writer.u64(PAYLOAD, 0x1122334455667788)
    writer.u64(VTABLE, RELEASE)
    writer.u64(VTABLE + 8, METHOD)
    writer.u64(INPUT, REPLY)


def remote_state(size=8, status=1, has_task=True, second=False):
    def setup(writer):
        reply_buffer(writer, size)
        writer.u32(TEST, status)
        writer.u64(MANAGER + 8, NODE)
        writer.u64(MANAGER + 0x10, SECOND if second else NODE)
        writer.u32(MANAGER + 0x18, 2 if second else 1)
        writer.u64(MANAGER + 0x70, 0x789)
        writer.u64(NODE, TASK if has_task else 0)
        writer.u64(NODE + 8, SECOND if second else 0)
        writer.u64(SECOND + 16, NODE if second else 0)
        writer.u64(TASK, VTABLE)
        writer.u32(TASK + 8, 1)
    return setup


LOG_FAKE = {"BdPlatformLog": lambda mu: 0}
LOG_OBSERVE = {"BdPlatformLog": "issssis"}
REMOTE_FAKES = {**LOG_FAKE, METHOD: lambda mu: get32(mu, TEST), RELEASE: lambda mu: 0,
                "BdFree": lambda mu: 0}
REMOTE_OBSERVE = {**LOG_OBSERVE, METHOD: "q", RELEASE: "qi", "BdFree": "q"}


CASES += [
    {"name": "BdBytePackerRemoveBuffer", "ret": "bool", "builds": STORE,
     "fakes": LOG_FAKE, "observe": LOG_OBSERVE,
     "inputs": [{"args": [PAYLOAD, 8, offset, INPUT, destination, size],
                 "setup": lambda w: reply_buffer(w, 8)}
                for offset, destination, size in ((0, OUTPUT, 8), (4, OUTPUT, 4), (4, OUTPUT, 5), (5, 0, 4))]},
    {"name": "BdByteBufferRead", "ret": "uchar", "builds": STORE,
     "fakes": LOG_FAKE, "observe": LOG_OBSERVE,
     "inputs": [{"args": [REPLY, OUTPUT, count], "setup": lambda w: reply_buffer(w, 8)}
                for count in (0, 4, 8, 9)]},
    {"name": "BdRemoteTaskAcceptReply", "ret": "void", "builds": STORE,
     "fakes": REMOTE_FAKES, "observe": REMOTE_OBSERVE,
     "inputs": [{"args": [TASK, INPUT, 0x1234], "setup": remote_state()},
                {"args": [TASK, TASK + 0x20, 0x1234], "setup": remote_state()}]},
    {"name": "BdPendingTaskPop", "ret": "void", "builds": STORE,
     "fakes": REMOTE_FAKES, "observe": REMOTE_OBSERVE,
     "inputs": [{"args": [MANAGER + 8], "setup": remote_state(second=second)} for second in (False, True)]},
    {"name": "BdRemoteTaskHandleReply", "ret": "void", "builds": STORE,
     "fakes": REMOTE_FAKES, "observe": REMOTE_OBSERVE,
     "inputs": [{"args": [MANAGER, INPUT], "setup": remote_state(size, status, task, second)}
                for size, status, task, second in ((8, 1, True, False), (4, 1, True, False), (8, 0, True, False),
                                                  (8, 1, False, False), (8, 1, True, True))]},
]


def user_get_id(mu):
    assert reg(mu, 0) == OBJECT and reg(mu, 1) == 0x1234
    mu.mem_write(reg(mu, 2), struct.pack("<Q", 0x1122334455667788))
    return 0


CASES.append({
    "name": "GdkUserGetId", "ret": "uint", "builds": STORE,
    "fakes": {"GdkQueryApi": query_user, METHOD: user_get_id, RELEASE: lambda mu: 0},
    "observe": {METHOD: "qqx", RELEASE: "q"},
    "inputs": [{"args": [0x1234, b"\xcc" * 8], "setup": user_setup(11, available)}
               for available in (False, True)],
})


LOBBY = TEST + 0xc000
AUTH = TEST + 0xc800
ADDR = TEST + 0xca00
COMMON = TEST + 0xcc00
CONNECTION = TEST + 0xce00
PREVIOUS = TEST + 0xd000
AUTH_BYTES = bytes(range(160))


def lobby_state(mode, has_previous):
    def setup(writer):
        writer.u32(TEST, mode)
        writer.write(AUTH, AUTH_BYTES)
        writer.u64(LOBBY + 0x408, PREVIOUS if has_previous else 0)
        writer.u8(LOBBY + 0x410, 1)
        for address in (COMMON, CONNECTION, PREVIOUS):
            writer.u64(address, VTABLE)
            writer.u32(address + 8, int(address == PREVIOUS))
        writer.u64(VTABLE, RELEASE)
    return setup


def lobby_allocate(mu):
    size = reg(mu, 0)
    assert size in (0xb8, 0xc0)
    mode = stage(mu)
    if (mode, size) in ((1, 0xb8), (2, 0xc0)):
        return 0
    return COMMON if size == 0xb8 else CONNECTION


def construct_lobby(mu):
    assert reg(mu, 0) == CONNECTION and reg(mu, 2) & 0xffffffff == 2048 and reg(mu, 3) & 0xffffffff == 4096
    assert get_stack(mu, 0x28, "<Q") == LOBBY
    owned_addr = struct.unpack("<Q", bytes(mu.mem_read(reg(mu, 1), 8)))[0]
    assert owned_addr == (0 if stage(mu) == 1 else COMMON)
    if owned_addr:
        assert get32(mu, COMMON + 8) == 2
    return CONNECTION


def start_lobby(mu):
    connection = 0 if stage(mu) == 2 else CONNECTION
    assert reg(mu, 0) == connection and reg(mu, 1) == LOBBY + 0x414
    assert bytes(mu.mem_read(reg(mu, 1), 160)) == AUTH_BYTES
    return int(bool(connection) and stage(mu) != 3)


CASES.append({
    "name": "BdLobbyServiceConnect", "ret": "bool", "builds": STORE,
    "fakes": {
        **LOG_FAKE,
        "BdLobbyDisconnectConnection": lambda mu: 0,
        "BdLobbyResetService": lambda mu: 0,
        "BdAllocate": lobby_allocate,
        "BdConstructCommonAddr": lambda mu: COMMON,
        "BdConstructLobbyConnection": construct_lobby,
        "BdStartLobbyConnection": start_lobby,
        "BdDestroyLobbyAddress": lambda mu: 0,
        RELEASE: lambda mu: 0,
    },
    "observe": {
        **LOG_OBSERVE,
        "BdLobbyDisconnectConnection": "q", "BdLobbyResetService": "q",
        "BdAllocate": "q", "BdConstructCommonAddr": "qq", "BdConstructLobbyConnection": "qxiiq",
        "BdStartLobbyConnection": "qq", "BdDestroyLobbyAddress": "q", RELEASE: "qi",
    },
    "inputs": [
        {"args": [LOBBY, ADDR, AUTH if has_auth else 0, 2048, 4096, 0x27],
         "setup": lobby_state(mode, previous)}
        for mode, has_auth, previous in ((0, False, True), (0, True, False), (0, True, True),
                                         (1, True, False), (2, True, True), (3, True, False))
    ],
})
