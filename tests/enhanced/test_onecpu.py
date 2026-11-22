"""P0009: one frame on the CPU at a time, and the frame's stage timing (Store build).

- r_renderOverlap 0 (the default): after the server and before the client reads input, the frame waits for the render
  thread (Sys_WaitRenderer), then for the display (P0007); with r_renderOverlap 1 it doesn't (the original's
  pipelining). A dedicated server's frame does neither.
- The real Sys_WaitRenderer there waits on renderCompletedEvent, in 1 ms timeouts, until the render thread is done.
- With timing off, the enhanced R_ToggleSmpFrameCmd (its waits timed) is the original: the same calls and writes.
- The timing: FrameTiming_Add keeps a stage's total, count and maximum; FrameTiming_EndFrame follows com_frameTiming
  (off: nothing; turned on: starts counting from now), and once a second writes a line of averages and maxima in
  milliseconds, the main thread's work being its frame less its waits, to t6_frame_timing.log next to
  t6_enhanced.dll.
"""
import random
import struct

from unicorn import x86_const as x86

import test_com_frame as cf
import test_framepace as fp
import test_latency as lat
import test_threads as th
from harness import TLS_BLOCK
from test_mouse import _map

BUILDS = ["t6mp_msstore"]
PATCH = "P0009"
RENDER_COMPLETED = 0x14406FBE0
THREAD_ID = 0x14406FBF0
COMPLETED_HANDLE = 0xE0E0


def frame_order(ctx, overlap, dedicated, real_wait, completed_results=()):
    """One enhanced frame: the order of the renderer wait, the display wait, the event loops, input and the screen."""
    v = lat.dll_vars(ctx)
    order = []
    results = list(completed_results)

    def wait(m, mu):
        handle, timeout = cf.arg(mu, 0), cf.arg(mu, 1) & 0xFFFFFFFF
        if handle == lat.WAITABLE_HANDLE:
            order.append("display")
            cf.result(mu, 0)
            return
        if handle == COMPLETED_HANDLE:
            order.append(("completed", timeout))
            cf.result(mu, results.pop(0) if results else 0)
            return
        fp.wait(m, mu)
    ctx.imports(dict(fp.IMPORTS, **{"kernel32.dll!WaitForSingleObject": (wait, 2)}))
    fakes = dict(fp.FAKES, Com_EventLoop=lambda mu: (order.append("events"), fp.event_loop(mu))[0],
                 IN_Frame=lambda mu: order.append("IN_Frame"), SCR_UpdateScreen=lambda mu: order.append("screen"),
                 sub_1405B5DB0=lambda mu: order.append("client"))
    if not real_wait:
        fakes["Sys_WaitRenderer"] = lambda mu: (order.append("renderer"), 0)[1]
    fk, ob = ctx.hooks(fakes), ctx.hooks(cf.SEEN_ALL)
    fp.DVARS.update(r_renderOverlap=overlap, com_frameTiming=0)
    try:
        p = fp.base_params(144, 4999, 5_000_777)
        p.update(dedicated=dedicated, notFirst=1, inputUpdated=0)
        base = fp.setup_for(ctx, p, 0, 1, 1)

        def setup(w):
            base(w)
            w.write(v["s_frameLatencyWaitable"], struct.pack("<Q", lat.WAITABLE_HANDLE))
            w.write(v["s_presentCount"], struct.pack("<ii", 4, 3))  # one present to wait for
            w.write(TLS_BLOCK + 0x3C, struct.pack("<I", 0x111))      # this thread is the main thread
            w.write(THREAD_ID, struct.pack("<I", 0x111))
            w.write(RENDER_COMPLETED, struct.pack("<Q", COMPLETED_HANDLE))
        got = ctx.m.call(ctx.enhanced("Com_Frame_Try_Block_Function"), [], "void", setup, fk, ob)
    finally:
        fp.DVARS.update(r_renderOverlap=1, com_frameTiming=0)
    assert not isinstance(got[0], str), got[0]
    return order


