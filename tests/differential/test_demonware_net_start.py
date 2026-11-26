"""Compare the Store bdNet start-parameter constructor with modeled callees."""

import struct

from test_service_lifecycle import reg, get32


def init_address(mu):
    mu.mem_write(reg(mu, 0), bytes(0x86))


def init_upnp(mu):
    mu.mem_write(reg(mu, 0), b"\x01")


def construct_host(mu):
    mu.mem_write(reg(mu, 0), struct.pack("<Q", reg(mu, 1)))


def append_host(mu):
    count_address = reg(mu, 0) + 0xc
    count = get32(mu, count_address)
    assert count < 4
    host = ("us", "eu", "jp", "au")[count]
    text = f"stun.{host}.demonware.net\0".encode()
    address = struct.unpack("<Q", mu.mem_read(reg(mu, 1), 8))[0]
    assert bytes(mu.mem_read(address, len(text))) == text
    mu.mem_write(count_address, struct.pack("<I", count + 1))


CASES = [
    {"name": "BdInitNetStartParams", "builds": ["t6mp_msstore", "t6zm_msstore"], "ret": "uint64",
     "fakes": {
         "BdInitAddress": init_address,
         "BdInitSocketRouterConfig": lambda _: 0,
         "BdInitUpnpConfig": init_upnp,
         "BdInitStringFromCString": construct_host,
         "BdAppendDiscoveryHost": append_host,
         "BdDestroyAuthString": lambda _: 0,
     },
     "inputs": [[bytes(0x180)], [bytes([0xa5]) * 0x180]]},
]
