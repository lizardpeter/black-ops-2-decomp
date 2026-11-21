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
BOTH = ["CoDMPServer_PC", "t6mp_msstore"]

PLAYERS_KB = 0x011C16E0
KB_SIZE, NUM_BUTTONS = 0x18, 45
CLIENT_UI = 0x01237050
CL_MAX_LOCAL_CLIENTS = 0x01237580
CLIENTS_PTR = 0x01237588  # clientActive_t *clients, allocated at startup
CLIENTS = OBJECTS
CG_ARRAY_PTR = 0x011AC548  # cg_t *cgArray, allocated when the cgame starts
CG = OBJECTS + 0x100000
CLS_FRAMETIME = 0x012376EC
MOUSE_IN = 0x011C1B18
COM_FRAMETIME = 0x01E29A28
FRAME_MSEC = 0x01237034
G_LEANED = 0x011C1B2C
ATTACK_EDGE = 0x011C1B30
LATCH = 0x011C1B31
DVAR_HANDLES = {0x011C16DC: 0x7101,  # cl_leanCountMin
                0x0798718C: 0x7102,  # cl_dblTapMaxHoldTime
                0x0798767C: 0x7103,  # cl_dblTapMaxDelayTime
                0x082324F0: 0x7104,  # player_standingViewHeight
                0x07987260: 0x7105,  # cl_stanceHoldTime
                0x079874A4: 0x7106,  # cl_dtpHoldTime
                0x079EF388: 0x7107,  # cl_anglespeedkey
                0x07987340: 0x7108,  # cl_yawspeed
                0x079EF194: 0x7109,  # cl_pitchspeed
                0x012374B0: 0x710A,  # m_filter
                0x08232590: 0x710B,  # bg_useClientDamageKick
                0x01DA7F64: 0x710C,  # cl_paused
                0x01237040: 0x710D,  # cl_mouseAccel
                0x079875E4: 0x710E,  # cl_showMouseRate
                0x012374A8: 0x710F,  # m_side
                0x01232E4C: 0x7110,  # m_yaw
                0x01232E50: 0x7111,  # m_pitch
                0x01237038: 0x7112,  # m_forward
                0x01237578: 0x7113}  # cl_freelook
LEAN_MIN, HOLD, DELAY, STANDING = 0x7101, 0x7102, 0x7103, 0x7104
STANCE_HOLD, DTP_HOLD, ANGLESPEEDKEY, YAWSPEED, PITCHSPEED, M_FILTER = 0x7105, 0x7106, 0x7107, 0x7108, 0x7109, 0x710A
DAMAGE_KICK, PAUSED = 0x710B, 0x710C
MOUSE_ACCEL, SHOW_RATE, M_SIDE, M_YAW, M_PITCH, M_FORWARD, FREELOOK = range(0x710D, 0x7114)

# clientActive_t fields
USING_ADS, SNAP_VALID, PS = 0x44, 0x50, 0x70
CMDS, CMD_NUMBER, CMD_SIZE = 0x42CA8, 0x44EA8, 0x44
SWITCHING_TO_INVENTORY, MAX_PITCH_SPEED, MAX_YAW_SPEED, WAS_IN_VEHICLE = 0x2BE4, 0x2BF8, 0x2BFC, 0x8A7C4
PS_INVENTORY_WEAPON = 0x570
PS_PM_TYPE, PS_OTHER_FLAGS, PS_EFLAGS2, PS_WEAPONSTATE, PS_LINK_FLAGS = 0x4, 0x18, 0x100, 0x1D0, 0x484
FOV_SCALE = 0x2BF4
# cg_t fields
CG_DAMAGE_KICK, CG_WEAPON_SELECT = 0x64A74, 0x69674
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


