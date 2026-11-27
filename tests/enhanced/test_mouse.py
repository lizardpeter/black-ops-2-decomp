"""P0002 (B0015, B0017, B0018): raw mouse input, no acceleration, no smoothing; P0003: MW2019 sensitivity (Store
build).

- CL_MouseMove is the original Store function with cl_mouseAccel 0, m_filter off, the FOV scale 1, m_yaw 0.022 and
  the profile's sensitivity at m_sens * 0.3: the same commands, calls and writes, the view angles within rounding.
- One count turns the view by exactly m_sens * 0.0066 degrees (rounded once, to float, from double), whatever the
  FOV scale and the profile's sensitivity; m_pitch's sign inverts the pitch; the yaw stays within [-180, 180).
- The same movement over two frames gives the same angles as in one (the angles are kept in double between calls).
- m_sens is registered on first use: 1.5, 0.01 to 100, archived.
- cl_mouseAccel and m_filter change nothing: the same inputs with them set give the same results.
- A 0 ms frame leaves the movement where it is (the original takes it and drops it).
- IN_MouseMove: where the raw counts equal the cursor's movement, it is the original Store function exactly; where
  they differ, the game gets the counts (menus still get the cursor).
- The raw counters add relative reports only, take and reset, and clamp instead of wrapping.
- In the game: the loader's 14-byte jumps at the exe's CL_MouseMove and IN_MouseMove reach the replacements (the
  redirect table applied to the emulated exe as runtime/loader does it).
"""
import random
import struct

import test_cl_input as tc
import test_win_input as tw

# The enhanced IN_MouseMove subclasses the window procedure once (win_keytime.cpp); a model here keeps these checks
# independent of whether test_keytime.py ran first.
IMPORTS = dict(tw.IMPORTS, **{"user32.dll!SetWindowLongPtrA": (lambda m, mu: tw.result(mu, 0), 0)})

PATCH = "P0002"  # and P0003 (records/patches.tsv)
BUILDS = ["t6mp_msstore"]

YAW = tc.CLIENTS + tc.S_VIEWANGLES + 4
MOUSE_IN = range(tc.S_MOUSE_IN, tc.S_MOUSE_IN + 0x14)


def names(ctx, spec):
    """{address: name} for every address a hook spec hooks (the exe's function and the DLL's copy or thunk)."""
    out = {}
    for name in spec:
        for va in ctx.hooks({name: None}):
            out[va] = name
    return out


def canon(ctx, calls, spec):
    """Observed calls with the function as a name: the enhanced DLL reaches an exe function through its exe-first
    thunk, where the hook fires first."""
    n = names(ctx, spec)
    return [(n.get(c[0], c[0]),) + tuple(c[1:]) for c in calls]


def fmodf_import(m, mu):
    """The CRT's fmodf (the enhanced CL_MouseMove's yaw wrap): exact, so fmod in double is the same."""
    import math
    from unicorn import x86_const as x86
    x, y = (struct.unpack("<f", struct.pack("<I", mu.reg_read(r) & 0xFFFFFFFF))[0]
            for r in (x86.UC_X86_REG_XMM0, x86.UC_X86_REG_XMM1))
    r = math.fmod(x, y) if math.isfinite(x) and y else float("nan")
    mu.reg_write(x86.UC_X86_REG_XMM0, struct.unpack("<I", struct.pack("<f", r))[0])


def fmod_import(m, mu):
    """The CRT's fmod (the enhanced yaw wrap, in double): exact."""
    import math
    from unicorn import x86_const as x86
    x, y = (struct.unpack("<d", struct.pack("<Q", mu.reg_read(r) & ((1 << 64) - 1)))[0]
            for r in (x86.UC_X86_REG_XMM0, x86.UC_X86_REG_XMM1))
    r = math.fmod(x, y) if math.isfinite(x) and y else float("nan")
    mu.reg_write(x86.UC_X86_REG_XMM0, struct.unpack("<Q", struct.pack("<d", r))[0])


SENS_DVAR = tc.DVAR_OBJECTS + 0xF00  # the m_sens dvar_t the registration fake returns (a float dvar)
SENS = {"value": 1.5}


def register_sens(mu):
    """_Dvar_RegisterFloat's fake: the m_sens dvar, holding SENS['value']."""
    tc.STATE["machine"].program_write(SENS_DVAR + 0x18, struct.pack("<i", 2))
    tc.STATE["machine"].program_write(SENS_DVAR + 0x20, struct.pack("<f", SENS["value"]))
    return SENS_DVAR


