"""Store GDK and Demonware wrappers, compared against the pinned executable without network I/O."""

import struct

from unicorn import x86_const as x86

from harness import OBJECTS

STORE = ["t6mp_msstore"]
GDK_MODULE = 0x140D450A8
NETWORK_ERROR = OBJECTS + 0x100
ADDRESS_INFO = OBJECTS + 0x200
SOCKADDR = OBJECTS + 0x300
SAVE_OBJECT = OBJECTS + 0x400
SAVE_VTABLE = OBJECTS + 0x500
SAVE_GET_PATH = OBJECTS + 0x600
SAVE_RELEASE = OBJECTS + 0x610


def return_value(mu, value):
    mu.reg_write(x86.UC_X86_REG_RAX, value)


def query_save_interface(mu):
    assert bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RCX), 16)) == bytes.fromhex(
        "58 3F 4C 70 29 E6 C2 4C B1 97 30 51 1B 99 6F E2"
    )
    assert bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RDX), 16)) == bytes.fromhex(
        "58 3F 4C 70 29 E6 C2 4C B1 97 30 51 1B 99 6E E2"
    )
    if struct.unpack("<Q", bytes(mu.mem_read(SAVE_OBJECT, 8)))[0] != SAVE_VTABLE:
        return 0x80004002
    mu.mem_write(mu.reg_read(x86.UC_X86_REG_R8), struct.pack("<Q", SAVE_OBJECT))
    return 0


def save_interface(writer):
    writer.u64(SAVE_OBJECT, SAVE_VTABLE)
    writer.u64(SAVE_VTABLE + 31 * 8, SAVE_GET_PATH)
    writer.u64(SAVE_VTABLE + 2 * 8, SAVE_RELEASE)


def get_save_path(mu):
    assert mu.reg_read(x86.UC_X86_REG_RCX) == SAVE_OBJECT
    assert mu.reg_read(x86.UC_X86_REG_RDX) == 0x1234
    assert mu.reg_read(x86.UC_X86_REG_R8) == 260
    mu.mem_write(mu.reg_read(x86.UC_X86_REG_R9), b"local-save\0")
    return 0


def model_load(machine, mu):
    name = machine.read_cstr(mu.reg_read(x86.UC_X86_REG_RCX))
    assert name == b"xgameruntime.dll"
    assert mu.reg_read(x86.UC_X86_REG_R8) == 0x800
    return_value(mu, struct.unpack("<Q", bytes(mu.mem_read(OBJECTS, 8)))[0])


def gdk_state(cached, load):
    def setup(writer):
        writer.u64(GDK_MODULE, cached)
        writer.u64(OBJECTS, load)
    return setup


def network_state(error):
    def setup(writer):
        writer.u32(NETWORK_ERROR, error)
        writer.write(ADDRESS_INFO, struct.pack("<iiiiQQQQ", 0, 2, 2, 0, 16, 0, SOCKADDR, 0))
        writer.write(SOCKADDR, struct.pack("<HH4s8x", 2, 0, b"\x7f\0\0\x01"))
    return setup


def network_error(mu):
    return struct.unpack("<I", bytes(mu.mem_read(NETWORK_ERROR, 4)))[0]


def lookup(machine, mu):
    hints = mu.reg_read(x86.UC_X86_REG_R8)
    name = machine.read_cstr(mu.reg_read(x86.UC_X86_REG_RCX))
    flags = 0 if name[:1].isalpha() else 4
    assert struct.unpack("<iiii", bytes(mu.mem_read(hints, 16))) == (flags, 2, 2, 0)
    error = network_error(mu)
    if not error:
        mu.mem_write(mu.reg_read(x86.UC_X86_REG_R9), struct.pack("<Q", ADDRESS_INFO))
    return_value(mu, error)


def send_ipv4(_, mu):
    expect = struct.pack("<H", 2) + struct.pack(">H", 27015) + b"\x01\x02\x03\x04" + bytes(8)
    stack = mu.reg_read(x86.UC_X86_REG_RSP)
    address = struct.unpack("<Q", bytes(mu.mem_read(stack + 0x28, 8)))[0]
    assert bytes(mu.mem_read(address, 16)) == expect
    return_value(mu, 0xFFFFFFFF if network_error(mu) else mu.reg_read(x86.UC_X86_REG_R8D))