# Globals per architecture (the x86 server, the x64 Store build: records/names.tsv). A Store-ready case uses only
# these; the server-only cases still name server addresses directly.
ADDR = {
    "x86": {"client_ui": CLIENT_UI, "max_local_clients": CL_MAX_LOCAL_CLIENTS, "clients_ptr": CLIENTS_PTR,
            "cg_array_ptr": CG_ARRAY_PTR, "cls_frametime": CLS_FRAMETIME, "com_frametime": COM_FRAMETIME,
            "frame_msec": FRAME_MSEC, "players_kb": PLAYERS_KB},
    "x64": {"client_ui": 0x140E61790, "max_local_clients": 0x140E5CA9C, "clients_ptr": 0x140E61BF8,
            "cg_array_ptr": 0x140E53A20, "com_frametime": 0x141F04894, "frame_msec": 0x140E5C804,
            "players_kb": 0x143C0C790},
}


# clientActive_t fields by name, per architecture (client.h's static_asserts)
FIELDS = {"x86": {"stance": STANCE, "stance_held": STANCE_HELD, "stance_position": STANCE_POSITION,
                  "stance_time": STANCE_TIME, "using_ads": USING_ADS},
          "x64": {"stance": 0x2BE8, "stance_held": 0x2BE4, "stance_position": 0x2BEC, "stance_time": 0x2BF0,
                  "using_ads": 0x5C}}


