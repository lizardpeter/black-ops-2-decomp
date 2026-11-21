"""client/cl_input.cpp.

The setups write CoDMPServer_PC's globals at their addresses (MAP, statics.tsv), so the cases run there. Functions
the server gave custom conventions are called with them (records/conventions.tsv).
"""
import random
import struct

from unicorn import x86_const as x86

from harness import OBJECTS, SCRATCH

rng = random.Random(2042)
STATE = {}
SERVER = ["CoDMPServer_PC"]

PLAYERS_KB = 0x011C16E0
KB_SIZE, NUM_BUTTONS = 0x18, 45
CLIENT_UI = 0x01237050
CL_MAX_LOCAL_CLIENTS = 0x01237580
CLIENTS_PTR = 0x01237588  # clientActive_t *clients, allocated at startup
CLIENTS = OBJECTS
COM_FRAMETIME = 0x01E29A28
FRAME_MSEC = 0x01237034
G_LEANED = 0x011C1B2C
ATTACK_EDGE = 0x011C1B30
LATCH = 0x011C1B31
DVAR_HANDLES = {0x011C16DC: 0x7101,  # cl_leanCountMin
                0x0798718C: 0x7102,  # cl_dblTapMaxHoldTime
                0x0798767C: 0x7103,  # cl_dblTapMaxDelayTime
                0x082324F0: 0x7104}  # player_standingViewHeight
LEAN_MIN, HOLD, DELAY, STANDING = 0x7101, 0x7102, 0x7103, 0x7104

# clientActive_t fields
USING_ADS, SNAP_VALID, PS = 0x44, 0x50, 0x70
PS_PM_FLAGS, PS_DELTA_ANGLES, PS_EFLAGS, PS_WEAPON, PS_VIEWHEIGHT = 0xC, 0x98, 0xFC, 0x1B8, 0x208
STANCE_HELD, STANCE, STANCE_POSITION, STANCE_TIME = 0x2BCC, 0x2BD0, 0x2BD4, 0x2BD8
WEAPNEXT_HELD, WEAPNEXT_TIME, VIEWANGLES = 0x2BDC, 0x2BE0, 0x2C8C
LAST_FIRE, USE_HELD, USE_TIME, USE_COUNT = 0x8A7B4, 0x8A7B8, 0x8A7BC, 0x8A7C0

KB = {n: i for i, n in enumerate(
    "LEFT RIGHT FORWARD BACK LOOKUP LOOKDOWN MOVELEFT MOVERIGHT STRAFE SPEED UP DOWN ANYUP MLOOK SPECTATE SPECNEXT "
    "SPECPREV ATTACK BREATH FRAG OFFHANDSECONDARY THROWBACK MELEE ACTIVATE RELOAD USE_RELOAD LEANLEFT LEANRIGHT PRONE "
    "CROUCH THROW STANCE SPRINT NIGHTVISION GAS REVERSE HANDBRAKE SWITCHSEAT VEHICLEATTACK VEHICLEATTACKSECOND "
    "VEHICLEMOVEUP VEHICLEMOVEDOWN VEHICLEBOOST JAM TALK".split())}

PREDICTED_PS = SCRATCH + 0x40000   # what the CG_GetPredictedPlayerState fake returns
WEAPON_DEF = SCRATCH + 0x50000     # what the BG_GetWeaponDef fake returns (or 0)
ARGV = SCRATCH + 0x60000           # Cmd_Argv's strings


def arg(mu, i):
    return struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_ESP) + 4 + 4 * i, 4)))[0]


def kbutton(down0=0, down1=0, downtime=0, msec=0, active=0, pressed=0, val=0.0):
    return struct.pack("<iiIIBBxxf", down0, down1, downtime & 0xFFFFFFFF, msec & 0xFFFFFFFF, active, pressed, val)


def random_kbutton(keys=(0, 0, 0, 5, 9, 13)):
    return kbutton(rng.choice(keys), rng.choice(keys), rng.choice([0, 1000, 5000]), rng.choice([0, 3, 16, 50]),
                   rng.randrange(2), rng.randrange(2), rng.choice([0.0, 1.0, 0.05, 0.5]))


