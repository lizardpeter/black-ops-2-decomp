"""P0005 (B0025): a flip-model swap chain with at most one frame queued (Store build).

- The swap chain description is the original's with DXGI_SWAP_EFFECT_FLIP_DISCARD, plus
  DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING exactly when IDXGIFactory5 reports tearing support.
- Device creation makes the original's calls, plus IDXGIDevice1::SetMaximumFrameLatency(1) on the DXGI device; when
  the flip-model swap chain can't be created (D3D11 or NVAPI), it is created again with the original description
  and the flip model is off from then on.
- RB_SwapBuffers presents with DXGI_PRESENT_ALLOW_TEARING exactly when tearing is allowed, the sync interval is 0
  and the swap chain isn't in exclusive fullscreen; on the flip model the render targets bound before the present
  are bound again after it (and the references released); everything else is the original's.
- RB_RenderThread resizes the buffers with the description's flags; everything else is the original's.
"""
import struct

from unicorn import x86_const as x86

import test_r_init as ri
import test_rb_backend as rb
from com import ComModel
from test_mouse import _map, read

BUILDS = ["t6mp_msstore"]
PATCH = "P0005"
DESC_SWAP_EFFECT, DESC_FLAGS = 0x3C, 0x40
FLIP_DISCARD, ALLOW_TEARING, ALLOW_MODE_SWITCH, PRESENT_ALLOW_TEARING = 4, 0x800, 2, 0x200
WAITABLE = 0x40  # DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT (P0007)
TEARING = {}


def flags_of(ctx):
    dll = _map(ctx)
    return dll["?r_flipModel@@3_NA"], dll["?r_allowTearing@@3_NA"], dll["s_tearingChecked"]


def tearing_imports(ctx, model_holder):
    """CreateDXGIFactory1 gives a factory whose QueryInterface for IDXGIFactory5 succeeds (TEARING["factory5"]) and
    whose CheckFeatureSupport reports TEARING["support"]."""
    def create_factory(m, mu):
        out = mu.reg_read(x86.UC_X86_REG_RDX)
        model = model_holder["model"]
        m.program_write(out, struct.pack("<Q", model.new("IDXGIFactory", mu)))
        mu.reg_write(x86.UC_X86_REG_RAX, 0)
    ctx.imports({"dxgi.dll!CreateDXGIFactory1": (create_factory, 2)})


def tearing_model(ctx):
    model = ComModel({("IDXGIFactory", 0): "pp", ("IDXGIFactory", 2 * 8): "", ("IUnknown", 28 * 8): "ipi",
                      ("IUnknown", 2 * 8): ""}, dumps={("IDXGIFactory", 0): {0: 16}})

    def qi(call):
        if not TEARING["factory5"]:
            return 0x80004002  # E_NOINTERFACE
        call.out_object(1, "IUnknown")
        return 0

    def check(call):
        call.write(call.arg(1), struct.pack("<i", 1 if TEARING["support"] else 0))
        return 0
    model.results[("IDXGIFactory", 0)] = qi
    model.results[("IUnknown", 28 * 8)] = check
    return model


def check_description_is_flip_model(ctx):
    holder = {}
    tearing_imports(ctx, holder)
    for factory5, support in ((False, False), (True, False), (True, True)):
        TEARING.update(factory5=factory5, support=support)
        for t, v in ri.AA[:6]:
            def setup(w, t=t, v=v):
                ri.aa(t, v)(w)
                holder["model"] = tearing_model(ctx)
                holder["model"].install(w.m)
            args = [bytes([0xCC]) * 0x48, ri.window(0x5150, 2560, 1440)]
            want = ctx.m.call(ctx.original("sub_14071E360"), args, "void", setup)
            got = ctx.m.call(ctx.enhanced("sub_14071E360"), args, "void", setup)
            assert not isinstance(got[0], str), got[0]
            desc = bytearray(want[1][0])
            desc[DESC_SWAP_EFFECT:DESC_SWAP_EFFECT + 4] = struct.pack("<I", FLIP_DISCARD)
            flags = struct.unpack_from("<I", desc, DESC_FLAGS)[0] | (ALLOW_TEARING if support else 0) | WAITABLE
            desc[DESC_FLAGS:DESC_FLAGS + 4] = struct.pack("<I", flags)
            assert got[1][0] == bytes(desc), f"tearing {factory5}/{support}, aa {t}: {got[1][0].hex()}"
            exe = {a: b for a, b in got[2].items() if a < 0x180000000}
            assert exe == {a: b for a, b in want[2].items() if a < 0x180000000}, "the exe's globals differ"