def check_frame_waits_for_the_renderer_before_input(ctx):
    for overlap in (0, 1):
        for dedicated in (0, 1):
            order = frame_order(ctx, overlap, dedicated, real_wait=False)
            if dedicated:
                assert "renderer" not in order and "display" not in order and "screen" not in order, order
                continue
            last_events = len(order) - 1 - order[::-1].index("events")
            if overlap:
                assert "renderer" not in order, order
                continue
            assert order.count("renderer") == 1, order
            assert order.index("IN_Frame") < order.index("renderer") < order.index("display") <= last_events, order
            assert last_events < order.index("screen"), order


def check_renderer_wait_is_on_render_completed(ctx):
    for results in ([0], [0x102, 0], [0x102, 0x102, 0x102, 0]):
        order = frame_order(ctx, 0, 0, real_wait=True, completed_results=results)
        waits = [o for o in order if isinstance(o, tuple)]
        assert waits == [("completed", 1)] * len(results), order
        last_wait = max(i for i, o in enumerate(order) if isinstance(o, tuple))
        assert last_wait < order.index("display") < order.index("screen"), order


def check_handoff_is_the_original_when_untimed(ctx):
    """test_threads.py's R_ToggleSmpFrameCmd inputs, the original's against the enhanced (timing off)."""
    case = th.CASES[-1]
    assert case["name"] == "R_ToggleSmpFrameCmd"
    ctx.imports(th.IMPORTS)
    fk, ob = ctx.hooks(case["fakes"]), ctx.hooks(case["observe"])
    dll = _map(ctx)
    ran = 0
    for k, inp in enumerate(case["inputs"]):
        def setup(w, inp=inp):
            inp["setup"](w)
            w.write(dll["s_timingOn"], b"\0")
        want = ctx.m.call(ctx.original("R_ToggleSmpFrameCmd"), inp["args"], "void", setup, fk, ob)
        got = ctx.m.call(ctx.enhanced("R_ToggleSmpFrameCmd"), inp["args"], "void", setup, fk, ob)
        assert not isinstance(got[0], str), f"input {k}: {got[0]}"
        exe = lambda w: {a: b for a, b in ctx.canon.writes(w).items() if a < 0x180000000 or a >= 0x200000000}
        assert exe(got[2]) == exe(want[2]), f"input {k}: writes differ"
        calls = [(ctx.canon.code(c[0]),) + tuple(c[1:]) for c in got[3]]
        assert calls == want[3], f"input {k}: calls differ\n {want[3][:6]}\n {calls[:6]}"
        ran += 1
    assert ran > 50, ran


# ---- the timing, function by function from prepared state (each call's writes are rolled back after it) ------------
FREQ = 10_000_000
STAGES = ["main", "wait renderer", "wait display", "wait jobs", "render frame", "render idle"]
TOTALS = 0x18  # FrameTimingTotals: ticks, count, max
LOG_HANDLE = 0x10C
MODULE = 0x7E57
DLL_PATH = "C:\\XboxGames\\Call of Duty Black Ops II\\Content\\t6_enhanced.dll"
IO = {}


def u64(mu, va):
    return struct.unpack("<Q", bytes(mu.mem_read(va, 8)))[0]


def qpc(m, mu):
    mu.mem_write(cf.arg(mu, 0), struct.pack("<q", IO["now"]))
    cf.result(mu, 1)


def qpf(m, mu):
    mu.mem_write(cf.arg(mu, 0), struct.pack("<q", FREQ))
    cf.result(mu, 1)


def module_handle(m, mu):
    name = b""
    a = cf.arg(mu, 0)
    while True:
        c = bytes(mu.mem_read(a + len(name), 2))
        if c == b"\0\0":
            break
        name += c
    IO["calls"].append(("GetModuleHandleW", name.decode("utf-16-le")))
    mu.reg_write(x86.UC_X86_REG_RAX, MODULE)


def module_file_name(m, mu):
    handle, buf, size = cf.arg(mu, 0), cf.arg(mu, 1), cf.arg(mu, 2) & 0xFFFFFFFF
    IO["calls"].append(("GetModuleFileNameW", handle, size))
    data = DLL_PATH.encode("utf-16-le") + b"\0\0"
    mu.mem_write(buf, data)
    cf.result(mu, len(DLL_PATH))