def kb_va(button, lc=0):
    return PLAYERS_KB + lc * KB_SIZE * NUM_BUTTONS + button * KB_SIZE


def setup(kb=None, client=None, ui_state=10, frame_time=10000, frame_msec=16, dvars=None, ps=None, globals_=None,
          **state):
    """kb: {button index: kbutton bytes}; client: {field offset: bytes or int}; ps: predicted playerState fields."""
    def run(w):
        STATE.clear()
        STATE.update(state)
        STATE["dvars"] = dvars or {}
        w.u32(CLIENT_UI + 8, ui_state)
        w.u32(CL_MAX_LOCAL_CLIENTS, 1)
        w.u32(CLIENTS_PTR, CLIENTS)
        w.u32(COM_FRAMETIME, frame_time)
        w.u32(FRAME_MSEC, frame_msec)
        for va, handle in DVAR_HANDLES.items():
            w.u32(va, handle)
        for button, data in (kb or {}).items():
            w.write(kb_va(button), data)
        for off, v in (client or {}).items():
            w.write(CLIENTS + off, v if isinstance(v, bytes) else struct.pack("<i", v))
        for off, v in (ps or {}).items():
            w.write(PREDICTED_PS + off, v if isinstance(v, bytes) else struct.pack("<i", v))
        for va, v in (globals_ or {}).items():
            w.write(va, v if isinstance(v, bytes) else struct.pack("<i", v))
        for i, s in enumerate(STATE.get("argv", [])):
            w.write(ARGV + 0x100 * i, s.encode() + b"\0")
        if "weapdef" in STATE and STATE["weapdef"] is not None:
            w.write(WEAPON_DEF + 0x44, struct.pack("<i", STATE["weapdef"]))
    return run


def cmd_argv(mu):
    i = arg(mu, 0)
    return ARGV + 0x100 * i if i < len(STATE.get("argv", [])) else ARGV + 0x100 * 15  # "" past the arguments


FAKES = {
    "?Dvar_GetInt@@YAHPBUdvar_t@@@Z": lambda mu: STATE["dvars"].get(arg(mu, 0), 0),
    "CG_GetPredictedPlayerState": lambda mu: PREDICTED_PS,
    "Cmd_Argv": cmd_argv,
    "Com_Printf": lambda mu: None,
    "Sys_Milliseconds": lambda mu: STATE.get("now", 0),
    "BG_GetWeaponDef": lambda mu: WEAPON_DEF if STATE.get("weapdef") is not None else 0,
    "BG_GetFireType": lambda mu: STATE.get("firetype", 1),
    "BG_WeaponBlocksProne": lambda mu: STATE.get("blocks_prone", 0),
    "CG_CycleWeapon": lambda mu: None,
    "Assert_MyHandler": lambda mu: 1,
}
SEEN = {
    "?Dvar_GetInt@@YAHPBUdvar_t@@@Z": "q", "CG_GetPredictedPlayerState": "i", "Cmd_Argv": "i",
    "Com_Printf": "is", "BG_GetWeaponDef": "i", "BG_GetFireType": "i", "BG_WeaponBlocksProne": "i",
    "CG_CycleWeapon": "ii", "Assert_MyHandler": "siiss", "CL_GetLocalClientGlobals": "i",
}


def stances():
    return [struct.pack("<i", s) for s in (0, 1, 2, 3)]


def kb_inputs(n=60):
    """A button in a random state, and a key event on it."""
    out = []
    for _ in range(n):
        out.append({"key": rng.choice([0, 5, 9, 13, -1]), "time": rng.choice([0, 1000, 7000]),
                    "value": rng.choice([0.0, 1.0, 0.25, -0.5]), "button": random_kbutton()})
    return out