def _device_compare(ctx, inputs, extra_check):
    ctx.imports(ri.DEVICE_IMPORTS)
    fk, ob = ctx.hooks(ri.DEVICE_FAKES), ctx.hooks(ri.DEVICE_SEEN)
    used = 0
    for i, inp in enumerate(inputs):
        want = ctx.m.call(ctx.original("R_CreateDeviceInternal"), inp["args"], "int", inp["setup"], fk, ob)
        if isinstance(want[0], str):
            continue
        got = ctx.m.call(ctx.enhanced("R_CreateDeviceInternal"), inp["args"], "int", inp["setup"], fk, ob)
        assert not isinstance(got[0], str), f"input {i}: {got[0]}"
        used += 1
        extra_check(i, want, got)
    assert used > 100, f"only {used} inputs ran"


def check_device_limits_frame_latency(ctx):
    """The original's inputs (the description is not flip model): the original's calls plus SetMaximumFrameLatency(1)
    on the DXGI device, right after the QueryInterface that gets it."""
    ri.D3D[("IUnknown", 12 * 8)] = "i"

    def extra(i, want, got):
        calls = list(got[3])
        latency = [k for k, c in enumerate(calls) if c[:3] == ("com", "IUnknown", 12 * 8)]
        queried = any(c[:3] == ("com", "ID3D11Device", 0) for c in calls)
        assert len(latency) == queried, f"input {i}: {calls}"
        if queried:
            assert calls[latency[0]][4:] == (1,), f"input {i}: {calls[latency[0]]}"
            assert calls[latency[0] - 1][:3] == ("com", "ID3D11Device", 0), f"input {i}: not after the QueryInterface"
            del calls[latency[0]]
        assert calls == want[3], f"input {i}: calls differ"
        assert got[0] == want[0] and got[1] == want[1], f"input {i}: result or buffers differ"
        exe = lambda w: {a: b for a, b in w.items() if a < 0x180000000}
        assert exe(got[2]) == exe(want[2]), f"input {i}: the exe's globals differ"
    # the description without the waitable flag (0xCC bytes have it): this check is about the device's latency, P0007's
    # acquisition of the waitable object is checked in test_latency.py
    inputs = []
    for inp in ri.CASES[-1]["inputs"]:
        args = list(inp["args"])
        desc = bytearray(args[2])
        desc[DESC_FLAGS] &= ~WAITABLE
        args[2] = bytes(desc)
        inputs.append(dict(inp, args=args))
    _device_compare(ctx, inputs, extra)