def setup(kb=None, client=None, ui_state=10, frame_time=10000, frame_msec=16, dvars=None, ps=None, globals_=None,
          cg=None, cg_null=False, frametime=16, **state):
    """kb: {button index: kbutton bytes}; client: {field offset: bytes or int}; ps: predicted playerState fields."""
    def run(w):
        STATE.clear()
        STATE.update(state)
        STATE["dvars"] = dvars or {}
        STATE["machine"] = w.m
        x64 = w.m.x64
        a = ADDR["x64" if x64 else "x86"]
        w.u32(a["client_ui"] + 8, ui_state)
        w.u32(a["max_local_clients"], 1)
        (w.u64 if x64 else w.u32)(a["clients_ptr"], CLIENTS)
        (w.u64 if x64 else w.u32)(a["cg_array_ptr"], 0 if cg_null else CG)
        if "cls_frametime" in a:
            w.u32(a["cls_frametime"], frametime)
        for off, v in (cg or {}).items():
            w.write(CG + off, v if isinstance(v, bytes) else struct.pack("<i", v))
        w.u32(a["com_frametime"], frame_time)
        w.u32(a["frame_msec"], frame_msec)
        if not x64:
            for va, handle in DVAR_HANDLES.items():
                w.u32(va, handle)
        for button, data in (kb or {}).items():
            w.write(a["players_kb"] + button * KB_SIZE, data)
        for off, v in (client or {}).items():
            off = FIELDS["x64" if x64 else "x86"][off] if isinstance(off, str) else off
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
    "?Dvar_GetFloat@@YAMPBUdvar_t@@@Z": lambda mu: float(STATE["dvars"].get(arg(mu, 0), 0.0)),
    "?Dvar_GetBool@@YA_NPBUdvar_t@@@Z": lambda mu: int(bool(STATE["dvars"].get(arg(mu, 0), 0))),
    "CG_AirburstLazeCancelledByMovement": lambda mu: STATE.get("cancelled", 0),
    "CG_GetAirburstMarkDistance": lambda mu: airburst(mu),
    "DevGui_IsActive": lambda mu: STATE.get("devgui", 0),
    "DevGui_MouseEvent": lambda mu: None,
    "UI_UpdateMouseScreenPosition": lambda mu: None,
    "UI_KeysBypassMenu": lambda mu: STATE.get("bypass", 0),
    "UI_MouseEvent": lambda mu: None,
    "IN_ShowSystemCursor": lambda mu: None,
    "IN_SetCursorPos": lambda mu: None,
    "Live_IsSystemUiActive": lambda mu: STATE.get("system_ui", 0),
    "Key_ClearStates": lambda mu: None,
    "Key_IsCatcherActive": lambda mu: STATE.get("catcher", 0),
    "CG_HandleLocationSelectionInput": lambda mu: STATE.get("location", 0),
    "CL_CmdButtons": lambda mu: None,
    "CL_MouseMove": lambda mu: mouse_move(mu),
    "CL_GamepadMove": lambda mu: None,
    "CL_HandleSpecialStateInput": lambda mu: None,
    "CL_FinishMove": lambda mu: finish_move(mu),
    "Com_ViewScaleMsec": lambda mu: 16.0,
    "Com_LocalClient_GetControllerIndex": lambda mu: 0,
    "GPad_IsActive": lambda mu: STATE.get("gpad", 0),
    "Demo_IsThirdPersonCamera": lambda mu: STATE.get("third", 0),
    "Demo_IsAnyFreeMoveCamera": lambda mu: STATE.get("free", 0),
    "Demo_IsFreeCameraLockedOnEntity": lambda mu: STATE.get("locked", 0),
    "Demo_ClampViewAngleForLockedOnEntity": lambda mu: None,
    "CL_ClampViewAngle": lambda mu: None,
    "MSG_SetDefaultUserCmd": lambda mu: None,
    "CL_IsFirstActiveLocalClient": lambda mu: STATE.get("first", 1),
    "IN_Frame": lambda mu: None,
    "Com_SessionMode_IsZombiesGame": lambda mu: STATE.get("zombies", 0),
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
    "?Dvar_GetInt@@YAHPBUdvar_t@@@Z": "q", "?Dvar_GetFloat@@YAMPBUdvar_t@@@Z": "q", "?Dvar_GetBool@@YA_NPBUdvar_t@@@Z": "q",
    "ClampChar": "i", "CG_AirburstLazeCancelledByMovement": "fffff", "CG_GetAirburstMarkDistance": "iqqxi",
    "DevGui_IsActive": "", "DevGui_MouseEvent": "ii", "UI_UpdateMouseScreenPosition": "iii", "UI_KeysBypassMenu": "i",
    "UI_IsFocusedMenuUi3dWindow": "i",
    "Live_IsSystemUiActive": "", "Key_ClearStates": "i", "Key_IsCatcherActive": "ii",
    "CG_HandleLocationSelectionInput": "i", "CL_CmdButtons": "i", "CL_MouseMove": "ixf", "CL_GamepadMove": "if",
    # the command is on the caller's stack when CL_CreateNewCommands makes it: its address isn't compared (x), the
    # fakes mark it instead
    "CL_HandleSpecialStateInput": "ix", "CL_FinishMove": "ix", "Com_ViewScaleMsec": "f",
    "Com_LocalClient_GetControllerIndex": "i", "GPad_IsActive": "i", "Demo_IsThirdPersonCamera": "",
    "Demo_IsAnyFreeMoveCamera": "", "Demo_IsFreeCameraLockedOnEntity": "", "Demo_ClampViewAngleForLockedOnEntity": "i",
    "CL_ClampViewAngle": "iiff", "MSG_SetDefaultUserCmd": "qq", "CL_IsFirstActiveLocalClient": "i", "IN_Frame": "",
    "Com_SessionMode_IsZombiesGame": "", "CL_SplitscreenEnabled": "", "UI_MouseEvent": "iii", "IN_ShowSystemCursor": "i", "IN_SetCursorPos": "ii", "CG_GetPredictedPlayerState": "i", "Cmd_Argv": "i",
    "Com_Printf": "is", "BG_GetWeaponDef": "i", "BG_GetFireType": "i", "BG_WeaponBlocksProne": "i",
    "CG_CycleWeapon": "ii", "Assert_MyHandler": "siiss", "CL_GetLocalClientGlobals": "i",
}


def airburst(mu):
    """CG_GetAirburstMarkDistance's fake: writes STATE['mark'] through its pointer when it reports a mark."""
    if STATE.get("mark") is None:
        return 0
    mu.mem_write(arg(mu, 3), struct.pack("<I", STATE["mark"]))  # the caller's local
    return 1


