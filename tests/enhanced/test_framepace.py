"""P0006 (B0026): the frame cap to the microsecond (Store build).

- The wait ends at the first event loop at or after its deadline on the performance counter, once the game's
  millisecond clock has moved; it never sleeps past the deadline (the timer is given at most the time left less a
  half-millisecond spin); the event loop runs at least once a millisecond while waiting.
- The next deadline is one period (1 / com_maxfps, capped by the server's limit) after this one was due, or after now
  when this frame is more than a period late; no cap (0) means no deadline.
- After the wait the frame is the original's: with the same msec, the same calls and the same writes.
"""
import struct

from unicorn import x86_const as x86

import test_com_frame as cf
from test_mouse import _map, read

BUILDS = ["t6mp_msstore"]
PATCH = "P0006"
QPC_HZ = 10_000_000
T = {}


def us_now():
    return T["us"]


def advance(us):
    T["us"] += us


# the clock, in microseconds: timeGetTime is its milliseconds (plus the base offset), the counter its 100 ns ticks
def time_get_time(m, mu):
    cf.result(mu, (cf.F["p"]["timeBase"] + T["us"] // 1000) & 0xFFFFFFFF)


def qpc(m, mu):
    advance(1)  # every read takes a microsecond, so spins end
    mu.mem_write(cf.arg(mu, 0), struct.pack("<q", T["us"] * 10))
    cf.result(mu, 1)


def qpf(m, mu):
    mu.mem_write(cf.arg(mu, 0), struct.pack("<q", QPC_HZ))
    cf.result(mu, 1)


def create_timer(m, mu):
    T["log"].append(("create", cf.arg(mu, 2)))
    cf.result(mu, 0x7173 if T["timer"] else 0)


def set_timer(m, mu):
    due = struct.unpack("<q", bytes(mu.mem_read(cf.arg(mu, 1), 8)))[0]
    T["due"] = -due // 10  # relative, 100 ns -> us
    T["log"].append(("set", T["us"], T["due"]))
    cf.result(mu, 1)


def wait(m, mu):
    handle = cf.arg(mu, 0)
    if handle == 0x7173:
        T["log"].append(("wait", T["us"]))
        advance(T["due"])
    cf.result(mu, 0)


def sleep(m, mu):
    ms = cf.arg(mu, 0) & 0xFFFFFFFF
    T["log"].append(("sleep", T["us"], ms))
    advance(ms * 1000 if ms else 1)
    T["sleeps"] += 1
    if T["sleeps"] > 5000:
        m.end_call()


IMPORTS = dict(cf.IMPORTS, **{"winmm.dll!timeGetTime": (time_get_time, 0), "kernel32.dll!Sleep": (sleep, 1),
                              "kernel32.dll!QueryPerformanceCounter": (qpc, 1),
                              "kernel32.dll!QueryPerformanceFrequency": (qpf, 1),
                              "kernel32.dll!CreateWaitableTimerExW": (create_timer, 4),
                              "kernel32.dll!SetWaitableTimer": (set_timer, 6),
                              "kernel32.dll!WaitForSingleObject": (wait, 2)})


def event_loop(mu):
    T["log"].append(("events", T["us"]))


def after_wait(mu):
    if "end" not in T:
        T["end"] = T["us"]


# The enhanced frame's own dvars (P0009), registered on first use: the registration is faked, returning a bool dvar
# (type 1 at +0x18, the value at +0x20, as the Store's Dvar_GetBool 14046B910 reads them) with the value DVARS gives.
# These checks keep the original's overlap of the main and render threads (r_renderOverlap 1) and no timing;
# test_onecpu.py covers the rest. The DLL caches the pointers, so each setup clears them (reset_dvars).
DVARS = {"r_renderOverlap": 1, "com_frameTiming": 0}
DVAR_FAKES = 0x31000000 + 0x1F8000  # in the harness's OBJECTS region


def _read_cstr(mu, va):
    out = b""
    while len(out) < 64:
        c = bytes(mu.mem_read(va + len(out), 1))
        if c == b"\0":
            break
        out += c
    return out.decode()


def dvar_register_bool(mu):
    name = _read_cstr(mu, mu.reg_read(x86.UC_X86_REG_RCX))
    ptr = DVAR_FAKES + 0x40 * sorted(DVARS).index(name)
    mu.mem_write(ptr, b"\0" * 0x40)
    mu.mem_write(ptr + 0x18, struct.pack("<I", 1))
    mu.mem_write(ptr + 0x20, bytes([int(bool(DVARS[name]))]))
    return ptr


def reset_dvars(ctx, w):
    dll = _map(ctx)
    for name in DVARS:
        w.write(dll[name], b"\0" * 8)


FAKES = dict(cf.FAKES, Com_EventLoop=event_loop, TaskManager2_ProcessTasks=after_wait,
             _Dvar_RegisterBool=dvar_register_bool)


def setup_for(ctx, p, next_frame_us, frequency_known, timer):
    dll = _map(ctx)

    def run(w):
        cf.frame_setup(p)(w)
        reset_dvars(ctx, w)
        T.clear()
        T.update(us=p["us0"], log=[], sleeps=0, timer=timer)
        if frequency_known:
            w.write(dll["s_qpcFrequency"], struct.pack("<q", QPC_HZ))
            w.write(dll["s_frameTimer"], struct.pack("<Q", 0x7173 if timer else 0))
        w.write(dll["s_nextFrame"], struct.pack("<q", next_frame_us * 10))
    return run


def base_params(maxfps, frame_ms, us0):
    p = dict(cf.frame_inputs(1)[0]["params"])
    p.update(maxfps=(6, maxfps), clientFpsLimit=0, dedicated=0, queue=0, busy=0, task=0, twitchPending=0, polls=0,
             timeBase=1000, frameTime=frame_ms, us0=us0, step=0, eventLoopMs=0, initialized=1)
    return p


def check_wait_ends_at_its_deadline(ctx):
    ctx.imports(IMPORTS)
    fk, ob = ctx.hooks(FAKES), ctx.hooks(cf.SEEN_ALL)
    dll = _map(ctx)
    checked = 0
    for maxfps in (144, 165, 240, 360, 60, 1000, 2000, 0):
        for lead_us in (-50_000, -7000, -1, 0, 1, 300, 499, 501, 999, 1000, 4166, 6944, 16667):
            for timer in (1, 0):
                for frequency_known, moved in ((1, 1), (0, 1), (1, 0)):
                    us0 = 5_000_000 + 777
                    deadline = us0 + lead_us
                    # the millisecond clock has moved since the last frame, or not yet (then the frame waits for it)
                    frame_ms = us0 // 1000 - moved
                    p = base_params(maxfps, frame_ms, us0)
                    got = ctx.m.call(ctx.enhanced("Com_Frame_Try_Block_Function"), [], "void",
                                     setup_for(ctx, p, deadline, frequency_known, timer), fk, ob)
                    assert not isinstance(got[0], str), got[0]
                    end = T["end"]
                    assert end // 1000 > frame_ms, f"{maxfps}/{lead_us}: ended before the millisecond clock moved"
                    events = [t for e, t, *_ in T["log"] if e == "events"]
                    effective = deadline if maxfps or lead_us <= 0 else deadline
                    start = max(effective, us0, (frame_ms + 1) * 1000)  # the deadline, or the clock's next millisecond
                    assert end >= effective, f"{maxfps}/{lead_us}: ended at {end - deadline} us"
                    assert end - start <= 260, f"{maxfps}/{lead_us}: {end - start} us late"
                    if moved:
                        assert end - max(effective, us0) <= 3, f"{maxfps}/{lead_us}: {end - effective} us late"
                    gaps = [b - a for a, b in zip(events, events[1:])]
                    assert all(g <= 1010 for g in gaps), f"{maxfps}/{lead_us}: event loop gaps {gaps}"
                    for entry in T["log"]:
                        if entry[0] == "set":  # the last half millisecond is spun through, never slept
                            assert entry[1] + entry[2] <= max(deadline, (frame_ms + 1) * 1000) - 499, \
                                f"{maxfps}/{lead_us}: the timer runs to {entry[1] + entry[2] - deadline} us"
                    period = QPC_HZ // maxfps if maxfps > 0 else 0
                    late = end * 10 - deadline * 10
                    want_next = deadline * 10 + period if period and late < period else end * 10 + period
                    nxt = struct.unpack("<q", read(got[2], dll["s_nextFrame"], 8, struct.pack("<q", deadline * 10)))[0]
                    # the last counter read (the one the wait ended on) may be a microsecond before T["end"]
                    assert abs(nxt - want_next) <= 20, f"{maxfps}/{lead_us}: next {nxt}, expected {want_next}"
                    if timer and lead_us > 600 and frequency_known:
                        assert any(e[0] == "wait" for e in T["log"]), f"{maxfps}/{lead_us}: never slept on the timer"
                    checked += 1
    assert checked > 300, checked


def canonical(ctx, calls):
    """The enhanced DLL's own copies of src functions (IN_Frame) reported under the exe's address, as the original
    calls them."""
    import symbols
    back = {va: symbols.resolve(ctx.build, e)[0] for e, va in ctx.recon.exports.items() if symbols.resolve(ctx.build, e)}
    return [(back.get(c[0], c[0]),) + tuple(c[1:]) if isinstance(c[0], int) else c for c in calls]


def check_rest_of_frame_is_the_original(ctx):
    """With the deadline passed and the millisecond clock minMsec ahead, both waits end on their first poll with the
    same msec; then everything matches the original."""
    ctx.imports(IMPORTS)
    fk, ob = ctx.hooks(dict(cf.FAKES, _Dvar_RegisterBool=dvar_register_bool)), ctx.hooks(cf.SEEN_ALL)
    dll = _map(ctx)
    own = set(range(dll["s_qpcFrequency"] & ~7, (dll["s_qpcFrequency"] + 8 + 7) & ~7)) | \
        set(range(dll["s_nextFrame"] & ~7, (dll["s_nextFrame"] + 8 + 7) & ~7)) | \
        set(range(dll["s_frameTimer"] & ~7, (dll["s_frameTimer"] + 8 + 7) & ~7))
    ran = 0
    for k, inp in enumerate(cf.frame_inputs(200, seed=0xFACE)):
        p = dict(inp["params"], step=0, eventLoopMs=0)
        maxfps = p["maxfps"]
        # the clock 20 ms past the last frame (one clock for both: the microsecond model), so both first polls end
        # their waits with the same msec
        p["initialized"] = 1

        def setup(w, p=p):
            cf.frame_setup(p)(w)
            reset_dvars(ctx, w)
            T.update(us=(p["frameTime"] + 20) * 1000 + 500, log=[], sleeps=0, timer=1)
            w.write(dll["s_nextFrame"], struct.pack("<q", 0))
        want = ctx.m.call(ctx.original("Com_Frame_Try_Block_Function"), [], "void", setup, fk, ob)
        if isinstance(want[0], str):
            continue
        got = ctx.m.call(ctx.enhanced("Com_Frame_Try_Block_Function"), [], "void", setup, fk, ob)
        assert not isinstance(got[0], str), f"input {k}: {got[0]}"
        if p["maxfps"][1] and isinstance(p["maxfps"][1], int) and p["maxfps"][1] > 0 and 1000 // p["maxfps"][1] > 20:
            continue  # a cap over 20 ms: the original waits more
        got = (got[0], got[1], ctx.canon.writes(got[2]), canonical(ctx, got[3]))
        want = (want[0], want[1], ctx.canon.writes(want[2]), want[3])
        if got[3] != want[3]:
            i = next((i for i, (a, b) in enumerate(zip(got[3], want[3])) if a != b), min(len(got[3]), len(want[3])))
            raise AssertionError(f"input {k}: calls differ from #{i}\n  original {want[3][i:i + 4]}\n"
                                 f"  enhanced {got[3][i:i + 4]}")
        exe = lambda w: {a: b for a, b in w.items() if a < 0x180000000 or a >= 0x200000000}
        rest = lambda w: {a: b for a, b in exe(w).items() if a not in own}
        if rest(got[2]) != rest(want[2]):
            a, b = rest(got[2]), rest(want[2])
            diff = sorted(x for x in set(a) | set(b) if a.get(x) != b.get(x))[:6]
            raise AssertionError(f"input {k}: writes differ at {[(hex(x), b.get(x), a.get(x)) for x in diff]}")
        ran += 1
    assert ran > 100, ran


CHECKS = [check_wait_ends_at_its_deadline, check_rest_of_frame_is_the_original]
