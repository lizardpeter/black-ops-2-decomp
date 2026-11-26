"""P0014: confine multiplayer permission to the connected synthetic profile."""

import symbols

BUILDS = ["t6mp_msstore", "t6zm_msstore"]
PATCH = "P0014"


def check_connected_local_profile_has_multiplayer_privilege(ctx):
    machine = ctx.m
    ctx.imports({"kernel32.dll!OutputDebugStringA": (lambda *_: None, 1)})
    _, _, enable_writes, _ = machine.call(ctx.enhanced("LocalDemonware_Enable"), [], "void")
    query = ctx.original("GdkQueryApi")
    failed = 0x80004002
    check = ctx.enhanced("GdkUserCheckPrivilege")
    state, _ = symbols.resolve(ctx.build, "store_primaryDemonwareState")
    fakes = {query: lambda _: failed}
    observe = {query: "qqq"}
    zombies = ctx.build == "t6zm_msstore"
    if zombies:
        register = ctx.original("_Dvar_RegisterBool")
        fakes[register] = lambda _: 1
        observe[register] = "sii"

    for local, connection_state, user, options, privilege, grant in (
        (False, 6, 0, 0, 254, False),
        (True, 4, 0, 0, 254, False),
        (True, 6, 123, 0, 254, False),
        (True, 6, 0, 2, 254, False),
        (True, 6, 0, 0, 252, False),
        (True, 6, 0, 0, 254, True),
    ):
        def setup(writer):
            if local:
                for address, data in enable_writes.items():
                    writer.write(address, data)
            writer.u32(state, connection_state)

        result, buffers, _, calls = machine.call(
            check, [user, options, privilege, b"\0", b"\xff" * 4], "uint", setup=setup,
            fakes=fakes, observe=observe,
        )
        assert result == (0 if grant else failed), (connection_state, privilege, result)
        assert buffers == ([b"\1", bytes(4)] if grant else [b"\0", b"\xff" * 4])
        if grant and zombies:
            assert calls == [(register, b"ui_showNewestLeaderboards", 0, 0)], calls
        elif grant:
            assert not calls
        else:
            assert len(calls) == 1 and calls[0][0] == query, calls


CHECKS = [check_connected_local_profile_has_multiplayer_privilege]
