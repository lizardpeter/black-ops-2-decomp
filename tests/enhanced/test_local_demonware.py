"""P0013: confine the native Demonware lobby transport to the local backend."""

from unicorn import x86_const as x86

BUILDS = ["t6mp_msstore", "t6zm_msstore"]
PATCH = "P0013"


def check_loopback_lobby_transport(ctx):
    machine = ctx.m
    enabled = ctx.enhanced("LocalDemonware_IsEnabled")
    enable = ctx.enhanced("LocalDemonware_Enable")
    ctx.imports({"kernel32.dll!OutputDebugStringA": (lambda *_: None, 1)})
    result, _, enable_writes, _ = machine.call(enable, [], "void")
    assert result is None and enable_writes

    def setup(writer):
        for address, data in enable_writes.items():
            writer.write(address, data)

    assert machine.call(enabled, [], "bool")[0] == 0
    assert machine.call(enabled, [], "bool", setup=setup)[0] == 1

    def htons(_, mu):
        assert mu.reg_read(x86.UC_X86_REG_ECX) == 3074
        mu.reg_write(x86.UC_X86_REG_EAX, 0x020C)

    def connect(_, mu):
        assert mu.reg_read(x86.UC_X86_REG_RCX) == 9
        assert mu.reg_read(x86.UC_X86_REG_R8) == 16
        address = mu.reg_read(x86.UC_X86_REG_RDX)
        assert bytes(mu.mem_read(address, 16)) == b"\x02\0\x0c\x02\x7f\0\0\x01" + bytes(8)
        mu.reg_write(x86.UC_X86_REG_EAX, 0)

    ctx.imports({
        "ws2_32.dll!htons": (htons, 1),
        "ws2_32.dll!connect": (connect, 3),
    })
    resolver = ctx.enhanced("BdGetHostByName")
    seed = bytes([0xCC]) * 132
    loopback = b"\x7f\0\0\x01"
    for host in (b"auth3.prod.demonware.net\0", b"auth3.dev.demonware.net\0",
                 b"lsg.2410.prod.demonware.net\0", b"lsg.2405.dev.demonware.net\0"):
        result, buffers, _, _ = machine.call(resolver, [host, seed, 1], setup=setup)
        assert result == 1 and buffers[1] == loopback + seed[4:]
        assert machine.call(resolver, [host, seed, 0], setup=setup)[0] == 0
    result, buffers, _, _ = machine.call(
        resolver, [b"telemetry.demonware.net\0", seed, 1], setup=setup
    )
    assert result == 0 and buffers[1] == seed

    stream = ctx.enhanced("BdConnectStream")
    for address, port, expected in ((loopback, 3074, 1), (loopback, 443, -5), (bytes(4), 3074, -5)):
        actual = machine.call(stream, [9, address, port], setup=setup)[0]
        assert actual == expected, (address, port, actual, machine.imports_called)
        assert any(name.lower() == "ws2_32.dll!connect" for name in machine.imports_called) == (expected == 1)


CHECKS = [check_loopback_lobby_transport]
