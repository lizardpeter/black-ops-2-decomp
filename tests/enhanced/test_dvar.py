"""P0018: cg_fovScale is registered without DVAR_CHEAT (Store build).

_Dvar_RegisterFloat passes Dvar_RegisterNew (a new dvar) or Dvar_Reregister (one already made, e.g. by a config file)
the original's flags without 0x80 for cg_fovScale, in any letter case, and the original's flags for every other name.
Everything else the call does (the lookup, the name, type, value, domain, description and result) is the original's.
"""
import struct

from unicorn import x86_const as x86

PATCH = "P0018"
BUILDS = ["t6mp_msstore"]
FIND, NEW, REREGISTER = 0x14046B8B0, 0x14046C1A0, 0x14046BF10
DVAR_CHEAT = 0x80
STATE = {}


def regs(mu, *rs):
    return [mu.reg_read(r) for r in rs]


def record(kind):
    def fake(mu):
        if kind == "find":
            STATE["calls"].append(("find", mu.mem_read(mu.reg_read(x86.UC_X86_REG_RCX), 32).split(b"\0")[0]))
            return 0x31000100 if STATE["exists"] else 0
        rcx, rdx, r8, r9 = regs(mu, x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)
        sp = mu.reg_read(x86.UC_X86_REG_RSP)
        stack = struct.unpack("<3Q", bytes(mu.mem_read(sp + 0x28, 24)))
        if kind == "new":  # (name, type, flags, &value, &domain, desc)
            name, typ, flags, pv, pd, desc = rcx, rdx, r8, r9, stack[0], stack[1]
        else:  # (dvar, name, type, flags, &value, &domain, desc)
            name, typ, flags, pv, pd, desc = rdx, r8, r9, stack[0], stack[1], stack[2]
        value = bytes(mu.mem_read(pv, 4))
        domain = bytes(mu.mem_read(pd, 8))
        STATE["calls"].append((kind, bytes(mu.mem_read(name, 32)).split(b"\0")[0], typ & 0xFFFFFFFF,
                               flags & 0xFFFFFFFF, value, domain, desc))
        return 0x31000200 if kind == "new" else 0
    return fake


FAKES = {FIND: record("find"), NEW: record("new"), REREGISTER: record("reregister")}


def run(ctx, va, name, flags, exists):
    STATE.update(calls=[], exists=exists)
    args = [name + b"\0", ("f", 1.0), ("f", 0.2), ("f", 2.0), flags, b"\0"]
    got = ctx.m.call(va, args, "uint64", fakes=FAKES)
    assert not isinstance(got[0], str), f"{name} {flags:#x}: {got[0]}"
    # the name and description pointers differ between runs (each call gets its own buffers): compare their text
    return got[0] if got[0] in (0x31000100, 0x31000200) else "other", [c[:6] for c in STATE["calls"]]


def check_cg_fovscale_loses_only_the_cheat_flag(ctx):
    orig, enh = ctx.original("_Dvar_RegisterFloat"), ctx.enhanced("_Dvar_RegisterFloat")
    for name in (b"cg_fovScale", b"CG_FOVSCALE", b"cg_fovscale"):
        for flags in (0x80, 0x81, 0x4080, 0, 0x4000, 0xFFFFFFFF):
            for exists in (0, 1):
                want_r, want = run(ctx, orig, name, flags, exists)
                got_r, got = run(ctx, enh, name, flags, exists)
                assert got_r == want_r, f"{name} {flags:#x} {exists}: result {got_r} != {want_r}"
                assert len(got) == len(want) == 2, f"{name}: calls {got} / {want}"
                assert got[0] == want[0], f"{name}: lookup {got[0]} != {want[0]}"
                w = list(want[1])
                w[3] &= ~DVAR_CHEAT
                assert list(got[1]) == w, f"{name} {flags:#x} {exists}: {got[1]} != {tuple(w)}"


def check_other_dvars_are_unchanged(ctx):
    orig, enh = ctx.original("_Dvar_RegisterFloat"), ctx.enhanced("_Dvar_RegisterFloat")
    for name in (b"cg_fov", b"cg_fovScaleX", b"cg_fovScal", b"", b"winvoice_mic_scaler"):
        for flags in (0x80, 0, 0x4081):
            for exists in (0, 1):
                assert run(ctx, enh, name, flags, exists) == run(ctx, orig, name, flags, exists), (name, flags)


CHECKS = [check_cg_fovscale_loses_only_the_cheat_flag, check_other_dvars_are_unchanged]