def mouse_hooks(ctx):
    imports = dict(tc.STORE_MOUSE["imports"], **{"api-ms-win-crt-math-l1-1-0.dll!fmodf": (fmodf_import, 0),
                                                 "api-ms-win-crt-math-l1-1-0.dll!fmod": (fmod_import, 0)})
    for name, (fn, n) in imports.items():
        ctx.m.on_import(name, (lambda f: lambda mu: f(ctx.m, mu))(fn), n)
    return (ctx.hooks(dict(tc.STORE_MOUSE["fakes"], _Dvar_RegisterFloat=register_sens)),
            ctx.hooks(tc.STORE_MOUSE["observe"]))


def read(writes, va, n, default=b"\0"):
    """n bytes of a call's writes (the harness reports each written byte as a one-byte bytes)."""
    if isinstance(default, int):
        default = bytes([default])
    return b"".join(writes.get(va + i, default[i:i + 1] if len(default) > 1 else default) for i in range(n))


def as_float(writes, va):
    return struct.unpack("<f", read(writes, va, 4))[0]


PITCH = tc.CLIENTS + tc.S_VIEWANGLES
ANGLES = range(PITCH, PITCH + 8)
F32 = lambda v: struct.unpack("<f", struct.pack("<f", v))[0]
SCALE = F32(0.0066 / 0.022)  # the moves' scale per unit of m_sens, as the enhanced code rounds it


def angle_diff(a, b):
    d = (a - b) % 360.0
    return min(d, 360.0 - d)


def equivalent(base, sens):
    """The original's settings that move like m_sens = sens: the profile's sensitivity sens * 0.3 (as a float), the FOV
    scale 1 and m_yaw 0.022; m_pitch keeps its sign at 0.022."""
    def run(w):
        base(w)
        tc.STATE["sens"] = F32(F32(sens) * SCALE)
        w.write(tc.CLIENTS + tc.S_FOV_SCALE, struct.pack("<f", 1.0))
        pitch = tc.STATE["dvars"].get(tc.M_PITCH, 0.0)
        for handle, v in ((tc.M_YAW, 0.022), (tc.M_PITCH, 0.022 if pitch > 0 else -0.022 if pitch < 0 else 0.0)):
            w.write(tc.DVAR_OBJECTS + (handle - 0x7100) * 0x80 + 0x20, struct.pack("<f", v))
    return run


def check_mouse_move_is_the_original_at_equivalent_settings(ctx):
    fk, ob = mouse_hooks(ctx)
    spec = {k: v for k, v in tc.STORE_MOUSE["observe"].items() if k != "GamerProfile_GetInt"}
    rng = random.Random(77)
    turned = 0
    for frame in (16, 1, 33):
        inputs = tc.store_mouse_inputs(150, rng=random.Random(frame), frame_msec=frame, finite=True,
                                       dvars_override={tc.M_FILTER: 0, tc.MOUSE_ACCEL: 0.0})
        for i, inp in enumerate(inputs):
            SENS["value"] = F32(rng.choice([1.5, 0.75, 4.0, rng.uniform(0.05, 10)]))
            want = ctx.m.call(ctx.original("CL_MouseMove"), inp["args"], "void", equivalent(inp["setup"], SENS["value"]),
                              fk, ob)
            got = ctx.m.call(ctx.enhanced("CL_MouseMove"), inp["args"], "void", inp["setup"], fk, ob)
            assert not isinstance(got[0], str), f"input {i}: {got[0]}"
            assert got[1] == want[1], f"input {i}: the command differs\n  original {want[1]}\n  enhanced {got[1]}"
            w3 = [c for c in canon(ctx, want[3], tc.STORE_MOUSE["observe"]) if c[0] != "GamerProfile_GetInt"]
            g3 = canon(ctx, got[3], spec)
            assert g3 == w3, f"input {i}: calls differ\n  original {w3}\n  enhanced {g3}"
            # not compared: the angles (above), the enhanced DLL's own data (m_sens, the double angles) and the dvar
            # its registration returns
            diff = sorted(a for a in set(got[2]) | set(want[2])
                          if a not in ANGLES and not ctx.recon.base <= a < ctx.recon.base + ctx.recon.size
                          and not SENS_DVAR <= a < SENS_DVAR + 0x30 and got[2].get(a) != want[2].get(a))
            assert not diff, f"input {i}: writes differ at {[hex(a) for a in diff[:6]]}"
            for va in (PITCH, PITCH + 4):
                if any(a in want[2] or a in got[2] for a in range(va, va + 4)):
                    w, g = as_float(want[2] if va in want[2] else {}, va), as_float(got[2] if va in got[2] else {}, va)
                    turned += 1
                    assert angle_diff(w, g) < 2e-3, f"input {i}: angle {g} vs the original's {w}"
    assert turned > 200, f"only {turned} angles changed: the check proves little"


