"""P0004 (B0019, B0020): key events on the game's clock, timed by the raw input thread (Store build).

- IN_RawKeyAdd records a key's change with the game's time (timeGetTime - sys_timeBase) in the key's ring; no key,
  the escape-sequence prefix key, a full ring and a game clock not yet set up record nothing.
- The subclassed window procedure sets g_wv.sysMsgTime on the game's clock before calling the game's: a key message
  takes the first raw change of that key in the same direction within 250 ms of its own time, dropping changes
  before it that don't match; anything else takes the message's time moved onto the game's clock. The game's
  procedure always gets the message unchanged.
- The game's window is subclassed once.
- The raw input window records keyboard reports while the game is in front.
"""
import struct

from unicorn import x86_const as x86

from test_mouse import _map, read

BUILDS = ["t6mp_msstore"]
PATCH = "P0004"
SYS_TIME_BASE, SYS_TIME_INIT = 0x1421E448C, 0x14449E974
G_WV = 0x1421E4510
HWND_SLOT, SYS_MSG_TIME = G_WV + 0x10, G_WV + 0x38
WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, WM_SYSKEYUP, WM_CHAR = 0x100, 0x101, 0x104, 0x105, 0x102
STATE = {}


def ret(mu, v):
    mu.reg_write(x86.UC_X86_REG_RAX, v & 0xFFFFFFFFFFFFFFFF)


def clock_imports(ctx):
    def call_window_proc(m, mu):
        args = [mu.reg_read(r) for r in (x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8,
                                         x86.UC_X86_REG_R9)]
        args.append(struct.unpack("<Q", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RSP) + 0x28, 8)))[0])
        STATE.setdefault("calls", []).append(("CallWindowProcA", args))
        ret(mu, 0x77)

    def set_window_long_ptr(m, mu):
        STATE.setdefault("calls", []).append(("SetWindowLongPtrA", [mu.reg_read(r) for r in (
            x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8)]))
        ret(mu, STATE.get("previous", 0x1111))

    ctx.imports({"winmm.dll!timeGetTime": (lambda m, mu: ret(mu, STATE["timeGetTime"]), 0),
                 "kernel32.dll!GetTickCount": (lambda m, mu: ret(mu, STATE["tick"]), 0),
                 "user32.dll!GetMessageTime": (lambda m, mu: ret(mu, STATE["msgTime"]), 0),
                 "user32.dll!CallWindowProcA": (call_window_proc, 0),
                 "user32.dll!SetWindowLongPtrA": (set_window_long_ptr, 0)})


def ring_layout(ctx):
    dll = _map(ctx)
    return dll["s_keyRing"], dll["s_keyHead"], dll["s_keyTail"]


def clock(w, base=1000, init=1):
    w.write(SYS_TIME_BASE, struct.pack("<i", base))
    w.write(SYS_TIME_INIT, struct.pack("<i", init))


def ring_setup(ctx, vk, entries, tail=0, base=1000, init=1, extra=None):
    ring, heads, tails = ring_layout(ctx)

    def run(w):
        clock(w, base, init)
        for i, (time, down) in enumerate(entries):
            w.write(ring + (vk * 8 + (tail + i) % 8) * 8, struct.pack("<iB3x", time, down))
        w.write(heads + vk, bytes([(tail + len(entries)) & 0xFF]))
        w.write(tails + vk, bytes([tail & 0xFF]))
        if extra:
            extra(w)
    return run


def rawkeyboard(vk, flags, make=0x1E):
    return struct.pack("<HHHHII", make, flags, 0, vk, 0x100, 0)


