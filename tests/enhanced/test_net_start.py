"""P0015: bdNet starts with the native public-address override at loopback (the game's own port) and no UPnP
discovery; everything else in the start parameters is the original's."""
import struct

import test_demonware_net_start as faithful

PATCH = "P0015"
BUILDS = ["t6mp_msstore"]
# offsets in include/demonware/bd_net_start.h's BdNetStartParams (x64)
OFFSETS = {"publicAddress": 56, "publicPort": 188, "upnpConfig": 368}


def check_loopback_public_address(ctx):
    fakes = dict(faithful.CASES[0]["fakes"])
    parsed = []

    def parse(mu):
        a = faithful.reg(mu, 1)
        parsed.append(bytes(mu.mem_read(a, 10)).split(b"\0")[0])
    hooks = ctx.hooks({**fakes, "BdParseSocketAddress": parse})
    for args in faithful.CASES[0]["inputs"]:
        want = ctx.m.call(ctx.original("BdInitNetStartParams"), args, "uint64", fakes=hooks)
        parsed.clear()
        got = ctx.m.call(ctx.enhanced("BdInitNetStartParams"), args, "uint64", fakes=hooks)
        assert not isinstance(got[0], str), got[0]
        assert parsed == [b"127.0.0.1"], parsed
        orig, enh = bytearray(want[1][0]), bytearray(got[1][0])
        # the override's three fields: the public port is the game port, UPnP is off; the rest is unchanged
        game_port = struct.unpack_from("<H", orig, 2)[0]
        assert struct.unpack_from("<H", enh, OFFSETS["publicPort"])[0] == game_port
        assert enh[OFFSETS["upnpConfig"]] == 0
        for name, size in (("publicPort", 2), ("upnpConfig", 1)):
            orig[OFFSETS[name]:OFFSETS[name] + size] = enh[OFFSETS[name]:OFFSETS[name] + size]
        assert orig == enh, [hex(i) for i in range(len(orig)) if orig[i] != enh[i]][:8]


CHECKS = [check_loopback_public_address]