def turn_setup(counts, start, sens, pitch_scale=0.022, cap=0.0, fov=1.0, profile=5.0, carry=None):
    """One frame of mouse look: counts (x, y) from (pitch, yaw) start, no aim assist; carry: an earlier frame's
    writes, applied first (its double angles)."""
    kb = {tc.KB["STRAFE"]: tc.kbutton(active=0), tc.KB["MLOOK"]: tc.kbutton(active=1)}
    client = {tc.S_FOV_SCALE: struct.pack("<f", fov), tc.S_MAX_YAW_SPEED: struct.pack("<f", cap),
              tc.S_MAX_PITCH_SPEED: struct.pack("<f", cap), tc.S_VIEWANGLES: struct.pack("<ff", *start)}
    globals_ = {tc.S_MOUSE_IN: struct.pack("<5i", counts[0], 0, counts[1], 0, 0),
                tc.S_CLIENT_GAME_STATES: struct.pack("<6i", 0, 0, 0, 0, 0, 1),
                tc.S_AA_GLOB + 0x6C: struct.pack("<B", 0)}
    base = tc.setup(kb=kb, client=client, globals_=globals_, frame_msec=16,
                    dvars={tc.M_YAW: 0.5, tc.M_PITCH: pitch_scale, tc.FREELOOK: 1}, sens=profile, update_view=0)

    def run(w):
        SENS["value"] = sens
        for a, v in (carry or {}).items():
            w.write(a, v)
        base(w)
    return run


def expected(start, counts, sens, sign=1.0):
    """MW2019: start (pitch, yaw) turned by counts * sens * 0.0066 degrees, in double, rounded once to float; the yaw
    wrapped into [-180, 180)."""
    import math
    d = F32(sens) * 0.0066
    pitch = F32(start[0]) + counts[1] * d * sign
    yaw = F32(start[1]) - counts[0] * d
    if yaw >= 180.0 or yaw < -180.0:
        yaw = math.fmod(yaw, 360.0)
        yaw += -360.0 if yaw >= 180.0 else 360.0 if yaw < -180.0 else 0.0
    return F32(pitch), F32(yaw)


def run_turn(ctx, setup):
    fk, ob = mouse_hooks(ctx)
    r = ctx.m.call(ctx.enhanced("CL_MouseMove"), [0, bytes(0x3C), ("f", 16.0)], "void", setup, fk, ob)
    assert not isinstance(r[0], str), r[0]
    return r, (as_float(r[2], PITCH) if PITCH in r[2] else None, as_float(r[2], PITCH + 4) if PITCH + 4 in r[2] else None)


def check_turn_is_mw2019_units(ctx):
    """Exactly m_sens * 0.0066 degrees per count, whatever the FOV scale, the profile's sensitivity and m_yaw; the
    pitch inverted by a negative m_pitch; the yaw within [-180, 180)."""
    for sens in (1.5, 0.5, 3.0, 0.01, 7.25):
        for counts in ((1, 1), (-1, 3), (400, -250), (12345, 0), (0, -7), (100000, 100000)):
            for start in ((0.0, 0.0), (10.0, 179.9), (-45.0, -179.9), (0.0, 539.5)):
                for fov, profile, sign in ((1.0, 5.0, 1.0), (0.4, 0.3, 1.0), (1.3, 20.0, -1.0)):
                    _, got = run_turn(ctx, turn_setup(counts, start, sens, 0.022 * sign, fov=fov, profile=profile))
                    want = expected(start, counts, sens, sign)
                    got = tuple(g if g is not None else F32(s) for g, s in zip(got, start))
                    assert got == want, f"sens {sens}, counts {counts}, start {start}, fov {fov}, " \
                                        f"profile {profile}, sign {sign}: {got}, expected {want}"
                    assert -180.0 <= got[1] < 180.0


