"""P0008 (B0024): the render thread waits for work instead of spinning (Store build).

Both the original and the enhanced RB_RenderThread run against one model of time and events: frames, the two other
requests and a buffer resize arrive at scripted times; a zero-timeout wait sees an event's current state (taking an
auto-reset event's signal), Sleep(0) lets 50 us pass, WaitForMultipleObjects waits for the first event or its
timeout. Checked:
- the same work in the same order: frames drawn, requests answered, resizes;
- each piece of work starts within a millisecond of the original's;
- the enhanced thread never calls Sleep(0), and between work it waits in WaitForMultipleObjects on the frame's event
  and the two requests, a millisecond at a time.
"""
import random
import struct

from unicorn import x86_const as x86

import test_rb_backend as rb
from harness import OBJECTS

BUILDS = ["t6mp_msstore"]
PATCH = "P0008"
E = {}
EV = rb.EV
BACKEND1, E8, E48 = EV["backend1"], EV["e8"], EV["e48"]
AUTO = {BACKEND1}


def arg(mu, i):
    return rb.arg(mu, i)


def result(mu, v):
    mu.reg_write(x86.UC_X86_REG_RAX, v & 0xFFFFFFFF)


def advance(m, us):
    E["t"] += us
    while E["schedule"] and E["schedule"][0][0] <= E["t"]:
        _, what = E["schedule"].pop(0)
        if what == "frame":
            E["set"].add(BACKEND1)
            m.program_write(rb.RT_ADDR["smpData"], struct.pack("<Q", rb.RT_DATA))
        elif what == "resize":
            m.program_write(rb.RT_ADDR["dx"] + 0x486, b"\x01")
        else:
            E["set"].add(what)
    if E["t"] > E["horizon"]:
        m.end_call()


def take(handle):
    if handle in E["set"]:
        if handle in AUTO:
            E["set"].discard(handle)
        return True
    return False


def wait_single(m, mu):
    handle, timeout = arg(mu, 0), arg(mu, 1) & 0xFFFFFFFF
    advance(m, 1)
    if timeout == 0:
        result(mu, 0 if take(handle) else 0x102)
        return
    while not take(handle) and E["t"] <= E["horizon"]:  # INFINITE: until it is set
        advance(m, 100)
    result(mu, 0)


def wait_multiple(m, mu):
    count, handles, wait_all, timeout = arg(mu, 0), arg(mu, 1), arg(mu, 2), arg(mu, 3) & 0xFFFFFFFF
    hs = list(struct.unpack(f"<{count}Q", bytes(mu.mem_read(handles, 8 * count))))
    E["multi"].append((tuple(hs), wait_all, timeout))
    end = E["t"] + timeout * 1000
    while True:
        for i, h in enumerate(hs):
            if take(h):
                result(mu, i)
                return
        if E["t"] >= end:
            result(mu, 0x102)
            return
        advance(m, 10)


def sleep(m, mu):
    E["sleeps"] += 1
    advance(m, 50 if arg(mu, 0) == 0 else 1000 * (arg(mu, 0) & 0xFFFFFFFF))


def reset(m, mu):
    E["set"].discard(arg(mu, 0))
    result(mu, 1)


def set_event(m, mu):
    if arg(mu, 0) in (BACKEND1, E8, E48):
        E["set"].add(arg(mu, 0))
    result(mu, 1)


IMPORTS = dict(rb.RT_IMPORTS, **{"kernel32.dll!WaitForSingleObject": (wait_single, 2),
                                 "kernel32.dll!WaitForMultipleObjects": (wait_multiple, 4),
                                 "kernel32.dll!Sleep": (sleep, 1), "kernel32.dll!ResetEvent": (reset, 1),
                                 "kernel32.dll!SetEvent": (set_event, 1)})


def work(name):
    def run(mu):
        E["work"].append((name, E["t"]))
    return run


WORK = ("RB_RenderThreadFrame", "sub_140721570", "sub_140721860", "sub_140753660")
FAKES = dict(rb.RT_FAKES, **{n: work(n) for n in WORK})


def setup_for(ctx, p, schedule, horizon):
    base = rb.rt_setup(p)
    from test_mouse import _map
    checked = _map(ctx)["s_tearingChecked"]

    def run(w):
        base(w)
        w.write(checked, b"\x01")  # the enhanced description asked DXGI already (P0005's check covers that)
        E.update(t=0, set=set(), schedule=sorted(schedule), horizon=horizon, sleeps=0, multi=[], work=[])
        w.write(rb.RT_ADDR["smpData"], struct.pack("<Q", 0))
        w.write(rb.RT_ADDR["dx"] + 0x486, b"\x00")
        w.write(rb.RT_ADDR["r_glob"] + 0x10, struct.pack("<i", 0))  # no remote screen updates
    return run


def check_waits_instead_of_spinning(ctx):
    ctx.imports(IMPORTS)
    fk, ob = ctx.hooks(FAKES), ctx.hooks(rb.RT_SEEN)
    r = random.Random(0xB0024)
    base_inputs = rb.rt_inputs(40, seed=21)
    checked = 0
    for k, inp in enumerate(base_inputs):
        p = dict(inp["params"], setjmp=[], hwnd=True, swap=True, busy=0)
        schedule = []
        t = 0
        for _ in range(r.randrange(2, 7)):
            t += r.choice([300, 1500, 4000, 7000, 16000])
            schedule.append((t, r.choice(["frame", "frame", "frame", E8, E48, "resize"])))
        horizon = t + 20000
        want = ctx.m.call(ctx.original("RB_RenderThread"), [0], "void", setup_for(ctx, p, schedule, horizon), fk, ob)
        want_work, orig_sleeps = list(E["work"]), E["sleeps"]
        assert not isinstance(want[0], str), want[0]
        got = ctx.m.call(ctx.enhanced("RB_RenderThread"), [0], "void", setup_for(ctx, p, schedule, horizon), fk, ob)
        assert not isinstance(got[0], str), got[0]
        names = lambda w: [n for n, _ in w]
        assert names(E["work"]) == names(want_work), f"input {k}: work differs\n {want_work}\n {E['work']}"
        late = [b - a for (_, a), (_, b) in zip(want_work, E["work"])]
        assert all(d <= 1000 for d in late), f"input {k}: work later than the original's by {late} us"
        assert E["sleeps"] == 0, f"input {k}: Sleep called {E['sleeps']} times"
        assert orig_sleeps > 0, f"input {k}: the original never idled (the script is too busy)"
        assert all(m == ((BACKEND1, E8, E48), 0, 1) for m in E["multi"]), f"input {k}: {E['multi'][:3]}"
        checked += 1
    assert checked > 30, checked


CHECKS = [check_waits_instead_of_spinning]