def create_file(m, mu):
    a = cf.arg(mu, 0)
    name = b""
    while True:
        c = bytes(mu.mem_read(a + len(name), 2))
        if c == b"\0\0":
            break
        name += c
    rsp = mu.reg_read(x86.UC_X86_REG_RSP)
    disposition, attributes = struct.unpack("<QQ", bytes(mu.mem_read(rsp + 0x28, 16)))
    IO["calls"].append(("CreateFileW", name.decode("utf-16-le"), cf.arg(mu, 1) & 0xFFFFFFFF,
                        cf.arg(mu, 2) & 0xFFFFFFFF, disposition & 0xFFFFFFFF, attributes & 0xFFFFFFFF))
    mu.reg_write(x86.UC_X86_REG_RAX, IO.get("create", LOG_HANDLE))


def write_file(m, mu):
    handle, buf, n, written = cf.arg(mu, 0), cf.arg(mu, 1), cf.arg(mu, 2) & 0xFFFFFFFF, cf.arg(mu, 3)
    IO["writes"].append((handle, bytes(mu.mem_read(buf, n)).decode()))
    mu.mem_write(written, struct.pack("<I", n))
    cf.result(mu, 1)


TIMING_IMPORTS = {"kernel32.dll!QueryPerformanceCounter": (qpc, 1),
                  "kernel32.dll!QueryPerformanceFrequency": (qpf, 1),
                  "kernel32.dll!GetModuleHandleW": (module_handle, 1),
                  "kernel32.dll!GetModuleFileNameW": (module_file_name, 3),
                  "kernel32.dll!CreateFileW": (create_file, 7), "kernel32.dll!WriteFile": (write_file, 5)}


def timing_state(ctx, st):
    """Writes a FrameTiming state: totals [(ticks, count, max)], reported [(ticks, count)], and the scalars."""
    d = _map(ctx)

    def run(w):
        IO.update(now=st["now"], calls=[], writes=[])
        if "create" in st:
            IO["create"] = st["create"]
        else:
            IO.pop("create", None)
        for i, (ticks, count, mx) in enumerate(st["totals"]):
            w.write(d["s_totals"] + TOTALS * i, struct.pack("<qqq", ticks, count, mx))
        for i, (ticks, count) in enumerate(st["reported"]):
            w.write(d["s_reported"] + 16 * i, struct.pack("<qq", ticks, count))
        w.write(d["s_timingOn"], bytes([st["on"]]))
        w.write(d["s_frequency"], struct.pack("<q", st["frequency"]))
        w.write(d["s_lastLine"], struct.pack("<q", st["lastLine"]))
        w.write(d["s_firstLine"], struct.pack("<q", st["firstLine"]))
        w.write(d["s_log"], struct.pack("<q", st["log"]))
        w.write(d["s_logFailed"], bytes([st["logFailed"]]))
        fp.reset_dvars(ctx, w)
    return run


def number(h):
    h = max(h, 0)
    return f"{h // 100}.{h // 10 % 10}{h % 10}"


def trunc_div(a, b):
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b > 0) else -q


def expected_line(st):
    deltas = [(t - r[0], c - r[1]) for (t, c, _), r in zip(st["totals"], st["reported"])]
    main_frames, render_frames = deltas[0][1], deltas[4][1]
    now, freq = st["now"], st["frequency"]
    out = f"{number((now - st['firstLine']) * 100 // freq)} s: {number(main_frames * 100 * freq // (now - st['lastLine']))} fps"
    for i, name in enumerate(STAGES):
        frames = main_frames if i < 4 else render_frames
        total = deltas[i][0]
        if i == 0:
            total -= deltas[1][0] + deltas[2][0] + deltas[3][0]
            name = "main work"
        avg = trunc_div(trunc_div(total * 100000, freq), frames) if frames else 0
        mx = st["totals"][i][2] * 100000 // freq
        out += f" | {name} {number(avg)} ({'frame max ' if i == 0 else 'max '}{number(mx)})"
    return out + "\r\n"