def check_split_frames_turn_the_same(ctx):
    """Two frames of a and b counts end at the same angles as one frame of a + b: the double angles carry over."""
    rng = random.Random(3)
    for _ in range(40):
        sens = F32(rng.choice([1.5, 0.8, rng.uniform(0.1, 5)]))
        a = (rng.randrange(-3000, 3000), rng.randrange(-200, 200))
        b = (rng.randrange(-3000, 3000), rng.randrange(-200, 200))
        start = (F32(rng.uniform(-30, 30)), F32(rng.uniform(-179, 179)))
        first, mid = run_turn(ctx, turn_setup(a, start, sens))
        mid = tuple(m if m is not None else s for m, s in zip(mid, start))
        _, two = run_turn(ctx, turn_setup(b, mid, sens, carry=first[2]))
        _, one = run_turn(ctx, turn_setup((a[0] + b[0], a[1] + b[1]), start, sens))
        two = tuple(t if t is not None else m for t, m in zip(two, mid))
        one = tuple(o if o is not None else s for o, s in zip(one, start))
        assert two == one, f"sens {sens}, {a} then {b} from {start}: {two}, in one frame {one}"


def check_sens_is_registered_on_first_use(ctx):
    fk, ob = mouse_hooks(ctx)
    ob = {**ob, **ctx.hooks({"_Dvar_RegisterFloat": "sfffis"})}
    r = ctx.m.call(ctx.enhanced("CL_MouseMove"), [0, bytes(0x3C), ("f", 16.0)], "void",
                   turn_setup((5, 5), (0.0, 0.0), 1.5), fk, ob)
    calls = [c for c in canon(ctx, r[3], {"_Dvar_RegisterFloat": "sfffis"}) if c[0] == "_Dvar_RegisterFloat"]
    f = lambda v: struct.unpack("<I", struct.pack("<f", v))[0]
    assert len(calls) == 1 and calls[0][1] == b"m_sens" and calls[0][2:6] == (f(1.5), f(0.01), f(100.0), 1), calls
    slot = next(a for n, a in _map(ctx).items() if n.startswith("?m_sens@@"))
    assert struct.unpack("<Q", read(r[2], slot, 8))[0] == SENS_DVAR, "m_sens doesn't hold the registered dvar"


def check_accel_and_filter_do_nothing(ctx):
    fk, ob = mouse_hooks(ctx)
    off = tc.store_mouse_inputs(150, rng=random.Random(5), frame_msec=16, finite=True,
                                dvars_override={tc.M_FILTER: 0, tc.MOUSE_ACCEL: 0.0})
    on = tc.store_mouse_inputs(150, rng=random.Random(5), frame_msec=16, finite=True,
                               dvars_override={tc.M_FILTER: 1, tc.MOUSE_ACCEL: 2.5})
    moved = 0
    for i, (a, b) in enumerate(zip(off, on)):
        x = ctx.m.call(ctx.enhanced("CL_MouseMove"), a["args"], "void", a["setup"], fk, ob)
        y = ctx.m.call(ctx.enhanced("CL_MouseMove"), b["args"], "void", b["setup"], fk, ob)
        assert x[1:] == y[1:], f"input {i}: cl_mouseAccel or m_filter changed the result"
        o = ctx.m.call(ctx.original("CL_MouseMove"), b["args"], "void", b["setup"], fk, ob)
        moved += o[1] != y[1] or o[2] != y[2]
    assert moved > 30, f"only {moved} inputs where the original's accel or filter mattered: the check proves little"


def check_zero_ms_frame_keeps_the_movement(ctx):
    fk, ob = mouse_hooks(ctx)
    inputs = tc.store_mouse_inputs(60, rng=random.Random(9), frame_msec=0, finite=True)
    for i, inp in enumerate(inputs):
        got = ctx.m.call(ctx.enhanced("CL_MouseMove"), inp["args"], "void", inp["setup"], fk, ob)
        want = ctx.m.call(ctx.original("CL_MouseMove"), inp["args"], "void", inp["setup"], fk, ob)
        assert not any(a in got[2] for a in MOUSE_IN), f"input {i}: the movement was taken in a 0 ms frame"
        assert got[1][0] == inp["args"][1], f"input {i}: the command changed in a 0 ms frame"
        assert got[3] == [], f"input {i}: calls in a 0 ms frame: {got[3]}"
        assert any(a in want[2] for a in MOUSE_IN), f"input {i}: the original didn't take it either (no test)"


