"""Compare the Store client's Demonware rich presence task builders."""

import struct

from harness import OBJECTS
from test_service_lifecycle import reg, get32

STATE = OBJECTS + 0x12000
SERVICE = STATE + 0x100
RESULT = STATE + 0x200
BUFFER = STATE + 0x300
VTABLE = STATE + 0x400
DESTROY = STATE + 0x500
TASK = STATE + 0x600
USERS = (0x2100000000000001, 0x2100000000000002)
PRESENCE = b"Custom Games"


def get64(mu, address):
    return struct.unpack("<Q", bytes(mu.mem_read(address, 8)))[0]


def setup(method, failure, count=2):
    def write(writer):
        writer.u8(STATE, method)
        writer.u8(STATE + 1, failure)
        writer.u32(STATE + 4, count)
        writer.ptr(SERVICE, 0x999)
        writer.ptr(RESULT, 0xABC)
        writer.ptr(BUFFER, VTABLE)
        writer.ptr(VTABLE, DESTROY)
    return write


def mode(mu, offset=1):
    return mu.mem_read(STATE + offset, 1)[0]


def allocate(mu):
    assert reg(mu, 0) == 0x50
    return BUFFER


def construct(mu):
    capacity = 0x6A if mode(mu, 0) == 1 else 0x40 + 9 * get32(mu, STATE + 4)
    assert (reg(mu, 0), reg(mu, 1), reg(mu, 2) & 0xFF) == (BUFFER, capacity, 1)
    return BUFFER


def init(mu):
    assert get64(mu, reg(mu, 0)) == BUFFER and get64(mu, RESULT) == 0
    assert (reg(mu, 1) & 0xFF, reg(mu, 2) & 0xFF) == (0x44, mode(mu, 0))


def write_u64(mu):
    assert reg(mu, 0) == BUFFER
    if mode(mu, 0) == 1:
        assert reg(mu, 1) == 0
    else:
        assert reg(mu, 1) in USERS
    return int(mode(mu) != 1)


def write_blob(mu):
    assert reg(mu, 0) == BUFFER
    assert reg(mu, 2) == len(PRESENCE)
    assert bytes(mu.mem_read(reg(mu, 1), reg(mu, 2))) == PRESENCE
    return int(mode(mu) != 2)


def start(mu):
    assert (reg(mu, 0), reg(mu, 1), get64(mu, reg(mu, 2))) == (0x999, RESULT, BUFFER)
    if mode(mu) == 3:
        return 5
    mu.mem_write(RESULT, struct.pack("<Q", TASK))
    return 0


def set_result(mu):
    assert (reg(mu, 0), reg(mu, 1), reg(mu, 2)) == (TASK, 0x321, get32(mu, STATE + 4))


def destroy(mu):
    assert (reg(mu, 0), reg(mu, 1), get32(mu, BUFFER + 8)) == (BUFFER, 1, 0)


FAKES = {
    "BdAllocate": allocate, "BdConstructTaskByteBuffer": construct, "BdInitializeTaskBuffer": init,
    "BdByteBufferWriteUInt64": write_u64, "BdByteBufferWriteBlob": write_blob, "BdStartRemoteTask": start,
    "BdSetRemoteTaskResult": set_result, "BdPlatformLog": lambda _: 0, DESTROY: destroy,
}
OBSERVE = {"BdByteBufferWriteUInt64": "qq", "BdPlatformLog": "issssis", DESTROY: "qi"}
INFO = bytes(9) + PRESENCE.ljust(0x2c - 9, b"\0") + struct.pack("<I", len(PRESENCE))

CASES = [
    {"name": "BdRichPresenceSetInfo", "builds": ["t6mp_msstore", "t6zm_msstore"], "ret": "uint64",
     "fakes": FAKES, "observe": OBSERVE,
     "inputs": [{"args": [SERVICE, RESULT, INFO], "setup": setup(1, failure)} for failure in range(4)]},
    {"name": "BdRichPresenceGetInfo", "builds": ["t6mp_msstore", "t6zm_msstore"], "ret": "uint64",
     "fakes": FAKES, "observe": OBSERVE,
     "inputs": [{"args": [SERVICE, RESULT, struct.pack("<QQ", *USERS), 0x321, count],
                 "setup": setup(2, failure, count)}
                for count, failure in ((0, 0), (2, 0), (2, 1), (2, 3))]},
]