def mouse_move(mu):
    """CL_MouseMove's fake: turns the view's pitch by STATE['dpitch'] (to exercise CL_CreateCmd's 90-degree limit)
    and marks the command."""
    va = CLIENTS + VIEWANGLES
    pitch = struct.unpack("<f", bytes(mu.mem_read(va, 4)))[0]
    STATE["machine"].program_write(va, struct.pack("<f", pitch + STATE.get("dpitch", 0.0)))
    cmd = arg(mu, 1)
    STATE["machine"].program_write(cmd + 0x2A, struct.pack("<h", 0x1234))  # gunPitch, as a marker


def aim_assist(mu):
    """AimAssist_UpdateMouseInput's fake: an output that depends on every input field the caller sets."""
    inp = bytes(mu.mem_read(arg(mu, 0), 0x30))
    dt, pitch, pa, pm, yaw, ya, ym, fa, ra, b0, b1, lc = struct.unpack("<9fIIi", inp)
    out = struct.pack("<fffHB", pitch * 0.5 + dt + pa + pm, yaw - 1.0 + ya + ym + fa + ra, 0.0, (b0 ^ b1 ^ lc) & 0xFFFF, 7)
    mu.mem_write(arg(mu, 1), out)  # the caller's local


def finish_move(mu):
    cmd = arg(mu, 1)
    STATE["machine"].program_write(cmd, struct.pack("<I", STATE.get("server_time", 4321) & 0xFFFFFFFF))


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


MOUSE_FAKES = {k: v for k, v in FAKES.items() if k != "CL_MouseMove"}
MOUSE_FAKES.update({
    "GamerProfile_GetFloat": lambda mu: STATE.get("sens", 5.0),
    "Demo_IsPlaying": lambda mu: STATE.get("demo", 0),
    "GPad_InUse": lambda mu: STATE.get("gpad_in_use", 0),
    "CG_ShouldUpdateViewAngles": lambda mu: STATE.get("update_view", 1),
    "AimAssist_UpdateMouseInput": lambda mu: aim_assist(mu),
    "Com_Printf": lambda mu: None,
})
MOUSE_SEEN = dict(SEEN, GamerProfile_GetFloat="ii", Demo_IsPlaying="", GPad_InUse="i", CG_ShouldUpdateViewAngles="i",
                  AimAssist_UpdateMouseInput="xx", Com_Printf="is")