def check_device_falls_back_to_blit_model(ctx):
    """A flip-model description the creation refuses (both paths): created again with the original's swap effect and
    without ALLOW_TEARING; r_flipModel and r_allowTearing are then false."""
    ri.D3D[("IUnknown", 12 * 8)] = "i"
    flip_va, tearing_va, _ = flags_of(ctx)
    seen = []

    def refuse_flip(mu, desc_index):
        desc = ri.stack_arg(mu, desc_index)
        effect, flags = struct.unpack("<II", bytes(mu.mem_read(desc + DESC_SWAP_EFFECT, 8)))
        seen.append((effect, flags))
        return effect == FLIP_DISCARD

    def d3d(m, mu):
        if refuse_flip(mu, 7):
            mu.reg_write(x86.UC_X86_REG_RAX, 0x887A0001)
            return
        ri.D3D11CreateDeviceAndSwapChain(m, mu)

    def nv(mu):
        if refuse_flip(mu, 7):
            return -1
        return ri.nv_create(mu)
    ctx.imports(dict(ri.DEVICE_IMPORTS, **{"d3d11.dll!D3D11CreateDeviceAndSwapChain": (d3d, 12)}))
    fk = ctx.hooks(dict(ri.DEVICE_FAKES, NvAPI_D3D11_CreateDeviceAndSwapChain=nv))
    ob = ctx.hooks(ri.DEVICE_SEEN)
    ran = 0
    for i, p in enumerate(ri.device_inputs(60, seed=5)):
        waitable = i % 2  # with the waitable flag the first fallback drops only that (P0007)
        desc = bytearray(0x48)
        struct.pack_into("<II", desc, DESC_SWAP_EFFECT, FLIP_DISCARD,
                         ALLOW_MODE_SWITCH | ALLOW_TEARING | (WAITABLE if waitable else 0))
        if p["factory_hr"] < 0 or p["enum_hr"] < 0 or not p["create_ok"] or not p["swapchain"]:
            continue
        p = dict(p, nv_status=0, nv_level=1)
        seen.clear()
        args = [0x5150, 0, bytes(desc), ri.window_parms(0x5150, False)]
        got = ctx.m.call(ctx.enhanced("R_CreateDeviceInternal"), args, "int", ri.device_setup(p), fk, ob)
        assert not isinstance(got[0], str), f"input {i}: {got[0]}"
        want = [(FLIP_DISCARD, ALLOW_MODE_SWITCH | ALLOW_TEARING | WAITABLE)] if waitable else []
        want += [(FLIP_DISCARD, ALLOW_MODE_SWITCH | ALLOW_TEARING), (0, ALLOW_MODE_SWITCH)]
        assert seen == want, f"input {i}: {seen}"
        assert struct.unpack_from("<II", got[1][0], DESC_SWAP_EFFECT) == (0, ALLOW_MODE_SWITCH), f"input {i}"
        assert read(got[2], flip_va, 1, 1) == b"\0" and read(got[2], tearing_va, 1) == b"\0", f"input {i}: flags"
        ran += 1
    assert ran > 10, f"only {ran} inputs ran"


def _present_setup(ctx, p, flip, tearing, fullscreen):
    flip_va, tearing_va, _ = flags_of(ctx)
    base = rb.make_setup(rb.STORE, p)

    def run(w):
        base(w)
        w.write(flip_va, bytes([flip]))
        w.write(tearing_va, bytes([tearing]))
        model = w.m._com
        model.sigs.update({("ID3D11DeviceContext", 89 * 8): "ipp", ("ID3D11DeviceContext", 33 * 8): "ipp",
                           ("IUnknown", 2 * 8): ""})
        model.dumps[("ID3D11DeviceContext", 33 * 8)] = {1: 8}

        def om_get(call):
            call.mu.mem_write(call.arg(1), struct.pack("<Q", model.new("IUnknown")))
            call.mu.mem_write(call.arg(2), struct.pack("<Q", model.new("IUnknown")))
            return 0
        model.results[("ID3D11DeviceContext", 89 * 8)] = om_get
        model.results[("IDXGISwapChain", 11 * 8)] = lambda call: (
            call.mu.mem_write(call.arg(0), struct.pack("<i", fullscreen)), 0)[1]
    return run