def raw_fakes(ctx, counts, active=1):
    def take(mu):
        rcx, rdx = tw.arg(mu, 0), tw.arg(mu, 1)
        mu.mem_write(rcx, struct.pack("<i", counts[0]))
        mu.mem_write(rdx, struct.pack("<i", counts[1]))
    return ctx.hooks(dict(tw.STORE_FAKES, IN_StartRawInput=lambda mu: active, IN_TakeRawMouse=take))


def check_in_mouse_move_turns_by_raw_counts(ctx):
    ctx.imports(IMPORTS)
    ob = ctx.hooks(tw.STORE_SEEN)
    rng = random.Random(11)
    inputs = tw.store_mouse_inputs(200)
    same = other = 0
    for i, inp in enumerate(inputs):
        # the cursor's movement this input produces: run the original once to see it
        cursor, old, (left, top, right, bottom) = inp["cursor"], inp["old"], inp["rect"]
        if inp["fullscreen"] == 1:  # both clamp the cursor to the window first (IN_ClampMouseMove)
            cursor = (min(max(cursor[0], left), right - 1), min(max(cursor[1], top), bottom - 1))
        moved = (cursor[0] - old[0], cursor[1] - old[1])
        raw = moved if rng.random() < 0.6 else (rng.randrange(-5000, 5000), rng.randrange(-5000, 5000))
        fk = raw_fakes(ctx, raw)
        want = ctx.m.call(ctx.original("IN_MouseMove"), [], "void", inp["setup"], fk, ob)
        got = ctx.m.call(ctx.enhanced("IN_MouseMove"), [], "void", inp["setup"], fk, ob)
        assert not isinstance(got[0], str), f"input {i}: {got[0]}"
        if raw == moved:
            same += 1
            diff = sorted(a for a in set(got[2]) | set(want[2]) if got[2].get(a) != want[2].get(a))
            assert not diff and canon(ctx, got[3], tw.STORE_SEEN) == canon(ctx, want[3], tw.STORE_SEEN), \
                f"input {i}: differs from the original with raw == cursor: {[hex(a) for a in diff[:6]]}"
        elif any(a in want[2] for a in MOUSE_IN):  # the game took the cursor's movement: it takes the counts instead
            other += 1
            index = struct.unpack("<i", bytes(want[2].get(tc.S_MOUSE_IN + 0x10 + k, 0) for k in range(4)))[0] \
                if tc.S_MOUSE_IN + 0x10 in want[2] else inp["index"]
            base = struct.pack("<5i", 3, -4, 5, 6, inp["index"])
            before = struct.unpack("<5i", base)
            after = struct.unpack("<5i", read(got[2], tc.S_MOUSE_IN, 0x14, base))
            j = inp["index"]
            assert after[j] == before[j] + raw[0] and after[2 + j] == before[2 + j] + raw[1], \
                f"input {i}: mouseIn {after}, expected the counts {raw} added to {before}"
    assert same > 50 and other > 10, f"too few inputs of each kind ({same}, {other})"


def check_raw_counters(ctx):
    add, take = ctx.enhanced("IN_RawInputAdd"), ctx.enhanced("IN_TakeRawMouse")
    dll = _map(ctx)
    dx, dy = dll["s_rawDx"], dll["s_rawDy"]

    def rawmouse(flags, x, y):
        return struct.pack("<HHIIiiI", flags, 0, 0, 0, x, y, 0)

    def counters(x, y):
        return lambda w: (w.write(dx, struct.pack("<q", x)), w.write(dy, struct.pack("<q", y)))

    for flags, x, y, sx, sy in ((0, 5, -7, 100, 200), (0, -3, 0, -(1 << 40), 1 << 40), (1, 500, 600, 1, 2),
                                (0, 0x7FFFFFFF, -0x7FFFFFFF, 0x7FFFFFFF, -0x7FFFFFFF)):
        r = ctx.m.call(add, [rawmouse(flags, x, y)], "void", counters(sx, sy), {}, {})
        # the harness reports net changes only: an unchanged counter isn't in the writes
        got = [struct.unpack("<q", read(r[2], a, 8))[0] if a in r[2] else s for a, s in ((dx, sx), (dy, sy))]
        want = [sx, sy] if flags & 1 else [sx + x, sy + y]
        assert got == want, f"add ({flags}, {x}, {y}) to ({sx}, {sy}): counters {got}, expected {want}"
    for sx, sy, want in ((12, -34, (12, -34)), (1 << 40, -(1 << 40), (0x7FFFFFFF, -0x7FFFFFFF)), (0, 0, (0, 0))):
        r = ctx.m.call(take, [bytes(4), bytes(4)], "void", counters(sx, sy), {}, {})
        got = tuple(struct.unpack("<i", b)[0] for b in r[1])
        assert got == want, f"take ({sx}, {sy}): {got}, expected {want}"
        rest = [struct.unpack("<q", read(r[2], a, 8))[0] for a in (dx, dy)]
        assert rest == [0, 0], f"take ({sx}, {sy}): counters left at {rest}"