def base_state(r):
    totals, reported = [], []
    for i in range(6):
        count = r.randrange(0, 5000)
        ticks = r.randrange(0, 40_000_000)
        rc, rt = r.randrange(0, count + 1), r.randrange(0, ticks + 1)
        totals.append((ticks, count, r.randrange(0, 2_000_000)))
        reported.append((rt, rc))
    first = r.randrange(1, 10**9)
    last = first + r.randrange(0, 10**9)
    return {"totals": totals, "reported": reported, "on": 1, "frequency": FREQ, "firstLine": first, "lastLine": last,
            "now": last + r.choice([FREQ, FREQ + 1, FREQ + r.randrange(10**7)]), "log": LOG_HANDLE, "logFailed": 0}


def end_frame(ctx, st, on):
    ctx.imports(TIMING_IMPORTS)
    fk = ctx.hooks({"_Dvar_RegisterBool": fp.dvar_register_bool})
    fp.DVARS["com_frameTiming"] = on
    try:
        got = ctx.m.call(ctx.enhanced("FrameTiming_EndFrame"), [], "void", timing_state(ctx, st), fk, {})
    finally:
        fp.DVARS["com_frameTiming"] = 0
    assert not isinstance(got[0], str), got[0]
    return got[2]


def state_after(ctx, st, writes):
    """The FrameTiming scalars after a call: the call's writes over the setup's state."""
    d = _map(ctx)
    out = {}
    for name, fmt, value in (("s_timingOn", "<B", st["on"]), ("s_lastLine", "<q", st["lastLine"]),
                             ("s_firstLine", "<q", st["firstLine"]), ("s_log", "<q", st["log"]),
                             ("s_logFailed", "<B", st["logFailed"]), ("s_frequency", "<q", st["frequency"])):
        size = struct.calcsize(fmt)
        raw = bytearray(struct.pack(fmt, value))
        for i in range(size):
            if d[name] + i in writes:
                raw[i] = writes[d[name] + i][0]
        out[name] = struct.unpack(fmt, bytes(raw))[0]
    return out


def changed(ctx, writes, name, size):
    d = _map(ctx)
    return any(d[name] + i in writes for i in range(size))


def check_timing_lines(ctx):
    r = random.Random(0x9009)
    for k in range(60):
        st = base_state(r)
        end_frame(ctx, st, 1)
        assert IO["calls"] == [], f"input {k}: {IO['calls']}"  # the log is open already
        assert len(IO["writes"]) == 1 and IO["writes"][0][0] == LOG_HANDLE, IO["writes"]
        want = expected_line(st)
        assert IO["writes"][0][1] == want, f"input {k}:\n {want!r}\n {IO['writes'][0][1]!r}"


def check_timing_opens_its_log_once(ctx):
    r = random.Random(0x10C)
    st = base_state(r)
    st.update(log=-1, logFailed=0)
    end_frame(ctx, st, 1)
    path = DLL_PATH.rsplit("\\", 1)[0] + "\\t6_frame_timing.log"
    assert IO["calls"] == [("GetModuleHandleW", "t6_enhanced.dll"), ("GetModuleFileNameW", MODULE, 520),
                           ("CreateFileW", path, 0x40000000, 1, 2, 0x80)], IO["calls"]
    assert len(IO["writes"]) == 2 and IO["writes"][0][1].startswith("t6_enhanced frame timing"), IO["writes"]
    assert IO["writes"][1][1] == expected_line(st), IO["writes"]
    # the file can't be created: no line, and no retry
    st.update(create=-1)
    writes = end_frame(ctx, st, 1)
    assert [c[0] for c in IO["calls"]] == ["GetModuleHandleW", "GetModuleFileNameW", "CreateFileW"] and not IO["writes"]
    assert state_after(ctx, st, writes)["s_logFailed"] == 1  # remembered: the next second doesn't try again
    st.update(logFailed=1)
    end_frame(ctx, st, 1)
    assert IO["calls"] == [] and not IO["writes"], IO


