"""P0013: confine the native Demonware lobby transport to the local service (always on)."""

from unicorn import x86_const as x86

BUILDS = ["t6mp_msstore", "t6zm_msstore"]
PATCH = "P0013"


def check_loopback_lobby_transport(ctx):
    machine = ctx.m
    started = []

    def bind(_, mu):
        address = mu.reg_read(x86.UC_X86_REG_RDX)
        started.append(bytes(mu.mem_read(address, 8)))
        mu.reg_write(x86.UC_X86_REG_RAX, 0)

    def getsockname(_, mu):
        # the port the system picked: 50000 (C3 50 in network order)
        mu.mem_write(mu.reg_read(x86.UC_X86_REG_RDX) + 2, b"\xc3\x50")
        mu.reg_write(x86.UC_X86_REG_RAX, 0)

    def returns(value):
        def fn(_, mu):
            mu.reg_write(x86.UC_X86_REG_RAX, value)
        return fn
    # the first lookup starts the service: its lock, CNG, folders, and the lobby listener on loopback
    ctx.imports({
        "kernel32.dll!OutputDebugStringA": (lambda *_: None, 1),
        "kernel32.dll!InitializeCriticalSection": (lambda *_: None, 1),
        "bcrypt.dll!BCryptOpenAlgorithmProvider": (returns(0), 4),
        "bcrypt.dll!BCryptSetProperty": (returns(0), 5),
        "kernel32.dll!MultiByteToWideChar": (returns(0), 6),
        "kernel32.dll!GetModuleFileNameW": (returns(0), 3),
        "ws2_32.dll!socket": (returns(7), 3),
        "ws2_32.dll!bind": (bind, 3),
        "ws2_32.dll!listen": (returns(0), 2),
        "ws2_32.dll!getsockname": (getsockname, 3),
        "kernel32.dll!CreateThread": (returns(0x77), 6),
        "kernel32.dll!CloseHandle": (returns(1), 1),
    })
    setup = None

    def htons(_, mu):
        assert mu.reg_read(x86.UC_X86_REG_ECX) == 50000  # the service's port, not the 3074 the game asked for
        mu.reg_write(x86.UC_X86_REG_EAX, 0x50C3)

    def connect(_, mu):
        assert mu.reg_read(x86.UC_X86_REG_RCX) == 9
        assert mu.reg_read(x86.UC_X86_REG_R8) == 16
        address = mu.reg_read(x86.UC_X86_REG_RDX)
        assert bytes(mu.mem_read(address, 16)) == b"\x02\0\xc3\x50\x7f\0\0\x01" + bytes(8)
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
    # a lookup of a Demonware host starts the service, its listener bound to a port of its own on loopback
    assert started and all(a == b"\x02\0\0\0\x7f\0\0\x01" for a in started), started

    stream = ctx.enhanced("BdConnectStream")
    for address, port, expected in ((loopback, 3074, 1), (loopback, 443, -5), (bytes(4), 3074, -5)):
        actual = machine.call(stream, [9, address, port], setup=setup)[0]
        assert actual == expected, (address, port, actual, machine.imports_called)
        assert any(name.lower() == "ws2_32.dll!connect" for name in machine.imports_called) == (expected == 1)


CHECKS = [check_loopback_lobby_transport]