def check_raw_keys_recorded(ctx):
    clock_imports(ctx)
    add = ctx.enhanced("IN_RawKeyAdd")
    ring, heads, tails = ring_layout(ctx)
    for vk, flags, init, filled, records in ((0x41, 0, 1, 0, True), (0x41, 1, 1, 3, True), (0x10, 0, 1, 7, True),
                                             (0x41, 0, 1, 8, False), (0, 0, 1, 0, False), (0xFF, 0, 1, 0, False),
                                             (0x41, 0, 0, 0, False), (0xFE, 1, 1, 0, True)):
        STATE.update(timeGetTime=0x80000123, calls=[])
        r = ctx.m.call(add, [rawkeyboard(vk, flags)], "void",
                       ring_setup(ctx, vk, [(5, True)] * filled, tail=250, init=init), {}, {})
        assert not isinstance(r[0], str), r[0]
        head = r[2].get(heads + vk, bytes([(250 + filled) & 0xFF]))[0]
        if not records:
            assert head == (250 + filled) & 0xFF and not any(ring <= a < ring + 256 * 64 for a in r[2]), \
                f"vk {vk:#x}, init {init}, {filled} queued: recorded"
            continue
        assert head == (251 + filled) & 0xFF, f"vk {vk:#x}: head {head}"
        slot = ring + (vk * 8 + (250 + filled) % 8) * 8
        time, down = struct.unpack("<iB", read(r[2], slot, 5, bytes(8)))
        assert (time, down) == (0x80000123 - 1000, 0 if flags & 1 else 1), \
            f"vk {vk:#x}: recorded ({time}, {down})"


def check_key_messages_get_raw_times(ctx):
    clock_imports(ctx)
    proc = ctx.enhanced("IN_KeyTimeWndProc")
    ring, heads, tails = ring_layout(ctx)
    game_proc = _map(ctx)["s_gameWndProc"]
    # game clock now 20000 (timeGetTime 21000, base 1000); the message 30 ms old: its time 19970 on the game's clock
    STATE.update(timeGetTime=21000, tick=5_000_030, msgTime=5_000_000)
    fallback = 19970
    cases = [
        # (message, vk, ring entries (time, down), expected time, entries left)
        (WM_KEYDOWN, 0x57, [(19975, True)], 19975, 0),
        (WM_KEYUP, 0x57, [(19981, False)], 19981, 0),
        (WM_SYSKEYDOWN, 0x12, [(19960, True)], 19960, 0),
        (WM_SYSKEYUP, 0x12, [(19960, False), (19990, True)], 19960, 1),
        (WM_KEYDOWN, 0x57, [(19950, False), (19975, True), (19990, False)], 19975, 1),  # an unmatched up dropped
        (WM_KEYDOWN, 0x57, [(17000, True), (19999, True)], 19999, 0),                   # a stale down dropped
        (WM_KEYDOWN, 0x57, [(17000, True)], fallback, 0),
        (WM_KEYDOWN, 0x57, [], fallback, 0),
        (WM_KEYUP, 0x57, [(19975, True)], fallback, 0),
        (WM_CHAR, 0x57, [(19975, True)], fallback, 1),                                   # not a key change
        (0x201, 0x01, [], fallback, 0),
        (WM_KEYDOWN, 0x157, [(19975, True)], 19975, 0),                                  # vk is wParam's low byte
    ]
    for msg, vk, entries, want, left in cases:
        STATE["calls"] = []
        setup = ring_setup(ctx, vk & 0xFF, entries, tail=254, extra=lambda w: w.u64(game_proc, 0xABCDEF))
        r = ctx.m.call(proc, [0x5150, msg, vk, 0x1E0001], "int", setup, {}, {})
        assert not isinstance(r[0], str), r[0]
        assert r[0] == 0x77, f"returned {r[0]:#x}"
        got = struct.unpack("<i", read(r[2], SYS_MSG_TIME, 4))[0]
        assert got == want, f"message {msg:#x}, vk {vk:#x}, {entries}: sysMsgTime {got}, expected {want}"
        tail = r[2].get(tails + (vk & 0xFF), bytes([254]))[0]
        assert (254 + len(entries) - tail) & 0xFF == left, f"message {msg:#x}, {entries}: {left} should be left"
        assert STATE["calls"] == [("CallWindowProcA", [0xABCDEF, 0x5150, msg, vk, 0x1E0001])], STATE["calls"]
    # the game's clock not set up yet: sysMsgTime is the game's own (MSG.time), untouched
    STATE["calls"] = []
    r = ctx.m.call(proc, [0x5150, WM_KEYDOWN, 0x57, 0], "int", ring_setup(ctx, 0x57, [(19975, True)], init=0), {}, {})
    assert SYS_MSG_TIME not in r[2] and len(STATE["calls"]) == 1, "changed sysMsgTime before the clock is set up"