def check_timing_turns_on_and_off(ctx):
    r = random.Random(0x0FF)
    for k in range(20):
        st = base_state(r)
        # off: timing stops, no lines, the next line starts again
        writes = end_frame(ctx, st, 0)
        s = state_after(ctx, st, writes)
        assert s["s_timingOn"] == 0 and s["s_lastLine"] == 0 and not IO["writes"] and not IO["calls"], (k, s)
        # turned on (or on with no line yet): counts start from now
        for on, last in ((0, st["lastLine"]), (1, 0)):
            st2 = dict(st, on=on, lastLine=last, firstLine=r.choice([0, st["firstLine"]]))
            writes = end_frame(ctx, st2, 1)
            s = state_after(ctx, st2, writes)
            assert s["s_timingOn"] == 1 and s["s_lastLine"] == st2["now"] and not IO["writes"], (k, s)
            assert s["s_firstLine"] == (st2["firstLine"] or st2["now"]), (k, s)
            for i, (ticks, count, _) in enumerate(st2["totals"]):
                raw = bytearray(struct.pack("<qq", *st2["reported"][i]))
                for j in range(16):
                    a = d_addr(ctx, "s_reported") + 16 * i + j
                    if a in writes:
                        raw[j] = writes[a][0]
                assert struct.unpack("<qq", bytes(raw)) == (ticks, count), (k, i)
                mx = bytearray(struct.pack("<q", st2["totals"][i][2]))
                for j in range(8):
                    a = d_addr(ctx, "s_totals") + TOTALS * i + 16 + j
                    if a in writes:
                        mx[j] = writes[a][0]
                assert struct.unpack("<q", bytes(mx))[0] == 0, (k, i)
        # on, less than a second since the last line: nothing
        st3 = dict(st, now=st["lastLine"] + FREQ - 1)
        writes = end_frame(ctx, st3, 1)
        assert not IO["writes"] and not IO["calls"] and not changed(ctx, writes, "s_lastLine", 8), k


def d_addr(ctx, name):
    return _map(ctx)[name]


def check_timing_add(ctx):
    ctx.imports(TIMING_IMPORTS)
    r = random.Random(0xADD)
    d = _map(ctx)
    for k in range(60):
        stage = r.randrange(6)
        ticks, count, mx = r.randrange(10**9), r.randrange(10**6), r.randrange(10**6)
        start = r.choice([0, r.randrange(1, 10**9)])
        now = start + r.choice([0, 1, mx - 1, mx, mx + 1, r.randrange(2 * 10**6)]) if start else r.randrange(10**9)
        now = max(now, start)

        def setup(w, stage=stage, ticks=ticks, count=count, mx=mx, now=now):
            IO.update(now=now, calls=[], writes=[])
            w.write(d["s_totals"] + TOTALS * stage, struct.pack("<qqq", ticks, count, mx))
        got = ctx.m.call(ctx.enhanced("FrameTiming_Add"), [stage, start], "void", setup, {}, {})
        assert not isinstance(got[0], str), got[0]
        raw = bytearray(struct.pack("<qqq", ticks, count, mx))
        for j in range(TOTALS):
            a = d["s_totals"] + TOTALS * stage + j
            if a in got[2]:
                raw[j] = got[2][a][0]
        after = struct.unpack("<qqq", bytes(raw))
        if not start:
            assert after == (ticks, count, mx), (k, after)
        else:
            dt = now - start
            assert after == (ticks + dt, count + 1, max(mx, dt)), (k, after, dt)
        others = {a for a in got[2] if d["s_totals"] <= a < d["s_totals"] + TOTALS * 6} - \
            set(range(d["s_totals"] + TOTALS * stage, d["s_totals"] + TOTALS * (stage + 1)))
        assert not others, k


CHECKS = [check_frame_waits_for_the_renderer_before_input, check_renderer_wait_is_on_render_completed,
          check_handoff_is_the_original_when_untimed, check_timing_add, check_timing_lines,
          check_timing_opens_its_log_once, check_timing_turns_on_and_off]
