"""Exercise the opt-in Custom Games transition through the shipped DLL."""
import struct

from harness import Writer
from unicorn import x86_const as x86

BUILDS = ["t6mp_msstore"]
PATCH = "P0012"


def check_custom_games_opens_native_lobby_once(ctx):
    machine = ctx.m
    state_pointer = 0x14265E878
    stats_schema_pointer = 0x1425A1BF0
    state = 0x30010000
    stack = state + 0x100
    lua_ready = False
    args = ["t6mp.exe", "--custom-games-extra"]
    calls = []

    def parse_command_line(_, mu):
        argc = mu.reg_read(x86.UC_X86_REG_RDX)
        scratch = Writer(machine)
        pointers = [scratch.alloc((arg + "\0").encode("utf-16le")) for arg in args]
        mu.mem_write(argc, struct.pack("<i", len(pointers)))
        mu.reg_write(x86.UC_X86_REG_RAX, scratch.alloc(struct.pack(f"<{len(pointers)}Q", *pointers)))

    def get_field(mu):
        field = machine.read_cstr(mu.reg_read(x86.UC_X86_REG_R8))
        top, = struct.unpack("<Q", mu.mem_read(state + 0x48, 8))
        if field in (b"CoD", b"Engine"):
            value_type = 5 if lua_ready else 0
        else:
            value_type = 9 if field == b"SwitchToPrivateLobby" else 10
            calls.append(field)
        mu.mem_write(top, struct.pack("<i12x", value_type))
        mu.mem_write(state + 0x48, struct.pack("<Q", top + 16))

    def protected_call(mu):
        top, = struct.unpack("<Q", mu.mem_read(state + 0x48, 8))
        calls.append(struct.unpack("<i4xi4x", mu.mem_read(top - 16, 16)))
        return 0

    ctx.imports({
        "kernel32.dll!GetCommandLineW": (lambda _, mu: mu.reg_write(x86.UC_X86_REG_RAX, 0), 0),
        "shell32.dll!CommandLineToArgvW": (parse_command_line, 2),
        "kernel32.dll!LocalFree": (lambda *_: None, 1),
        "kernel32.dll!EnterCriticalSection": (lambda *_: None, 1),
        "kernel32.dll!LeaveCriticalSection": (lambda *_: None, 1),
    })
    functions = {
        "StoreHksGetField": get_field,
        "StoreHksProtectedCall": protected_call,
        "StoreLuiTransitionToMenu": lambda _: 0,
        "Cbuf_InsertText": lambda _: 0,
        "StoreLiveStatsAreFetched": lambda _: 0,
        "StoreLiveStatsReset": lambda _: 0,
    }
    observed = {
        ctx.original("StoreLuiTransitionToMenu"): "ibsiqqb",
        ctx.original("Cbuf_InsertText"): "is",
        ctx.original("StoreLiveStatsAreFetched"): "ii",
        ctx.original("StoreLiveStatsReset"): "ibi",
    }
    fakes = {ctx.original(name): callback for name, callback in functions.items()}

    def setup(writer):
        writer.ptr(state_pointer, state)
        writer.ptr(stats_schema_pointer, stack + 0x100)
        writer.write(state + 0x48, struct.pack("<QQ", stack, stack))

    def call(name, **kwargs):
        result, _, writes, observed_calls = machine.call(ctx.enhanced(name), [], "void", **kwargs)
        assert result is None, result
        for address, data in writes.items():
            machine.program_write(address, data)
        return observed_calls

    def tick():
        return call("CustomGames_UpdateStartup", setup=setup, fakes=fakes, observe=observed)

    activate = ctx.original("StoreSetControllerInUse")
    redirect = ctx.enhanced("CL_MapLoading_SetupDevmapParty")

    devmap_fakes = {
        ctx.original(name): lambda _: 0 for name in (
            "StoreSetControllerInUse", "sub_1406F52E0", "sub_1405A68D0", "sub_1401B3900"
        )
    }

    def devmap_calls(mode):
        result, _, _, observed_calls = machine.call(
            redirect, [b"mp_la\0"], "void",
            setup=lambda writer: writer.u32(0x143ED9A14, mode),
            fakes=devmap_fakes, observe={activate: "ib"},
        )
        assert result is None, result
        return observed_calls

    def read_options():
        call("CustomGames_ReadStartupOptions")

    read_options()
    assert tick() == []
    assert calls == []
    assert devmap_calls(9) == [(activate, 0, 1)]

    args[1] = "--custom-games"
    read_options()
    assert tick() == []
    assert calls == []
    lua_ready = True
    insert = ctx.original("Cbuf_InsertText")
    transition = ctx.original("StoreLuiTransitionToMenu")
    assert tick() == [(insert, 0, b"setclientbeingusedandprimaryandactive")]
    assert tick() == [(ctx.original("StoreLiveStatsAreFetched"), 0, 0),
                      (ctx.original("StoreLiveStatsReset"), 0, 0, 5),
                      (transition, 0, 0, b"PrivateOnlineGameLobby", 9, 0, 0, 0),
                      (insert, 0, b"setclientbeingusedandprimaryandactive; xstartpartyhost")]
    assert calls == [b"SwitchToPrivateLobby", b"SwitchToPrivateLobby", (3, 0),
                     b"SessionModeSetOnlineGame", (1, 0), b"SessionModeSetOffline", (1, 1)]
    assert tick() == []
    assert devmap_calls(9) == []
    assert devmap_calls(1) == [(activate, 0, 1)]


def check_devmap_fallback_matches_original(ctx):
    mode_flags = 0x143ED9A14
    original = ctx.original("CL_MapLoading_SetupDevmapParty")
    reconstruction = ctx.enhanced("CL_MapLoading_SetupDevmapParty")
    observed = {
        ctx.original("StoreSetControllerInUse"): "ib",
        ctx.original("sub_1406F52E0"): "ib",
        ctx.original("sub_1405A68D0"): "i",
        ctx.original("Com_Error"): "is",
        ctx.original("sub_1401B3900"): "s",
    }
    fakes = {function: lambda _: 0 for function in observed}
    for mode, condition, backup in ((0, 0, 0), (2, 0, 0), (4, 0, 0), (0, 4, 0), (9, 0, 1)):
        def setup(writer):
            writer.u32(mode_flags, mode)
            writer.u32(0x143ED8224, condition)
            writer.u32(0x141EF91E8, 1)
            writer.u8(0x141EF8675, backup)

        args = [b"mp_la\0"]
        stock = ctx.m.call(original, args, "void", setup=setup, fakes=fakes, observe=observed)
        actual = ctx.m.call(reconstruction, args, "void", setup=setup, fakes=fakes, observe=observed)
        assert stock[0] is None, stock[0]
        assert actual == stock, (mode, condition, backup, stock, actual)


CHECKS = [check_devmap_fallback_matches_original, check_custom_games_opens_native_lobby_once]