def mouse_inputs(n=400):
    out = []
    for _ in range(n):
        kb = {KB["STRAFE"]: kbutton(active=rng.random() < 0.25), KB["MLOOK"]: kbutton(active=rng.random() < 0.5)}
        client = {PS + PS_PM_FLAGS: rng.choice([0, 0, 0, 0x800]),
                  PS + PS_WEAPONSTATE: rng.choice([0, 0, 0, 0x2C]),
                  PS + PS_OTHER_FLAGS: rng.choice([0, 0, 0, 0x400]),
                  PS + PS_EFLAGS2: rng.choice([0, 0, 0, 0x40000]),
                  PS + PS_PM_TYPE: rng.choice([0, 1, 1]), PS + PS_LINK_FLAGS: rng.choice([0, 1]),
                  FOV_SCALE: struct.pack("<f", rng.choice([1.0, rng.uniform(0.3, 1.5)])),
                  MAX_YAW_SPEED: struct.pack("<f", rng.choice([0.0, -5.0, 90.0, 400.0, 1e-3, rng.uniform(1, 500)])),
                  MAX_PITCH_SPEED: struct.pack("<f", rng.choice([0.0, 60.0, 720.0, rng.uniform(1, 500)])),
                  VIEWANGLES: struct.pack("<ff", rng.uniform(-80, 80), rng.uniform(-180, 180)),
                  0x2C80 + 0x0: 0}
        mouse = struct.pack("<5i", rng.choice([0, 3, -40, 1000, rng.randrange(-500, 500)]), rng.choice([0, -2, 15]),
                            rng.choice([0, 9, -300, rng.randrange(-500, 500)]), rng.choice([0, 4]), rng.randrange(2))
        cmd = struct.pack("<III", 0, rng.getrandbits(32), rng.getrandbits(32)) + bytes(0x18) + \
            bytes([rng.randrange(256) for _ in range(5)]) + bytes(0x44 - 0x29)
        out.append({"args": [0, cmd, ("f", rng.choice([16.0, 6.9, 33.3]))],
                    "setup": setup(kb=kb, client=client, globals_={MOUSE_IN: mouse},
                                   frame_msec=rng.choice([0, 1, 7, 16, 33, rng.randrange(2, 60)]),
                                   dvars={M_FILTER: rng.randrange(2), MOUSE_ACCEL: rng.choice([0.0, 0.5, rng.uniform(0, 3)]),
                                          SHOW_RATE: rng.randrange(2), M_SIDE: rng.choice([0.25, rng.uniform(0.01, 1)]), M_YAW: rng.choice([0.022, rng.uniform(0.5, 5)]),
                                          M_PITCH: rng.choice([0.022, -0.022, 0.0, rng.uniform(-5, 5)]),
                                          M_FORWARD: rng.choice([0.25, rng.uniform(0.01, 1)]),
                                          FREELOOK: rng.choice([1, 1, 0])},
                                   sens=rng.choice([5.0, rng.uniform(0.5, 20)]), demo=rng.random() < 0.2,
                                   gpad_in_use=rng.random() < 0.2, update_view=rng.random() < 0.8,
                                   mark=None)})
    return out


