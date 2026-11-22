"""P0007: the frame-latency waitable object, waited on before the frame reads input (Store build).

- A swap chain created with DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT gets its waitable object
  (IDXGISwapChain2: SetMaximumFrameLatency(1), GetFrameLatencyWaitableObject) and the counts start again; without the
  flag, or without IDXGISwapChain2, there is none.
- Each successful present on it is counted (RB_SwapBuffers).
- R_WaitForFrameLatency waits once per counted present not yet waited for, each up to two refresh periods (at least
  50 ms, at most 1 s); a wait that times out marks the next frame to drain every pending signal instead of counting.
- The frame waits after the server and before the client's second event loop (keys) and its screen (mouse).
"""
import struct

from unicorn import x86_const as x86

import test_com_frame as cf
import test_framepace as fp
import test_r_init as ri
import test_rb_backend as rb
from test_mouse import _map, read

BUILDS = ["t6mp_msstore"]
PATCH = "P0007"
WAITABLE_HANDLE = 0x7A17
VID_CONFIG = 0x14282E5C0
W = {}


def dll_vars(ctx):
    d = _map(ctx)
    return {k: d[k] for k in ("s_frameLatencyWaitable", "s_presentCount", "s_waitedPresentCount",
                              "s_resyncFrameLatency")}


def wait_model(m, mu):
    handle, timeout = cf.arg(mu, 0), cf.arg(mu, 1) & 0xFFFFFFFF
    W["waits"].append((handle, timeout))
    cf.result(mu, W["results"].pop(0) if W["results"] else 0x102)  # WAIT_TIMEOUT once the script runs out


def state_setup(ctx, waitable, presented, waited, resync, hz, results):
    v = dll_vars(ctx)

    def run(w):
        W.update(waits=[], results=list(results))
        w.write(v["s_frameLatencyWaitable"], struct.pack("<Q", waitable))
        w.write(v["s_presentCount"], struct.pack("<i", presented))
        w.write(v["s_waitedPresentCount"], struct.pack("<i", waited))
        w.write(v["s_resyncFrameLatency"], bytes([resync]))
        w.write(VID_CONFIG + 0x14, struct.pack("<I", hz))
    return run


def check_wait_pairs_presents(ctx):
    ctx.imports({"kernel32.dll!WaitForSingleObject": (wait_model, 2)})
    v = dll_vars(ctx)
    for hz, timeout in ((60, 50), (144, 50), (20, 100), (0, 50), (1, 1000), (3, 667)):
        for presented, waited in ((5, 5), (6, 5), (8, 5), (0, 0x7FFFFFFF), (-0x80000000, 0x7FFFFFFF)):
            for resync in (0, 1):
                for results in ([0] * 8, [0, 0x102], [0x102], [0, 0, 0, 0x102]):
                    for waitable in (WAITABLE_HANDLE, 0):
                        got = ctx.m.call(ctx.enhanced("R_WaitForFrameLatency"), [], "void",
                                         state_setup(ctx, waitable, presented, waited, resync, hz, results))
                        assert not isinstance(got[0], str), got[0]
                        rd = lambda name, fmt, d: struct.unpack(fmt, read(got[2], v[name], struct.calcsize(fmt),
                                                                           struct.pack(fmt, d)))[0]
                        waited_after = rd("s_waitedPresentCount", "<i", waited)
                        resync_after = rd("s_resyncFrameLatency", "<B", resync)
                        label = f"{hz} Hz, {presented}/{waited}, resync {resync}, {results}, waitable {waitable:#x}"
                        if not waitable:
                            assert W["waits"] == [] and waited_after == waited, label
                            continue
                        assert all(h == WAITABLE_HANDLE for h, _ in W["waits"]), label
                        if resync:
                            drained = next(i for i, r in enumerate(results + [0x102]) if r != 0) + 1
                            assert W["waits"] == [(WAITABLE_HANDLE, 0)] * drained, label
                            assert waited_after == presented and not resync_after, label
                            continue
                        due = (presented - waited) & 0xFFFFFFFF
                        if due > 100:
                            due = None  # a wrapped count: checked by the first timeout below
                        ok = 0
                        for r in results + [0x102] * 8:
                            if ok == due or r != 0:
                                break
                            ok += 1
                        timed_out = due is None or ok < due
                        n = ok + 1 if timed_out else ok
                        if due is not None:
                            assert W["waits"] == [(WAITABLE_HANDLE, timeout)] * n, f"{label}: {W['waits']}"
                        assert waited_after == presented, label
                        assert resync_after == (1 if timed_out else 0), label