def send_ipv6(_, mu):
    buffer = struct.unpack("<I4xQ", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RDX), 16)))[0]
    stack = mu.reg_read(x86.UC_X86_REG_RSP)
    address, length = struct.unpack("<QI", bytes(mu.mem_read(stack + 0x30, 12)))
    expect = struct.pack("<H", 23) + struct.pack(">H", 27015) + bytes(4) + bytes(range(1, 17)) + bytes(104)
    assert length == 128 and bytes(mu.mem_read(address, length)) == expect
    mu.mem_write(mu.reg_read(x86.UC_X86_REG_R9), struct.pack("<I", buffer))
    return_value(mu, 0xFFFFFFFF if network_error(mu) else 0)


def destination(family):
    return bytes(range(1, 17)) + bytes(112) + struct.pack("<HH", family, 0)


CASES = [
    {"name": "GdkLoadRuntime", "ret": "uint", "builds": STORE,
     "imports": {"kernel32.dll!LoadLibraryExA": (model_load, 0)},
     "inputs": [{"args": [], "setup": gdk_state(cached, load)} for cached, load in ((0, 0), (0, 0x1234), (0x5678, 0))]},
    {"name": "GdkSavePath", "ret": "uint", "builds": STORE,
     "fakes": {"GdkQueryApi": query_save_interface, SAVE_GET_PATH: get_save_path, SAVE_RELEASE: lambda mu: 0},
     "observe": {SAVE_RELEASE: "q"},
     "inputs": [[0, 260, bytes(260)], {"args": [0x1234, 260, bytes(260)], "setup": save_interface}]},
    {"name": "Sys_ReportUnhandledException", "ret": "int", "builds": STORE,
     "fakes": {"Com_Error": lambda mu: 0}, "observe": {"Com_Error": "is"},
     "imports": {"faultrep.dll!ReportFault": (lambda _, mu: return_value(mu, 0), 0)}, "inputs": [[OBJECTS]]},
    {"name": "BdGetHostByName", "ret": "uint", "builds": STORE,
     "fakes": {"BdPlatformLog": lambda mu: 0}, "observe": {"BdPlatformLog": "issssis"},
     "imports": {
         "ws2_32.dll!getaddrinfo": (lookup, 0),
         "ws2_32.dll!freeaddrinfo": (lambda _, mu: return_value(mu, 0), 0),
         "ws2_32.dll!WSAGetLastError": (lambda _, mu: return_value(mu, network_error(mu)), 0),
         "api-ms-win-crt-string-l1-1-0.dll!isalpha": (lambda _, mu: return_value(mu, int(chr(mu.reg_read(x86.UC_X86_REG_CL)).isalpha())), 0),
     },
     "inputs": [{"args": [name, b"\xcc" * 264, capacity], "setup": network_state(error)}
                for name in (b"test.invalid\0", b"127.0.0.1\0") for capacity in (0, 1, 2) for error in (0, 11001, 10051)]},
    {"name": "BdSendTo", "ret": "int", "builds": STORE,
     "fakes": {"BdPlatformLog": lambda mu: 0}, "observe": {"BdPlatformLog": "issssis"},
     "imports": {
         "ws2_32.dll!htons": (lambda _, mu: return_value(mu, int.from_bytes(
             mu.reg_read(x86.UC_X86_REG_CX).to_bytes(2, "little"), "big")), 0),
         "ws2_32.dll!sendto": (send_ipv4, 0),
         "ws2_32.dll!WSASendTo": (send_ipv6, 0),
         "ws2_32.dll!WSAGetLastError": (lambda _, mu: return_value(mu, network_error(mu)), 0),
     },
     "inputs": [{"args": [socket, destination(family), 27015, b"test", 4], "setup": network_state(error)}
                for socket in (-1, 8) for family in (2, 23, 99) for error in (0, 10004, 10035, 10040, 10051, 10099)]},
]