def handler_cases(name, button, down, extra_kb=(), client=None, **state):
    """Cases for IN_<name>(localClientNum, key, time[, value]) on playersKb[0][button]."""
    inputs = []
    for e in kb_inputs(40):
        kb = {button: e["button"]}
        for b in extra_kb:
            kb[b] = random_kbutton()
        args = [0, e["key"], e["time"]] + ([("f", e["value"])] if down else [])
        cl = dict(client or {})
        cl[USE_HELD] = rng.randrange(2)
        cl[USE_TIME] = rng.choice([0, 9000, 9900])
        cl[USE_COUNT] = rng.randrange(3)
        inputs.append({"args": args, "setup": setup(kb=kb, client=cl, ui_state=rng.choice([5, 6, 10]),
                                                    dvars={HOLD: 300, DELAY: 250}, **state)})
    return {"name": name, "ret": "void", "builds": SERVER, "inputs": inputs, "fakes": FAKES, "observe": SEEN}


def stance_inputs(down):
    out = []
    for _ in range(80):
        kb = {KB["STANCE"]: random_kbutton(), KB["DOWN"]: random_kbutton(), KB["PRONE"]: random_kbutton(),
              KB["VEHICLEMOVEDOWN"]: random_kbutton()}
        client = {STANCE: rng.randrange(4), STANCE_POSITION: rng.randrange(4), STANCE_HELD: rng.randrange(2),
                  PS + PS_EFLAGS: rng.choice([0, 0x4000, 0x4001]), PS + PS_WEAPON: 7}
        ps = {PS_VIEWHEIGHT: struct.pack("<f", rng.choice([40.0, 60.0, 61.0])), PS_PM_FLAGS: rng.choice([0, 0x8000])}
        args = [0, rng.choice([5, 9]), rng.choice([0, 7000])] + ([("f", 1.0)] if down else [])
        out.append({"args": args, "setup": setup(kb=kb, client=client, ps=ps, ui_state=rng.choice([5, 6, 10]),
                                                 dvars={STANDING: 60}, blocks_prone=rng.randrange(2))})
    return out


def keystate_inputs():
    out = []
    for _ in range(120):
        b = kbutton(downtime=rng.choice([0, 9990, 9000, 12000]), msec=rng.choice([0, 1, 7, 16, 40, -5]),
                    active=rng.randrange(2))
        out.append({"args": [b], "setup": setup(frame_time=10000, frame_msec=rng.choice([16, 33, 7, 1]))})
    out.append({"args": [kbutton(msec=5)], "setup": setup(frame_msec=0)})
    return out


def keydown_cheat_inputs():
    out = []
    for _ in range(60):
        argv = ["+attack", rng.choice(["", "5", "13", "-1"]), rng.choice(["", "1234", "0"]), rng.choice(["", "0.5", "1"])]
        out.append({"args": [random_kbutton(keys=(0, 5, 13))], "setup": setup(argv=argv)})
    return out