def check_swap_chain_waitable_acquired(ctx):
    """R_CreateDeviceInternal with each description: the waitable flag gets IDXGISwapChain2's object, latency 1."""
    ri.D3D[("IUnknown", 12 * 8)] = "i"
    ri.D3D[("IDXGISwapChain", 0)] = "pp"
    ri.DUMPS[("IDXGISwapChain", 0)] = {0: 16}
    ri.D3D[("IUnknown", 31 * 8)] = "i"
    ri.D3D[("IUnknown", 33 * 8)] = ""
    ctx.imports(dict(ri.DEVICE_IMPORTS, **{"kernel32.dll!CloseHandle": (lambda m, mu: cf.result(mu, 1), 1)}))
    fk, ob = ctx.hooks(ri.DEVICE_FAKES), ctx.hooks(ri.DEVICE_SEEN)
    v = dll_vars(ctx)
    ran = 0
    for i, p in enumerate(ri.device_inputs(40, seed=11)):
        if p["factory_hr"] < 0 or p["enum_hr"] < 0 or not p["create_ok"] or not p["swapchain"]:
            continue
        for flags, qi_ok in ((0x802 | 0x40, 1), (0x802, 1), (0x802 | 0x40, 0)):
            desc = bytearray(0x48)
            struct.pack_into("<II", desc, 0x3C, 4, flags)
            base = ri.device_setup(p)

            def setup(w, qi_ok=qi_ok):
                base(w)
                model = w.m._com

                def qi(call):
                    if not qi_ok:
                        return 0x80004002
                    call.out_object(1, "IUnknown")
                    return 0
                model.results[("IDXGISwapChain", 0)] = qi
                model.results[("IUnknown", 33 * 8)] = lambda call: WAITABLE_HANDLE
                w.write(v["s_frameLatencyWaitable"], struct.pack("<Q", 0))
                w.write(v["s_presentCount"], struct.pack("<ii", 9, 4))
            got = ctx.m.call(ctx.enhanced("R_CreateDeviceInternal"), [0x5150, 0, bytes(desc),
                                                                      ri.window_parms(0x5150, False)], "int", setup, fk,
                             ob)
            assert not isinstance(got[0], str), f"input {i}: {got[0]}"
            if got[0] in (-1, 0xFFFFFFFF):
                continue  # the NVAPI device was refused: the function returns before the swap chain matters
            calls = [c for c in got[3] if c[0] == "com" and c[1] in ("IDXGISwapChain", "IUnknown") and
                     c[2] in (0, 31 * 8, 33 * 8)]
            handle = struct.unpack("<Q", read(got[2], v["s_frameLatencyWaitable"], 8))[0]
            counts = struct.unpack("<ii", read(got[2], v["s_presentCount"], 8, struct.pack("<ii", 9, 4)))
            assert counts == (0, 0), f"input {i}: the counts don't start again: {counts}"
            if flags & 0x40 and qi_ok:
                kinds = [(c[1], c[2]) for c in calls]
                assert kinds == [("IDXGISwapChain", 0), ("IUnknown", 31 * 8), ("IUnknown", 33 * 8)], f"{i}: {calls}"
                assert calls[0][4][0] == "bytes" and calls[0][4][1][:4] == bytes.fromhex("c42abea8"), calls[0]
                assert calls[1][4:] == (1,), calls[1]
                assert handle == WAITABLE_HANDLE, f"input {i}: {handle:#x}"
            else:
                assert handle == 0, f"input {i}, flags {flags:#x}, qi {qi_ok}: {handle:#x}"
                assert all(c[2] == 0 for c in calls) and len(calls) == (1 if flags & 0x40 else 0), calls
            ran += 1
    assert ran > 20, ran


