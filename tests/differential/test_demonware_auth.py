"""Compare the Store GDK task pump before the Demonware HTTP exchange."""

import struct

from harness import OBJECTS
from test_service_lifecycle import METHOD, RELEASE, get32, query_user, reg, user_setup

AUTH = OBJECTS + 0x30000
TASK = AUTH + 0x3600
VTABLE = TASK + 0x100
DESTROY = TASK + 0x200
TOKEN = TASK + 0x300


def setup(status, step, pending=False, allocate=True):
    def write(writer):
        writer.ptr(AUTH + 0x150, TASK)
        writer.u32(AUTH + 0x158, step)
        writer.ptr(TASK, VTABLE)
        writer.u32(TASK + 0x40, status)
        writer.ptr(TASK + 0x48, TOKEN)
        writer.u8(TASK + 0x60, pending)
        writer.u8(TASK + 0x61, allocate)
        writer.ptr(VTABLE, DESTROY)
        writer.write(TOKEN, b"test GDK error\0")
    return write


def create_token(mu):
    assert reg(mu, 0) == TASK + 0x400 and reg(mu, 1) == 0x333
    return TASK + 0x400


def copy_token(mu):
    assert reg(mu, 1) == TOKEN
    mu.mem_write(reg(mu, 0), struct.pack("<Q", TOKEN))
    return reg(mu, 0)


def send_auth(mu):
    assert reg(mu, 0) == AUTH
    assert get32(mu, AUTH + 0x158) == 2
    assert bytes(mu.mem_read(reg(mu, 1), 8)) == struct.pack("<Q", TOKEN)
    return 0


CASES = [{
    "name": "BdHandleGdkAuthTask", "builds": ["t6mp_msstore", "t6zm_msstore"], "ret": "void",
    "inputs": [{"args": [AUTH], "setup": setup(*args)} for args in
               ((4, 0, True), (4, 0), (2, 7), (2, 0), (2, 0, False, False), (2, 1))],
    "fakes": {
        "BdGdkAuthTaskIsPending": lambda mu: mu.mem_read(TASK + 0x60, 1)[0],
        "BdGdkAuthTaskError": lambda _: TASK + 0x48,
        "BdGdkAuthTaskUser": lambda _: 0x555,
        "BdGdkAuthTaskToken": lambda _: TOKEN,
        "BdSetGdkAuthUser": lambda _: 0,
        "XCurlEasyHandle": lambda _: 0x333,
        "BdAllocate": lambda mu: TASK + 0x400 if mu.mem_read(TASK + 0x61, 1)[0] else 0,
        "BdConstructTokenTask": create_token,
        "BdCopyAuthString": copy_token,
        "BdDestroyAuthString": lambda _: 0,
        "BdSendAuthRequest": send_auth,
        "BdPlatformLog": lambda _: 0,
        DESTROY: lambda _: 0,
    },
    "observe": {"BdSetGdkAuthUser": "qq", "BdConstructTokenTask": "qq", DESTROY: "qi",
                "BdPlatformLog": "issssis"},
}]


CASES.append({
    "name": "GdkUserCheckPrivilege", "builds": ["t6mp_msstore", "t6zm_msstore"], "ret": "uint",
    "inputs": [{"args": [0x1234, 0, 254, bytes(1), bytes(4)], "setup": user_setup(20, available)}
               for available in (False, True)],
    "fakes": {"GdkQueryApi": query_user, METHOD: lambda _: 7, RELEASE: lambda _: 0},
    "observe": {METHOD: "qqiiqq", RELEASE: "q"},
})