CASES = [
    {"name": "CL_LeanCount_Reset", "ret": "void", "builds": SERVER,
     "inputs": [{"args": [], "setup": setup(globals_={G_LEANED: 7})}]},
    {"name": "CL_LeanCount_Increment", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(globals_={G_LEANED: n}, dvars={LEAN_MIN: m})} for n in (0, 4) for m in (0, 3)]},
    {"name": "CL_LeanCount_ShouldIncrementStat", "ret": "bool", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(globals_={G_LEANED: n}, dvars={LEAN_MIN: m})}
                for n in (0, 2, 3, 4) for m in (0, 3)]},
    {"name": "CG_IsSprinting", "ret": "bool", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0], "setup": setup(ps={PS_VIEWHEIGHT: struct.pack("<f", h), PS_PM_FLAGS: f}, dvars={STANDING: 60})}
                for h in (59.9, 60.0, 61.0, float("nan")) for f in (0, 0x8000, 0x8001)]},
    {"name": "IN_KeyDown", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [e["button"], e["key"], e["time"], ("f", e["value"])], "setup": setup()} for e in kb_inputs()]},
    {"name": "IN_KeyUp", "ret": "void", "builds": SERVER,
     "inputs": [{"args": [e["button"], e["key"], e["time"]], "setup": setup(frame_msec=rng.choice([16, 33]))}
                for e in kb_inputs()]},
    {"name": "IN_KeyDownCheat", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": keydown_cheat_inputs()},
    {"name": "IN_KeyUpCheat", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": keydown_cheat_inputs()},
    {"name": "CL_KeyState", "ret": "float", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": keystate_inputs()},
    {"name": "CL_SetStance", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, s], "setup": setup(kb={KB["PRONE"]: kbutton(active=p), KB["DOWN"]: kbutton(active=d)})}
                for s in range(4) for p in (0, 1) for d in (0, 1)]},
    {"name": "CL_ToggleStance", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, s], "setup": setup(kb={KB["PRONE"]: kbutton(active=p), KB["DOWN"]: kbutton(active=d)},
                                                client={STANCE: c},
                                                ps={PS_VIEWHEIGHT: struct.pack("<f", 61.0), PS_PM_FLAGS: f},
                                                dvars={STANDING: 60})}
                for s in range(4) for c in range(4) for p in (0, 1) for d in (0, 1) for f in (0, 0x8000)]},
    {"name": "IN_UpDown", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(kb={KB["PRONE"]: kbutton(active=p), KB["DOWN"]: kbutton(active=d),
                                                KB["ANYUP"]: random_kbutton(), KB["UP"]: random_kbutton()},
                                            client={STANCE: c}, argv=["+gostand", "5", "100", ""])}
                for c in range(4) for p in (0, 1) for d in (0, 1)]},
    {"name": "IN_UpUp", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(kb={KB["ANYUP"]: random_kbutton((0, 5)), KB["UP"]: random_kbutton((0, 5))},
                                            argv=["-gostand", rng.choice(["", "5"]), "200"])} for _ in range(20)]},
    {"name": "IN_SpeedDown", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(kb={KB["SPEED"]: random_kbutton((0, 5))}, client={USING_ADS: b"\x01"},
                                            argv=["+speed", "5", "100", ""])} for _ in range(10)]},
    {"name": "IN_SpeedUp", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(kb={KB["SPEED"]: random_kbutton((0, 5))}, client={USING_ADS: b"\x01"},
                                            argv=["-speed", "5", "100"])} for _ in range(10)]},
    {"name": "IN_UpdateUseCount", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": setup(client={USE_TIME: t, USE_COUNT: 2, USE_HELD: b"\x01"}, ui_state=u,
                                              dvars={HOLD: 300})}
                for lc in (0, 1) for t in (9000, 9800) for u in (5, 6)]},
    {"name": "IN_UpdateUseHeld", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0], "setup": setup(client={USE_TIME: t, USE_COUNT: 2, USE_HELD: bytes([h])}, ui_state=u,
                                             dvars={DELAY: 250})}
                for t in (9000, 9800) for u in (5, 6) for h in (0, 1)]},
    handler_cases("IN_Activate_Down", KB["ACTIVATE"], True),
    handler_cases("IN_Activate_Up", KB["ACTIVATE"], False),
    handler_cases("IN_Reload_Down", KB["RELOAD"], True),
    handler_cases("IN_Reload_Up", KB["RELOAD"], False),
    handler_cases("IN_UseReload_Down", KB["USE_RELOAD"], True),
    handler_cases("IN_UseReload_Up", KB["USE_RELOAD"], False),
    handler_cases("IN_Melee_Down", KB["MELEE"], True),
    handler_cases("IN_Melee_Up", KB["MELEE"], False),
    handler_cases("IN_Frag_Down", KB["FRAG"], True),
    handler_cases("IN_Frag_Up", KB["FRAG"], False),
    handler_cases("IN_Smoke_Down", KB["OFFHANDSECONDARY"], True),
    handler_cases("IN_Smoke_Up", KB["OFFHANDSECONDARY"], False),
    handler_cases("IN_Attack_Up", KB["ATTACK"], False, extra_kb=(KB["JAM"],),
                  globals_={ATTACK_EDGE: b"\x01"}),
    {"name": "IN_Attack_Down", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, rng.choice([5, 9]), rng.choice([0, 7000]), ("f", 1.0)],
                 "setup": setup(kb={KB["ATTACK"]: random_kbutton(), KB["JAM"]: random_kbutton()},
                                client={SNAP_VALID: rng.choice([0, 1, 1]), PS + PS_WEAPON: 3,
                                        LAST_FIRE: rng.choice([0, 9900, 9950, 9999])},
                                globals_={ATTACK_EDGE: bytes([rng.randrange(2)])},
                                now=rng.choice([10000, 10037, 10099]), weapdef=rng.choice([None, 50, 100, 300]),
                                firetype=rng.choice([0, 1, 1]))} for _ in range(120)] + [
         # the jam roll's edges: now % 100 equal to the chance (37, 0), one under and one over it
         {"args": [0, 5, 7000, ("f", 1.0)],
          "setup": setup(kb={KB["ATTACK"]: kbutton(), KB["JAM"]: kbutton()},
                         client={SNAP_VALID: 1, PS + PS_WEAPON: 3, LAST_FIRE: now - gap},
                         now=now, weapdef=300, firetype=1)}
         for now in (10036, 10037, 10038, 10000, 10099) for gap in (253, 299, 300)]},
    {"name": "IN_Stance_Down", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": stance_inputs(True)},
    {"name": "IN_Stance_Up", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": stance_inputs(False)},
    {"name": "IN_WeapNextInventoryDown", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0], "setup": setup(ui_state=u)} for u in (5, 6)]},
    {"name": "IN_WeapNextInventoryUp", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0], "setup": setup(ui_state=u, client={WEAPNEXT_HELD: bytes([h])})} for u in (5, 6) for h in (0, 1)]},
    {"name": "IN_ToggleADS", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0], "setup": setup(ui_state=u, client={USING_ADS: bytes([a])}, globals_={LATCH: bytes([l])},
                                             kb={KB["THROW"]: kbutton(val=v)})}
                for u in (5, 6) for a in (0, 1) for l in (0, 1) for v in (0.0, 0.0999, 0.1, 1.0)]},
    {"name": "IN_CenterView", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0], "setup": setup(ui_state=u, client={PS + PS_DELTA_ANGLES: struct.pack("<f", x)})}
                for u in (5, 6) for x in (0.0, -0.0, 12.5, float("nan"))]},
    {"name": "IN_LowerStance", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(ui_state=u, client={STANCE: c},
                                            kb={KB["PRONE"]: kbutton(active=p), KB["DOWN"]: kbutton(active=d)})}
                for u in (5, 6) for c in range(4) for p in (0, 1) for d in (0, 1)]},
    {"name": "IN_RaiseStance", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [], "setup": setup(ui_state=u, client={STANCE: c},
                                            kb={KB["PRONE"]: kbutton(active=p), KB["DOWN"]: kbutton(active=d)})}
                for u in (5, 6) for c in range(4) for p in (0, 1) for d in (0, 1)]},
    {"name": "CL_GetLocalClientUIGlobals", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[0], [1]]},
    {"name": "CL_GetLocalClientConnectionState", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": setup(ui_state=u)} for lc in (0, 1) for u in (0, 5, 10)]},
    {"name": "IN_IsTalkKeyHeld", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [], "setup": setup(kb={KB["TALK"]: kbutton(active=a)})} for a in (0, 1)]},
    {"name": "resetBit@?$bitarray@$0EA", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[struct.pack("<II", 0xFFFFFFFF, 0x12345678), b] for b in list(range(64)) + [64, 100]]},
    {"name": "testBit@?$bitarray@$0EA", "ret": "bool", "builds": SERVER, "fakes": FAKES,
     "observe": {"Assert_MyHandler": "siisss"},
     "inputs": [[struct.pack("<II", 0xA5A5F00F, 0x12345678), b] for b in list(range(64)) + [64]]},
]