def check_presents_counted(ctx):
    v = dll_vars(ctx)
    ctx.imports(rb.IMPORTS)
    fk, ob = ctx.hooks(rb.STORE_FAKES), ctx.hooks(rb.STORE_SEEN)
    ran = 0
    for inp in rb.inputs(rb.STORE, 30, seed=3)[1:]:
        p = inp["params"]
        for waitable in (WAITABLE_HANDLE, 0):
            for hr in (0, 0x887A0005 - (1 << 32), 1):
                base = inp["setup"]

                def setup(w, waitable=waitable, hr=hr):
                    base(w)
                    w.m._com.results[("IDXGISwapChain", 8 * 8)] = lambda call: hr
                    w.write(v["s_frameLatencyWaitable"], struct.pack("<Q", waitable))
                    w.write(v["s_presentCount"], struct.pack("<i", 41))
                got = ctx.m.call(ctx.enhanced("RB_SwapBuffers"), [], "void", setup, fk, ob)
                assert not isinstance(got[0], str), got[0]
                count = struct.unpack("<i", read(got[2], v["s_presentCount"], 4, struct.pack("<i", 41)))[0]
                presented = not p["capture"] and waitable and hr >= 0
                assert count == 41 + (1 if presented else 0), f"capture {p['capture']}, {waitable:#x}, {hr}: {count}"
                ran += 1
    assert ran > 50, ran


def check_frame_waits_before_the_client_reads_input(ctx):
    v = dll_vars(ctx)
    order = []

    def wait(m, mu):
        handle = cf.arg(mu, 0)
        if handle == WAITABLE_HANDLE:
            order.append("latency")
            cf.result(mu, 0)
            return
        fp.wait(m, mu)
    ctx.imports(dict(fp.IMPORTS, **{"kernel32.dll!WaitForSingleObject": (wait, 2)}))
    fakes = dict(fp.FAKES, Com_EventLoop=lambda mu: (order.append("events"), fp.event_loop(mu))[0],
                 IN_Frame=lambda mu: order.append("IN_Frame"),
                 SCR_UpdateScreen=lambda mu: order.append("screen"),
                 sub_1405B5DB0=lambda mu: order.append("client"))
    fk, ob = ctx.hooks(fakes), ctx.hooks(cf.SEEN_ALL)
    for dedicated in (0, 1):
        for presented in (3, 5):
            order.clear()
            p = fp.base_params(144, 4999, 5_000_777)
            p.update(dedicated=dedicated, notFirst=1, inputUpdated=0)
            base = fp.setup_for(ctx, p, 0, 1, 1)

            def setup(w):
                base(w)
                w.write(v["s_frameLatencyWaitable"], struct.pack("<Q", WAITABLE_HANDLE))
                w.write(v["s_presentCount"], struct.pack("<ii", presented, 3))
            got = ctx.m.call(ctx.enhanced("Com_Frame_Try_Block_Function"), [], "void", setup, fk, ob)
            assert not isinstance(got[0], str), got[0]
            if dedicated:
                assert "latency" not in order and "screen" not in order, order
                continue
            waits = order.count("latency")
            assert waits == presented - 3, order
            last_events = len(order) - 1 - order[::-1].index("events")
            first_latency = order.index("latency") if waits else last_events
            assert order.index("IN_Frame") < first_latency <= last_events < order.index("screen"), order
            assert order[last_events - 1] in ("latency", "IN_Frame", "client"), order


CHECKS = [check_wait_pairs_presents, check_swap_chain_waitable_acquired, check_presents_counted,
          check_frame_waits_before_the_client_reads_input]