CASES = [
    {"name": "CL_MouseMove", "ret": "void", "builds": SERVER, "fakes": MOUSE_FAKES, "observe": MOUSE_SEEN,
     "inputs": mouse_inputs()},
    {"name": "I_fmin", "ret": "float", "builds": SERVER,
     "inputs": [[("f", a), ("f", b)] for a in (0.0, -0.0, 1.5, -2.0, float("nan"), float("inf")) for b in (0.0, 1.5, float("nan"), -float("inf"))]},
    {"name": "I_fmax", "ret": "float", "builds": SERVER,
     "inputs": [[("f", a), ("f", b)] for a in (0.0, -0.0, 1.5, -2.0, float("nan"), float("inf")) for b in (0.0, 1.5, float("nan"), -float("inf"))]},
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
    {"name": "CL_KeyState", "ret": "float", "builds": BOTH, "fakes": {"Assert_MyHandler": lambda mu: 1},
     "observe": {"Assert_MyHandler": "siiss"},
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
    {"name": "CL_GetLocalClientGlobals", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": setup(globals_={CL_MAX_LOCAL_CLIENTS: m, CLIENTS_PTR: c})}
                for lc in (0, 1, 2) for m in (0, 1, 2) for c in (0, CLIENTS)]},
    {"name": "CL_GetLocalClientMigrationState", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": setup(globals_={CLIENT_UI + 0x10: m})} for lc in (0, 1) for m in range(4)]},
    {"name": "CG_GetLocalClientGlobals", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": setup(cg_null=n)} for lc in (0, 1) for n in (False, True)]},
    {"name": "IN_IsTalkKeyHeld", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [], "setup": setup(kb={KB["TALK"]: kbutton(active=a)})} for a in (0, 1)]},
    {"name": "CL_UpdateAirburstMarkDistance", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, PREDICTED_PS, struct.pack("<II", 0, 0) * 0x11, ("f", 1.0), ("f", -0.5), ("f", 0.0),
                          ("f", 2.0), ("f", 0.25)],
                 "setup": setup(mark=m, cancelled=c)}
                for m in (None, 0, 1234, 0xFFFF, 0x10000) for c in (0, 1)]},
    {"name": "CL_AdjustAngles", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0],
                 "setup": setup(kb={b: random_kbutton() for b in (KB["LEFT"], KB["RIGHT"], KB["LOOKUP"], KB["LOOKDOWN"],
                                                                  KB["STRAFE"], KB["SPEED"])},
                                client={PS + PS_PM_FLAGS: rng.choice([0, 0, 0x800, 0x801]),
                                        MAX_PITCH_SPEED: struct.pack("<f", rng.choice([0.0, -1.0, 90.0, 140.5])),
                                        MAX_YAW_SPEED: struct.pack("<f", rng.choice([0.0, 120.0, 33.3])),
                                        VIEWANGLES: struct.pack("<ff", rng.uniform(-90, 90), rng.uniform(-360, 360))},
                                frame_time=10000, frame_msec=rng.choice([7, 16, 33]), frametime=rng.choice([7, 16, 33, 100]),
                                dvars={ANGLESPEEDKEY: rng.choice([1.5, 2.0, 0.3]), YAWSPEED: 140.0, PITCHSPEED: 140.0})}
                for _ in range(150)]},
    {"name": "CL_StanceButtonUpdate", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0],
                 "setup": setup(kb={KB["PRONE"]: kbutton(active=tp), KB["DOWN"]: kbutton()},
                                client={STANCE_HELD: h, STANCE: rng.randrange(4), STANCE_POSITION: sp,
                                        STANCE_TIME: t},
                                ps={PS_VIEWHEIGHT: struct.pack("<f", 61.0), PS_PM_FLAGS: f},
                                dvars={STANDING: 60, STANCE_HOLD: 300, DTP_HOLD: 150})}
                for h in (0, 1) for sp in range(4) for t in (9600, 9700, 9800, 9900) for f in (0, 0x8000)
                for tp in (0, 1) if tp == 0 or (sp == 0 and t == 9600)]},
    {"name": "CL_AddCurrentStanceToCmd", "ret": "void", "builds": BOTH, "fakes": {"Assert_MyHandler": lambda mu: 1},
     "observe": {"Assert_MyHandler": "siiss"},
     "inputs": [{"args": [0, struct.pack("<III", 0, b0, b1) + bytes(0x38)], "setup": setup(client={"stance": st})}
                for st in range(5) for b0, b1 in ((0, 0), (0xFFFFFFFF, 0xFFFFFFFF), (0x12345678, 0x9ABCDEF0))]},
    {"name": "CL_WeapNextButtonUpdate", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, bytes(0x44)],
                 "setup": setup(client={SWITCHING_TO_INVENTORY: bytes([sw]), PS + PS_INVENTORY_WEAPON: inv,
                                        PS + PS_WEAPON: cur},
                                cg={CG_WEAPON_SELECT: sel})}
                for sw in (0, 1) for inv in (5, 7) for cur in (5, 9) for sel in (5, 7)]},
    {"name": "CL_KeyMove", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, struct.pack("<III", 0, rng.getrandbits(32), rng.getrandbits(32)) + bytes(0x38)],
                 "setup": setup(kb={b: random_kbutton() for b in range(NUM_BUTTONS)},
                                client={PS + PS_EFLAGS: rng.choice([0, 0x4000]), WAS_IN_VEHICLE: rng.randrange(2),
                                        USING_ADS: bytes([rng.randrange(2)]), USE_COUNT: rng.choice([0, 1, 2, 2]),
                                        USE_TIME: 9000, STANCE_HELD: rng.randrange(2), STANCE: rng.randrange(4),
                                        STANCE_POSITION: rng.randrange(4), STANCE_TIME: 9500,
                                        SWITCHING_TO_INVENTORY: bytes([rng.randrange(2)])},
                                ps={PS_VIEWHEIGHT: struct.pack("<f", 61.0), PS_PM_FLAGS: rng.choice([0, 0x8000])},
                                frame_time=10000, frame_msec=rng.choice([7, 16, 33, 127]),
                                dvars={STANDING: 60, STANCE_HOLD: 300, DTP_HOLD: 150})}
                for _ in range(200)]},
    {"name": "CL_MouseEvent", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, 320, 240, dx, dy],
                 "setup": setup(devgui=d, bypass=b, globals_={CLIENT_UI + 4: kc, MOUSE_IN: struct.pack("<5i", 3, -4, 5, 6, i)})}
                for d in (0, 1) for b in (0, 1) for kc in (0, 0x8, 0x40, 0x11) for dx, dy in ((0, 0), (3, 0), (0, -2))
                for i in (0, 1)]},
    {"name": "CL_GetMouseMovement", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [CLIENTS, bytes(4), bytes(4)],
                 "setup": setup(dvars={M_FILTER: f}, globals_={MOUSE_IN: struct.pack("<5i", *mv, i)})}
                for f in (0, 1) for i in (0, 1) for mv in ((3, -4, 5, 6), (1, 2, -7, 0), (0x7FFFFFF, 3, 0, -1))]},
    {"name": "SetMeleeChargeCmd", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[struct.pack("<fffHB", 1.0, 2.0, 3.0, e, d) + b"\0", bytes(0x44)] for e, d in ((0, 0), (1023, 200))]},
    {"name": "CL_SetCursorPos", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[10, 20], [-1, 0]]},
    {"name": "CL_ShowSystemCursor", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[0], [1]]},
    {"name": "CL_CopyDamageKickAngles", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, bytes(0x44)], "setup": setup(cg={CG_DAMAGE_KICK: struct.pack("<ff", x, y)})}
                for x, y in ((0.0, 0.0), (1.5, -2.25), (179.9, -180.0), (0.0055, -0.0055), (200.0, 359.0))]},
    {"name": "CL_CreateCmd", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [bytes([0xCC]) * CMD_SIZE, 0],
                 "setup": setup(client={VIEWANGLES: struct.pack("<f", pitch), STANCE: 1},
                                dvars={DAMAGE_KICK: kick, STANDING: 60},
                                cg={CG_DAMAGE_KICK: struct.pack("<ff", 2.5, -1.0)},
                                system_ui=sysui, catcher=cat, location=loc, gpad=rng.randrange(2),
                                third=third, free=free, locked=rng.randrange(2), dpitch=dp)}
                for sysui, cat, loc in ((1, 0, 0), (0, 0, 0), (0, 1, 0), (0, 1, 1))
                for pitch, dp in ((10.0, 5.0), (10.0, 95.0), (10.0, -95.0), (350.0, 100.0), (0.0, 90.0),
                                  (200.0, 0.0), (-170.0, -30.0))
                for third, free in ((0, 0), (1, 0), (0, 1))
                for kick in (0, 1)]},
    {"name": "CL_CreateNewCommands", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0],
                 "setup": setup(ui_state=u, client={CMD_NUMBER: n, VIEWANGLES: struct.pack("<f", 10.0)},
                                globals_={CLIENT_UI + 0x10: mig}, server_time=n * 7)}
                for u in (8, 9, 10) for n in (0, 126, 127, 0x7FFFFFFF) for mig in range(4)]},
    {"name": "CL_Input", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0],
                 "setup": setup(ui_state=u, client={CMD_NUMBER: 5}, dvars={PAUSED: pz}, zombies=z, first=f)}
                for u in (9, 10) for pz in (0, 1) for z in (0, 1) for f in (0, 1)]},
    {"name": "resetBit@?$bitarray@$0EA", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[struct.pack("<II", 0xFFFFFFFF, 0x12345678), b] for b in list(range(64)) + [64, 100]]},
    {"name": "testBit@?$bitarray@$0EA", "ret": "bool", "builds": SERVER, "fakes": FAKES,
     "observe": {"Assert_MyHandler": "siisss"},
     "inputs": [[struct.pack("<II", 0xA5A5F00F, 0x12345678), b] for b in list(range(64)) + [64]]},
]