def check_window_procedure(ctx):
    """WM_INPUT: a mouse report is added while the game's window is in front; keyboard and other devices' reports,
    reports while another window is in front, and other messages aren't. DefWindowProcA always gets the message (it
    frees the WM_INPUT data)."""
    from unicorn import x86_const as x86
    wndproc = ctx.enhanced("IN_RawInputWndProc")
    dx, dy = (_map(ctx)[n] for n in ("s_rawDx", "s_rawDy"))
    state = {}

    def get_raw_input_data(m, mu):
        # (hRawInput, RID_INPUT, pData, pcbSize, cbSizeHeader): the report as Windows lays out RAWINPUT on x64
        header = struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RSP) + 0x28, 4)))[0]  # 5th argument
        size = struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_R9), 4)))[0]       # *pcbSize
        state["args"] = (mu.reg_read(x86.UC_X86_REG_RDX) & 0xFFFFFFFF, header, size)
        data = mu.reg_read(x86.UC_X86_REG_R8)
        kind, flags, x, y = state["report"]
        mu.mem_write(data, struct.pack("<IIQQ", kind, 48, 0x1234, 0) + struct.pack("<HHIIiiI", flags, 0, 0, 0, x, y, 0))
        mu.reg_write(x86.UC_X86_REG_RAX, 48)

    def foreground(m, mu):
        mu.reg_write(x86.UC_X86_REG_RAX, state["foreground"])

    def default(m, mu):
        state["defproc"] = state.get("defproc", 0) + 1
        mu.reg_write(x86.UC_X86_REG_RAX, 0)

    ctx.imports({"user32.dll!GetRawInputData": (get_raw_input_data, 0), "user32.dll!GetForegroundWindow": (foreground, 0),
                 "user32.dll!DefWindowProcA": (default, 0)})
    hwnd_slot = 0x1421E4510 + 0x10  # g_wv.hWnd
    for msg, kind, flags, fg, x, y, adds in ((0xFF, 0, 0, 0x5150, 7, -9, True), (0xFF, 0, 1, 0x5150, 7, -9, False),
                                             (0xFF, 1, 0, 0x5150, 7, -9, False), (0xFF, 0, 0, 0x9999, 7, -9, False),
                                             (0x200, 0, 0, 0x5150, 7, -9, False), (0xFF, 0, 0, 0x5150, -2**31 + 1, 2**31 - 1, True)):
        state.update(report=(kind, flags, x, y), foreground=fg, defproc=0)

        def setup(w):
            w.u64(hwnd_slot, 0x5150)
            w.write(dx, struct.pack("<q", 100))
            w.write(dy, struct.pack("<q", -100))
        r = ctx.m.call(wndproc, [0x777, msg, 0, 0x4444], "int", setup, {}, {})
        assert not isinstance(r[0], str), f"message {msg:#x}: {r[0]}"
        assert msg != 0xFF or state.get("args") == (0x10000003, 24, 48), f"GetRawInputData called with {state.get('args')}"
        got = [struct.unpack("<q", read(r[2], a, 8))[0] if a in r[2] else s for a, s in ((dx, 100), (dy, -100))]
        want = [100 + x, -100 + y] if adds else [100, -100]
        assert got == want, f"message {msg:#x}, type {kind}, flags {flags}, foreground {fg:#x}: {got}, expected {want}"
        assert state["defproc"] == 1, f"message {msg:#x}: DefWindowProcA called {state['defproc']} times"