def check_window_subclassed_once(ctx):
    clock_imports(ctx)
    hook = ctx.enhanced("IN_TimeWindowMessages")
    dll = _map(ctx)
    timed, game_proc = dll["s_timedWindow"], dll["s_gameWndProc"]
    proc = ctx.enhanced("IN_KeyTimeWndProc")
    for hwnd, already, previous, calls in ((0x5150, 0, 0x1111, 1), (0x5150, 0x5150, 0x1111, 0), (0, 0, 0x1111, 0),
                                           (0x6000, 0x5150, 0x2222, 1), (0x5150, 0, 0, 1)):
        STATE.update(calls=[], previous=previous)

        def setup(w, hwnd=hwnd, already=already):
            w.u64(HWND_SLOT, hwnd)
            w.u64(timed, already)
        r = ctx.m.call(hook, [], "void", setup, {}, {})
        assert not isinstance(r[0], str), r[0]
        assert len(STATE["calls"]) == calls, f"hWnd {hwnd:#x}, subclassed {already:#x}: {STATE['calls']}"
        if calls:
            args = STATE["calls"][0][1]
            assert [args[0], args[1] & 0xFFFFFFFF, args[2]] == [hwnd, 0xFFFFFFFC, proc], STATE["calls"]
            now_timed = struct.unpack("<Q", read(r[2], timed, 8, struct.pack("<Q", already)))[0]
            now_proc = struct.unpack("<Q", read(r[2], game_proc, 8, bytes(8)))[0]
            want = (hwnd, previous) if previous else (already, 0)
            assert (now_timed, now_proc) == want, f"hWnd {hwnd:#x}: ({now_timed:#x}, {now_proc:#x}), expected {want}"


def check_raw_window_records_keys(ctx):
    """WM_INPUT keyboard reports go to IN_RawKeyAdd while the game's window is in front, not otherwise."""
    clock_imports(ctx)
    wndproc = ctx.enhanced("IN_RawInputWndProc")
    ring, heads, tails = ring_layout(ctx)
    state = {}

    def get_raw_input_data(m, mu):
        data = mu.reg_read(x86.UC_X86_REG_R8)
        mu.mem_write(data, struct.pack("<IIQQ", state["kind"], 40, 0x1234, 0) + rawkeyboard(0x44, state["flags"]))
        ret(mu, 40)

    ctx.imports({"user32.dll!GetRawInputData": (get_raw_input_data, 0),
                 "user32.dll!GetForegroundWindow": (lambda m, mu: ret(mu, state["fg"]), 0),
                 "user32.dll!DefWindowProcA": (lambda m, mu: ret(mu, 0), 0)})
    STATE.update(timeGetTime=4321)
    for kind, flags, fg, records in ((1, 0, 0x5150, True), (1, 1, 0x5150, True), (1, 0, 0x9999, False),
                                     (0, 0, 0x5150, False)):
        state.update(kind=kind, flags=flags, fg=fg)
        setup = ring_setup(ctx, 0x44, [], tail=3, extra=lambda w: w.u64(HWND_SLOT, 0x5150))
        r = ctx.m.call(wndproc, [0x777, 0xFF, 0, 0x4444], "int", setup, {}, {})
        assert not isinstance(r[0], str), r[0]
        head = r[2].get(heads + 0x44, bytes([3]))[0]
        assert head == (4 if records else 3), f"type {kind}, foreground {fg:#x}: head {head}"
        if records:
            time, down = struct.unpack("<iB", read(r[2], ring + (0x44 * 8 + 3) * 8, 5))
            assert (time, down) == (3321, 0 if flags else 1), (time, down)


CHECKS = [check_raw_window_records_keys, check_raw_keys_recorded, check_key_messages_get_raw_times, check_window_subclassed_once]