def check_present(ctx):
    ctx.imports(rb.IMPORTS)
    fk, ob = ctx.hooks(rb.STORE_FAKES), ctx.hooks(rb.STORE_SEEN)
    checked = 0
    for k, inp in enumerate(rb.inputs(rb.STORE, 40, seed=99)[1:]):
        p = inp["params"]
        if p["capture"]:
            continue
        for flip in (0, 1):
            for tearing in (0, 1):
                for fullscreen in (0, 1):
                    setup = _present_setup(ctx, p, flip, tearing, fullscreen)
                    want = ctx.m.call(ctx.original("RB_SwapBuffers"), [], "void", setup, fk, ob)
                    got = ctx.m.call(ctx.enhanced("RB_SwapBuffers"), [], "void", setup, fk, ob)
                    assert not isinstance(got[0], str), got[0]
                    sync = 1 if p["vsync"] or p["loading"] else 0
                    calls = list(got[3])
                    present = [c for c in calls if c[:3] == ("com", "IDXGISwapChain", 8 * 8)]
                    assert len(present) == 1, calls
                    expect = PRESENT_ALLOW_TEARING if tearing and sync == 0 and not fullscreen else 0
                    assert present[0][4:] == (sync, expect), f"{flip}/{tearing}/{fullscreen}: {present}"
                    gets = [c for c in calls if c[:3] == ("com", "ID3D11DeviceContext", 89 * 8)]
                    sets = [c for c in calls if c[:3] == ("com", "ID3D11DeviceContext", 33 * 8)]
                    assert len(gets) == len(sets) == flip, calls
                    if flip:
                        i_get, i_present, i_set = (calls.index(gets[0]), calls.index(present[0]),
                                                   calls.index(sets[0]))
                        assert i_get < i_present < i_set, calls
                        rtv, dsv = calls[i_set + 1:i_set + 3]
                        assert rtv[:3] == dsv[:3] == ("com", "IUnknown", 2 * 8), "the views are released"
                    # the rest is the original's: drop the patch's own calls and compare
                    own = {("com", "ID3D11DeviceContext", 89 * 8), ("com", "ID3D11DeviceContext", 33 * 8),
                           ("com", "IUnknown", 2 * 8)}
                    rest = [c for c in calls if c[:3] not in own]
                    if tearing and sync == 0:  # R_Present asks for the fullscreen state first
                        first = next(i for i, c in enumerate(rest) if c[:3] == ("com", "IDXGISwapChain", 11 * 8))
                        del rest[first]
                    orig = [c if c[:3] != ("com", "IDXGISwapChain", 8 * 8) else c[:5] + (expect,) for c in want[3]]
                    orig = [c for c in orig if c[:3] not in own]
                    assert rest == orig, f"{flip}/{tearing}/{fullscreen}: calls differ\n {rest}\n {orig}"
                    checked += 1
    assert checked > 50, checked


def idle_wait_times_out(m, mu):
    """P0008's idle wait, where the original calls Sleep(0): it times out and is logged as that Sleep, so both runs
    see the same script (test_renderthread.py checks the wait itself)."""
    rb.log(m, 0x51EE, 0)
    mu.reg_write(x86.UC_X86_REG_RAX, 0x102)


def check_resize_uses_swap_chain_flags(ctx):
    flip_va, tearing_va, checked_va = flags_of(ctx)
    ctx.imports(dict(rb.RT_IMPORTS, **{"kernel32.dll!WaitForMultipleObjects": (idle_wait_times_out, 4)}))
    fk, ob = ctx.hooks(rb.RT_FAKES), ctx.hooks(rb.RT_SEEN)
    ran = 0
    for k, inp in enumerate(rb.rt_inputs(120, seed=7)):
        p = inp["params"]
        if not (p["resize"] and p["hwnd"] and p["swap"]):
            continue
        for tearing in (0, 1):
            def setup(w, p=p, tearing=tearing):
                rb.rt_setup(p)(w)
                w.write(tearing_va, bytes([tearing]))
                w.write(checked_va, b"\x01")  # asked already: no factory
            want = ctx.m.call(ctx.original("RB_RenderThread"), [0], "void", setup, fk, ob)
            got = ctx.m.call(ctx.enhanced("RB_RenderThread"), [0], "void", setup, fk, ob)
            assert not isinstance(got[0], str), got[0]
            flags = ALLOW_MODE_SWITCH | (ALLOW_TEARING if tearing else 0) | WAITABLE
            resize = lambda c: c[:3] == ("com", "IDXGISwapChain", 13 * 8)
            orig = [c[:8] + (flags,) if resize(c) else c for c in want[3]]
            assert got[3] == orig, f"input {k}: calls differ"
            ran += any(resize(c) for c in got[3])
    assert ran > 10, ran


CHECKS = [check_description_is_flip_model, check_device_limits_frame_latency, check_device_falls_back_to_blit_model,
          check_present, check_resize_uses_swap_chain_flags]