def _map(ctx):
    import re
    from pathlib import Path
    out = {}
    path = Path(__file__).resolve().parents[2] / "build" / ctx.build / "enhanced" / "recon.map"
    for line in path.read_text().splitlines():
        m = re.match(r"^\s*\w+:\w+\s+(\S+)\s+([0-9a-fA-F]{16})\s", line)
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def check_loader_redirects_reach_the_replacements(ctx):
    """runtime/loader: the redirect table applied to the emulated exe; the exe's own entry points then run the
    enhanced functions (compared with calling them directly)."""
    exe = ctx.m.images[0]
    table = ctx.recon.exports["t6_redirects"]
    count = struct.unpack("<Q", bytes(ctx.m.mu.mem_read(table, 8)))[0]
    patched = {}
    for k in range(count):
        rva, target = struct.unpack("<QQ", bytes(ctx.m.mu.mem_read(table + 8 + 32 * k, 16)))
        head = bytes(ctx.m.mu.mem_read(table + 24 + 32 * k, 16))
        assert bytes(ctx.m.mu.mem_read(exe.base + rva, 16)) == head, f"redirect {k}: prologue differs"
        patched[exe.base + rva] = bytes(ctx.m.mu.mem_read(exe.base + rva, 14))
        ctx.m.mu.mem_write(exe.base + rva, b"\xFF\x25\0\0\0\0" + struct.pack("<Q", target))
        # Unicorn keeps code it has already translated (earlier checks ran the original): drop it, as a CPU would
        # never have run the code before the loader patched it
        ctx.m.mu.ctl_remove_cache(exe.base + rva, exe.base + rva + 14)
    try:
        fk, ob = mouse_hooks(ctx)
        inputs = tc.store_mouse_inputs(60, rng=random.Random(13), frame_msec=16, finite=True,
                                       dvars_override={tc.M_FILTER: 1, tc.MOUSE_ACCEL: 1.5})
        through = ctx.original("CL_MouseMove")
        assert through in patched, "CL_MouseMove isn't in the redirect table"
        for i, inp in enumerate(inputs):
            via_exe = ctx.m.call(through, inp["args"], "void", inp["setup"], fk, ob)
            direct = ctx.m.call(ctx.enhanced("CL_MouseMove"), inp["args"], "void", inp["setup"], fk, ob)
            spec = tc.STORE_MOUSE["observe"]
            assert via_exe[:3] == direct[:3] and canon(ctx, via_exe[3], spec) == canon(ctx, direct[3], spec), \
                f"input {i}: the exe's patched entry doesn't run the replacement\n  {via_exe[0]} {direct[0]} " \
                f"{via_exe[1] == direct[1]} {sorted(set(via_exe[2]) ^ set(direct[2]))[:4]}\n  {via_exe[3]}\n  {direct[3]}"
        assert ctx.original("IN_MouseMove") in patched, "IN_MouseMove isn't in the redirect table"
    finally:
        for va, data in patched.items():
            ctx.m.mu.mem_write(va, data)
            ctx.m.mu.ctl_remove_cache(va, va + 14)


def check_raw_registration_failure_falls_back(ctx):
    ctx.imports(IMPORTS)
    ob = ctx.hooks(tw.STORE_SEEN)
    for i, inp in enumerate(tw.store_mouse_inputs(200)):
        fk = raw_fakes(ctx, (12345, -6789), active=0)
        want = ctx.m.call(ctx.original("IN_MouseMove"), [], "void", inp["setup"], fk, ob)
        got = ctx.m.call(ctx.enhanced("IN_MouseMove"), [], "void", inp["setup"], fk, ob)
        assert not isinstance(got[0], str), (i, got[0])
        assert want[:3] == got[:3] and canon(ctx, want[3], tw.STORE_SEEN) == canon(ctx, got[3], tw.STORE_SEEN), (i, "registration-failure fallback differs from Store")

CHECKS = [check_mouse_move_is_the_original_at_equivalent_settings, check_turn_is_mw2019_units,
          check_split_frames_turn_the_same, check_sens_is_registered_on_first_use, check_accel_and_filter_do_nothing,
          check_zero_ms_frame_keeps_the_movement, check_in_mouse_move_turns_by_raw_counts,
          check_raw_counters, check_window_procedure, check_loader_redirects_reach_the_replacements,
          check_raw_registration_failure_falls_back]
