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
                0x01237578: 0x7113,  # cl_freelook
                0x01118754: 0x7114,  # cg_airburstStrafeRestriction
                0x011AC530: 0x7115}  # cg_airburstCameraRestriction
LEAN_MIN, HOLD, DELAY, STANDING = 0x7101, 0x7102, 0x7103, 0x7104
STANCE_HOLD, DTP_HOLD, ANGLESPEEDKEY, YAWSPEED, PITCHSPEED, M_FILTER = 0x7105, 0x7106, 0x7107, 0x7108, 0x7109, 0x710A
DAMAGE_KICK, PAUSED = 0x710B, 0x710C
MOUSE_ACCEL, SHOW_RATE, M_SIDE, M_YAW, M_PITCH, M_FORWARD, FREELOOK = range(0x710D, 0x7114)
STRAFE_RESTRICTION, CAMERA_RESTRICTION = 0x7114, 0x7115

FLOAT_DVARS = {ANGLESPEEDKEY, YAWSPEED, PITCHSPEED, MOUSE_ACCEL, M_SIDE, M_YAW, M_PITCH, M_FORWARD,
               STRAFE_RESTRICTION, CAMERA_RESTRICTION}
BOOL_DVARS = {M_FILTER, DAMAGE_KICK, SHOW_RATE, FREELOOK}
DVAR_STORE = {STANDING: 0x14265A860,   # the Store's dvar pointers identified so far (records/names.tsv)
              M_FILTER: 0x140E60788, MOUSE_ACCEL: 0x140E61BF0, M_SIDE: 0x140E61BE8, M_YAW: 0x140E5D238,
              FREELOOK: 0x140E5D108, M_PITCH: 0x140E5CDB8, M_FORWARD: 0x140E5CCD8,
              STRAFE_RESTRICTION: 0x140E4F870, CAMERA_RESTRICTION: 0x140E4F860, DAMAGE_KICK: 0x14265CDC0,
              PAUSED: 0x141F04888}
DVAR_OBJECTS = OBJECTS + 0x1C0000      # one dvar_t per handle, 0x80 apart, in the build's layout


def write_dvars(w, values):
    """Point every dvar global at a dvar_t of the right type holding values.get(handle, 0)."""
    x64 = w.m.x64
    globals_ = {h: va for h, va in DVAR_STORE.items()} if x64 else {h: va for va, h in DVAR_HANDLES.items()}
    for handle, va in globals_.items():
        obj = DVAR_OBJECTS + (handle - 0x7100) * 0x80
        type_ = 2 if handle in FLOAT_DVARS else 1 if handle in BOOL_DVARS else 6
        v = values.get(handle, 0)
        w.write(obj + (0x18 if x64 else 0x10), struct.pack("<i", type_))
        w.write(obj + (0x20 if x64 else 0x18), struct.pack("<f", v) if type_ == 2 else struct.pack("<i", int(v)))
        (w.u64 if x64 else w.u32)(va, obj)


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

PREDICTED_PS = OBJECTS + 0x100000 + 0x480A8  # cg_t.predictedPlayerState (CG, below)
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
            "players_kb": 0x143C0C790, "cls_frametime": 0x142CB7DD0},
}


# clientActive_t fields by name, per architecture (client.h's static_asserts)
FIELDS = {"x86": {"stance": STANCE, "stance_held": STANCE_HELD, "stance_position": STANCE_POSITION,
                  "stance_time": STANCE_TIME, "using_ads": USING_ADS, "ps": PS, "was_in_vehicle": WAS_IN_VEHICLE,
                  "use_count": USE_COUNT, "use_time": USE_TIME, "switching_to_inventory": SWITCHING_TO_INVENTORY},
          "x64": {"stance": 0x2BE8, "stance_held": 0x2BE4, "stance_position": 0x2BEC, "stance_time": 0x2BF0,
                  "using_ads": 0x5C, "ps": 0x88, "was_in_vehicle": 0x8A3E0, "use_count": 0x8A3DC,
                  "use_time": 0x8A3D8, "switching_to_inventory": 0x2BFC}}
CG_FIELDS = {"x86": {"weapon_select": CG_WEAPON_SELECT}, "x64": {"weapon_select": 0x6B554}}


def field(x64, key):
    """A clientActive_t field: a name, ("ps", offset in playerState_s), or a server offset."""
    f = FIELDS["x64" if x64 else "x86"]
    if isinstance(key, tuple):
        return f[key[0]] + key[1]
    return f[key] if isinstance(key, str) else key


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
            off = CG_FIELDS["x64" if x64 else "x86"][off] if isinstance(off, str) else off
            w.write(CG + off, v if isinstance(v, bytes) else struct.pack("<i", v))
        w.u32(a["com_frametime"], frame_time)
        w.u32(a["frame_msec"], frame_msec)
        write_dvars(w, STATE["dvars"])
        for button, data in (kb or {}).items():
            w.write(a["players_kb"] + button * KB_SIZE, data)
        for off, v in (client or {}).items():
            w.write(CLIENTS + field(x64, off), v if isinstance(v, bytes) else struct.pack("<i", v))
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


def laze_values():
    """Moves and restrictions for the airburst laze check: each limit just under, at and over, a 0 restriction, and
    NaN and infinite moves."""
    out = []
    nan, inf = float("nan"), float("inf")
    for moves in ((0.0, 0.0, 0.0, 0.0, 127.0), (127.0, 0.0, 0.0, 0.0, 127.0), (30.0, 40.0, 0.0, 0.0, 127.0),
                  (0.0, 0.0, 3.0, -4.0, 1.0), (-127.0, 127.0, 127.0, -127.0, 127.0), (1.0, 1.0, 1.0, 1.0, 0.0),
                  (nan, 0.0, 0.0, 0.0, 127.0), (0.0, 0.0, inf, 0.0, 127.0), (12.5, -7.25, 0.5, 99.0, 63.5)):
        for sr, cr in ((0.5, 0.5), (0.39370078, 5.0), (0.3937008, 0.0), (0.0, 0.0), (5.0, 5.0), (1.4142135, 0.03937)):
            out.append((moves, sr, cr))
    return out


def laze_inputs():
    return [{"args": [("f", v) for v in moves], "setup": setup(dvars={STRAFE_RESTRICTION: sr, CAMERA_RESTRICTION: cr})}
            for moves, sr, cr in laze_values()]


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


def spectator_input(mu):
    """CL_HandleSpectatorInput's fake (the client in EDI, the buttons on the stack): marks the buttons with the client,
    so the call and its arguments show in the buffer."""
    bits = arg(mu, 0)
    STATE["machine"].program_write(bits, struct.pack("<I", 0xC0DE0000 | mu.reg_read(x86.UC_X86_REG_EDI) & 0xFFFF))


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


# The Store build inlines CL_GetMouseMovement, Com_LocalClient_GetControllerIndex, GPad_InUse, Demo_IsPlaying,
# AimAssist_UpdateMouseInput and SetMeleeChargeCmd into CL_MouseMove: its case sets their state (Store addresses,
# records/names.tsv) and fakes only what both builds still call.
S_MOUSE_IN, S_CLIENT_GAME_STATES, S_DEMO_STATE = 0x140E5C7F0, 0x141EF91E0, 0x1425A3308
S_AA_GLOB = 0x142C64990  # aaGlobArray[0]: ps.weaponstate +0x48, initialized +0x6C, prev_button_bits +0x12B0
S_PS, S_FOV_SCALE, S_MAX_PITCH_SPEED, S_MAX_YAW_SPEED, S_VIEWANGLES = 0x88, 0x2C0C, 0x2C10, 0x2C14, 0x2CA4


def argq(mu, i):
    """Argument i of an x64 call, at the callee's entry (RCX, RDX, R8, R9)."""
    return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[i])


def apply_auto_melee(mu):
    """AimAssist_ApplyAutoMelee's fake (x64): turns the output and sets the melee charge from every AimInput field the
    caller sets, so a wrong input shows."""
    dt, pitch, pa, pm, yaw, ya, ym, fa, ra, b0, b1, lc = struct.unpack("<9fIIi", bytes(mu.mem_read(argq(mu, 0), 0x30)))
    out = argq(mu, 1)
    p0, y0 = struct.unpack("<ff", bytes(mu.mem_read(out, 8)))
    mu.mem_write(out, struct.pack("<fffHB", p0 + pitch * 0.25 + dt + pa + pm, y0 - 0.5 + yaw + ya + ym + fa + ra, 0.0,
                                  (b0 ^ b1 ^ lc) & 0xFFFF, 9))


def floorf_import(m, mu):
    import math
    x = struct.unpack("<f", struct.pack("<I", mu.reg_read(x86.UC_X86_REG_XMM0) & 0xFFFFFFFF))[0]
    r = x if math.isnan(x) or math.isinf(x) else float(math.floor(x))
    mu.reg_write(x86.UC_X86_REG_XMM0, struct.unpack("<I", struct.pack("<f", r))[0])


STORE_MOUSE = {
    # GamerProfile_GetFloat is inlined: GamerProfile_GetInt's bits are the sensitivity
    "fakes": {"GamerProfile_GetInt": lambda mu: struct.unpack("<i", struct.pack("<f", STATE.get("sens", 5.0)))[0],
              "CG_ShouldUpdateViewAngles": lambda mu: STATE.get("update_view", 1),
              "AimAssist_UpdateTweakables": lambda mu: None, "AimAssist_UpdateAdsLerp": lambda mu: None,
              "AimAssist_ApplyAutoMelee": apply_auto_melee, "CL_UpdateAirburstMarkDistance": lambda mu: None},
    # the AimInput is the caller's local (x); the Store's CL_UpdateAirburstMarkDistance ignores its local client, so
    # the original leaves rcx as it happens to be (x)
    "observe": {"GamerProfile_GetInt": "ii", "CG_ShouldUpdateViewAngles": "i", "AimAssist_UpdateTweakables": "xi",
                "AimAssist_UpdateAdsLerp": "x", "AimAssist_ApplyAutoMelee": "xx",
                "CL_UpdateAirburstMarkDistance": "xqqfffff"},
    # the recon calls floorf; the Store inlines it
    "imports": {"api-ms-win-crt-math-l1-1-0.dll!floorf": (floorf_import, 0)},
}


def store_mouse_inputs(n=400, rng=rng, dvars_override=None, frame_msec=None, finite=False):
    """The Store's CL_MouseMove inputs. The enhanced build's checks (tests/enhanced/test_mouse.py) pass their own rng,
    dvars that override the drawn ones, a fixed frame_msec, and finite (no NaN scales or caps)."""
    out = []
    nan = 1.0 if finite else float("nan")
    for _ in range(n):
        kb = {KB["STRAFE"]: kbutton(active=rng.random() < 0.25), KB["MLOOK"]: kbutton(active=rng.random() < 0.5)}
        client = {S_PS + PS_PM_FLAGS: rng.choice([0, 0, 0, 0x800]),
                  S_PS + PS_WEAPONSTATE: rng.choice([0, 0, 0, 0x2C]),
                  S_PS + PS_OTHER_FLAGS: rng.choice([0, 0, 0, 0x400]),
                  S_PS + PS_EFLAGS2: rng.choice([0, 0, 0, 0x40000]),
                  S_PS + PS_PM_TYPE: rng.choice([0, 1, 1]), S_PS + PS_LINK_FLAGS: rng.choice([0, 1]),
                  # a NaN scale or cap: what /fp:fast changed (cl_input.cpp, MOUSE_CAP_OUT_OF_RANGE)
                  S_FOV_SCALE: struct.pack("<f", rng.choice([1.0, rng.uniform(0.3, 1.5), 1.0, nan if rng.random() < 0.03 else 1.0])),
                  S_MAX_YAW_SPEED: struct.pack("<f", rng.choice([0.0, -5.0, 90.0, 400.0, 1e-3, nan, rng.uniform(1, 500)])),
                  S_MAX_PITCH_SPEED: struct.pack("<f", rng.choice([0.0, 60.0, 720.0, nan, rng.uniform(1, 500)])),
                  S_VIEWANGLES: struct.pack("<ff", rng.uniform(-80, 80), rng.uniform(-180, 180))}
        mouse = struct.pack("<5i", rng.choice([0, 3, -40, 1000, rng.randrange(-500, 500)]), rng.choice([0, -2, 15]),
                            rng.choice([0, 9, -300, rng.randrange(-500, 500)]), rng.choice([0, 4]), rng.randrange(2))
        cmd = struct.pack("<III", 0, rng.getrandbits(32), rng.getrandbits(32)) + bytes(0x18) + \
            bytes([rng.randrange(256) for _ in range(5)]) + bytes(0x3C - 0x29)
        controller_lc = rng.choice([0, 0, 0, 1])  # a record for another local client: no controller
        last_input = rng.choice([1, 1, 2, 3, 0])  # 0: the gamepad, no aim assist from the mouse
        globals_ = {S_MOUSE_IN: mouse,
                    S_CLIENT_GAME_STATES: struct.pack("<6i", 0, controller_lc, rng.choice([0, 2]), 0, 0, last_input),
                    S_DEMO_STATE: rng.choice([0, 0, 0, 1, 2]),
                    # AimOutput's melee fields are only set by AimAssist_ApplyAutoMelee: skipping it (aim assist not
                    # initialized, or weapon states 0x38-0x3A) leaves them uninitialized (records/bugs.tsv B0016)
                    S_AA_GLOB + 0x48: rng.choice([0, 0, 0x2C, 0x37, 0x38, 0x3A, 0x3B]),
                    S_AA_GLOB + 0x6C: struct.pack("<B", rng.random() < 0.85),
                    S_AA_GLOB + 0x12B0: b"\xAA" * 8}
        out.append({"args": [0, cmd, ("f", rng.choice([16.0, 6.9, 33.3]))],
                    "setup": setup(kb=kb, client=client, globals_=globals_,
                                   frame_msec=frame_msec if frame_msec is not None else
                                   rng.choice([0, 1, 7, 16, 33, rng.randrange(2, 60)]),
                                   dvars={M_FILTER: rng.randrange(2), MOUSE_ACCEL: rng.choice([0.0, 0.5, rng.uniform(0, 3)]),
                                          M_SIDE: rng.choice([0.25, rng.uniform(0.01, 1)]), M_YAW: rng.choice([0.022, rng.uniform(0.5, 5)]),
                                          M_PITCH: rng.choice([0.022, -0.022, 0.0, rng.uniform(-5, 5)]),
                                          M_FORWARD: rng.choice([0.25, rng.uniform(0.01, 1)]),
                                          FREELOOK: rng.choice([1, 1, 0]), **(dvars_override or {})},
                                   sens=rng.choice([5.0, rng.uniform(0.5, 20)]),
                                   update_view=rng.random() < 0.8)})
    return out


def store_mouse_edges():
    """What the random inputs rarely reach: a strafe or forward move just below a half (floorf(x + 0.5f) rounds the
    sum up where the server's rounding doesn't) and NaN movement (from the FOV scale), which /fp:fast treats as none
    when both axes are NaN and turns by differently when one is."""
    out = []
    nan = float("nan")
    for sens in (0.49999997, -0.50000006, 1.4999999, -2.5, 2.5, 0.5, -0.49999997, 126.49999):
        for strafe, mlook in ((1, 0), (0, 0)):
            for fov, mouse in ((1.0, (1, 0, 1, 0)), (nan, (1, 0, 1, 0)), (float("inf"), (0, 0, 1, 0)),
                               (float("inf"), (1, 0, 0, 0))):
                kb = {KB["STRAFE"]: kbutton(active=strafe), KB["MLOOK"]: kbutton(active=mlook)}
                client = {S_FOV_SCALE: struct.pack("<f", fov), S_MAX_YAW_SPEED: struct.pack("<f", 90.0),
                          S_MAX_PITCH_SPEED: struct.pack("<f", 90.0), S_VIEWANGLES: struct.pack("<ff", 10.0, 20.0)}
                cmd = struct.pack("<III", 0, 1, 2) + bytes(0x18) + bytes([5, 250, 0, 3, 4]) + bytes(0x3C - 0x29)
                # an infinite scale on a zero axis makes that axis NaN and the other infinite: the move goes ahead
                # with a NaN turn (MOUSE_MIN_127)
                globals_ = {S_MOUSE_IN: struct.pack("<5i", *mouse, 0),
                            S_CLIENT_GAME_STATES: struct.pack("<6i", 0, 0, 0, 0, 0, 1),
                            S_AA_GLOB + 0x6C: struct.pack("<B", 1)}
                out.append({"args": [0, cmd, ("f", 16.0)],
                            "setup": setup(kb=kb, client=client, globals_=globals_, frame_msec=16,
                                           dvars={M_SIDE: 1.0, M_FORWARD: 1.0, M_YAW: 0.022, M_PITCH: 0.022,
                                                  FREELOOK: 0},
                                           sens=sens, update_view=1)})
    return out


# CL_CreateCmd on the Store (140194E20): CL_AdjustAngles, Com_ViewScaleMsec, GPad_IsActive, the catcher tests,
# CL_HandleSpecialStateInput, AngleDelta, the demo camera's clamps (Demo_ClampViewAngleForLockedOnEntity with
# CG_GetEntity and the AI lock-on lookup) and CL_CopyDamageKickAngles are inlined, so the case sets their state.
S_CLS = 0x142CB7C80              # frametime +0x150, frametime_base +0x154
S_SLOWMO = 0x141F02F10           # g_slowmoCommon
S_GAMEPADS = 0x14420D5A0         # GamePad[], 0x7C apart: enabled
S_SYSTEM_UI = 0x1443C54F8        # 0x24 bytes per controller: active
S_DEMO = 0x1425A3280             # state +0x88, playback +0xB0680, playbackInited +0xB0688
S_CG_ENTITIES = 0x140E4F8E0      # cg_entitiesArray[0]
S_AI_TYPES = 0x140E59268         # cg_level.aiTypes
S_CG_DAMAGE_KICK, S_CG_PM_TYPE, S_CG_OTHER_FLAGS = 0x64B54, 0x480AC, 0x480C0
S_CAMERA_MODE = OBJECTS + 0x188000          # demoPlayback (0x26A980 bytes) placed so its fields land in the
S_PLAYBACK = S_CAMERA_MODE - 0xD4C00        # objects region: cameraMode, then the lock at +0xEBD34
S_ENTITIES, S_ENTITY = OBJECTS + 0x1A0000, 0x3D8
S_AI_OBJECTS = OBJECTS + 0x1B0000           # AITypeScript objects (demoLockOn at +0x48), 0x80 apart
NAN = float("nan")


def keystate(mu):
    """CL_KeyState's fake: each button's fraction from STATE (by its index in playersKb)."""
    i = (argq(mu, 0) - 0x143C0C790) // KB_SIZE
    return STATE.get("keys", {}).get(i, 0.0)


def store_mouse_move(mu):
    """CL_MouseMove's fake (x64): turns the pitch by STATE['dpitch'] and marks the command (rightmove)."""
    va = CLIENTS + S_VIEWANGLES
    pitch = struct.unpack("<f", bytes(mu.mem_read(va, 4)))[0]
    STATE["machine"].program_write(va, struct.pack("<f", pitch + STATE.get("dpitch", 0.0)))
    STATE["machine"].program_write(argq(mu, 1) + 0x25, b"\x55")


def store_mark(offset, value):
    """A fake that writes value at offset in the command (its second argument), to show it ran on this command."""
    return lambda mu: STATE["machine"].program_write(argq(mu, 1) + offset, value)


def angle_normalize(mu):
    import math
    x = struct.unpack("<f", struct.pack("<I", mu.reg_read(x86.UC_X86_REG_XMM0) & 0xFFFFFFFF))[0]
    return x if math.isnan(x) or math.isinf(x) else math.fmod(x, 360.0) + 0.25


STORE_CREATE_CMD = {
    "fakes": {"CL_KeyState": keystate, "CG_HandleLocationSelectionInput": lambda mu: STATE.get("location", 0),
              "CL_CmdButtons": store_mark(0x24, b"\x11"), "CL_KeyMove": store_mark(0x26, b"\x22"),
              "CL_MouseMove": store_mouse_move, "CL_GamepadMove": store_mark(0x27, b"\x33"),
              "CL_HandleSpectatorInput": store_mark(0, b"\x44\x44"),
              "AngleNormalize360": angle_normalize, "CL_AddCurrentStanceToCmd": store_mark(0x28, b"\x66"),
              "CL_FinishMove": store_mark(0, b"\x21\x43\x00\x00"),
              "Com_IsDedicatedServer": lambda mu: STATE.get("dedicated", 0), "Com_Error": lambda mu: None},
    # the command may be the caller's buffer or a local copied into it (x)
    "observe": {"CL_KeyState": "q", "CG_HandleLocationSelectionInput": "ix", "CL_CmdButtons": "ix",
                "CL_KeyMove": "ix", "CL_MouseMove": "ixf", "CL_GamepadMove": "ixf", "CL_HandleSpectatorInput": "ix",
                "AngleNormalize360": "f", "CL_AddCurrentStanceToCmd": "ix", "CL_FinishMove": "ix",
                "Com_IsDedicatedServer": "", "Com_Error": "is"},
    "imports": STORE_MOUSE["imports"],  # the recon's AngleDelta calls floorf; the Store inlines it
}


def store_world(w, ents, ai_views):
    w.u64(S_CG_ENTITIES, S_ENTITIES)
    for index, views in ai_views.items():
        obj = S_AI_OBJECTS + index * 0x80
        w.write(obj + 0x48, struct.pack("<f", 300.0) + b"".join(struct.pack("<iff", 5, lo, hi) for lo, hi in views))
        w.u64(S_AI_TYPES + 8 * index, obj)
    for i, (etype, valid, ai) in enumerate(ents):
        base = S_ENTITIES + i * S_ENTITY
        w.write(base + 0x25E, struct.pack("<H", ai))
        w.write(base + 0x2CC, struct.pack("<h", etype))
        w.write(base + 0x3CC, struct.pack("<I", 0x2 if valid else 0x1))


# entity 0-6: an actor with lock-on views, one whose AI type is unset, one with no type, a helicopter, another
# kind, an invalid actor, an actor whose type's low byte is set (0x103)
STORE_ENTITIES = ((16, 1, 3), (16, 1, 7), (16, 1, 0), (12, 1, 0), (1, 1, 3), (16, 0, 3), (16, 1, 0x103))
STORE_AI_VIEWS = {3: ((-20.0, 45.0), (-5.0, 10.0))}


def store_create_cmd_setup(r, pitch, dpitch, demo_state, inited, mode, locked, active, finite):
    nan = 1.0 if finite else NAN
    client = {S_PS + PS_PM_FLAGS: r.choice([0, 0, 0, 0x800]),
              S_MAX_YAW_SPEED: struct.pack("<f", r.choice([0.0, -3.0, 90.0, r.uniform(1, 400), nan])),
              S_MAX_PITCH_SPEED: struct.pack("<f", r.choice([0.0, 60.0, r.uniform(1, 400), nan])),
              S_VIEWANGLES: struct.pack("<ff", pitch, r.uniform(-180, 180))}
    kb = {KB["SPEED"]: kbutton(active=r.random() < 0.3), KB["STRAFE"]: kbutton(active=r.random() < 0.3)}
    keys = {KB[k]: r.choice([0.0, 0.0, 1.0, 0.5, r.random()]) for k in ("LEFT", "RIGHT", "LOOKUP", "LOOKDOWN")}
    start, end = r.choice([(1.0, 0.25), (0.25, 1.0), (0.5, 0.5), (r.uniform(0, 2), r.uniform(0, 2)), (nan, 1.0)])
    slowmo = struct.pack("<fB3xiffii", r.choice([1.0, r.uniform(0, 2), 0.24, 1.01, nan]), r.randrange(2), 0, start,
                         end, 0, r.choice([0, 300, 0, -5]))
    ctrl_lc = r.choice([0, 0, 0, 1])  # another client's record: no controller (index -1)
    globals_ = {S_CLS + 0x150: struct.pack("<if", r.choice([16, 33, 0, r.randrange(1, 100)]),
                                           r.choice([0.016, 0.033, 0.0, 1e-4, r.uniform(0, 0.1)])),
                S_SLOWMO + 0x1C: slowmo,
                S_CLIENT_GAME_STATES: struct.pack("<6i", 0, ctrl_lc, 0, 0, 0, 1),
                S_GAMEPADS: struct.pack("<B", r.random() < 0.3),
                S_SYSTEM_UI: struct.pack("<B", r.random() < 0.1),
                0x140E61794: struct.pack("<i", r.choice([0, 0, 0, 4, 0x10, 0x14, 0x40])),  # keyCatchers
                S_DEMO + 0x88: struct.pack("<i", demo_state), S_DEMO + 0xB0680: struct.pack("<Q", S_PLAYBACK),
                S_DEMO + 0xB0688: struct.pack("<B", inited), S_CAMERA_MODE: struct.pack("<i", mode),
                S_PLAYBACK + 0xEBD34: struct.pack("<iB", locked, active)}
    cg = {S_CG_DAMAGE_KICK: struct.pack("<ff", r.choice([2.5, 0.0, -179.9]), r.choice([-1.0, 0.0055, 200.0])),
          S_CG_PM_TYPE: r.choice([0, 1, 5]), S_CG_OTHER_FLAGS: r.choice([0, 0, 2, 3])}
    base = setup(kb=kb, client=client, globals_=globals_, cg=cg, dvars={DAMAGE_KICK: r.randrange(2)},
                 keys=keys, location=r.random() < 0.5, dpitch=dpitch, dedicated=r.random() < 0.2)

    def run(w):
        base(w)
        store_world(w, STORE_ENTITIES, STORE_AI_VIEWS)
    return run


def store_create_cmd_inputs(n=600, finite=False):
    r = random.Random(2794)
    out = []
    pitches = ((10.0, 5.0), (10.0, 95.0), (10.0, -95.0), (350.0, 100.0), (0.0, 90.0), (200.0, 0.0),
               (-170.0, -30.0), (89.0, 2.0), (-89.0, -2.0), (179.0, 3.0), (60.0, -200.0))
    for _ in range(n):
        pitch, dpitch = r.choice(pitches) if r.random() < 0.6 else (r.uniform(-400, 400), r.uniform(-200, 200))
        if not finite and r.random() < 0.03:
            pitch = NAN
        demo_state = r.choice([2, 2, 2, 0, 1])
        inited, mode = r.choice([1, 1, 1, 0]), r.choice([0, 1, 2, 2])
        locked, active = r.choice([(-1, 1), (0, 1), (0, 0)] + [(i, 1) for i in range(1, len(STORE_ENTITIES))])
        out.append({"args": [bytes([0xCC]) * 0x3C, 0],
                    "setup": store_create_cmd_setup(r, pitch, dpitch, demo_state, inited, mode, locked, active,
                                                    finite)})
    return out


def store_create_cmd(mu):
    """CL_CreateCmd's fake (x64): a command made from the client's cmdNumber and STATE, in the hidden result buffer."""
    out = argq(mu, 0)
    n = struct.unpack("<i", bytes(mu.mem_read(CLIENTS + 0x44AC4, 4)))[0]
    STATE["machine"].program_write(out, struct.pack("<IiI", STATE.get("server_time", 4321) & 0xFFFFFFFF, n, 0xA5A5A5A5) +
                                   bytes(range(0x30)))
    return out


STORE_NEW_COMMANDS = {
    "fakes": {"CL_CreateCmd": store_create_cmd, "MSG_SetDefaultUserCmd": lambda mu: None},
    "observe": {"CL_CreateCmd": "xi", "MSG_SetDefaultUserCmd": "qq"},
    "inputs": [{"args": [0], "setup": setup(ui_state=u, client={0x44AC4: n}, globals_={0x140E617A0: mig},
                                            server_time=n * 7)}
               for u in (8, 9, 10) for n in (0, 126, 127, 0x7FFFFFFF) for mig in range(4)],
}


STORE_INPUT = {
    # CL_SplitscreenEnabled is false and CL_IsFirstActiveLocalClient inlined: cl_paused is still read
    "fakes": {"IN_Frame": lambda mu: None, "CL_CreateNewCommands": lambda mu: None},
    "observe": {"IN_Frame": "", "CL_CreateNewCommands": "i"},
    "inputs": [{"args": [0], "setup": setup(ui_state=u, dvars={PAUSED: pz}, globals_={0x140E61790: fl})}
               for u in (9, 10, 11) for pz in (0, 1) for fl in (0, 1)],
}


CASES = [
    {"name": "CL_MouseMove", "ret": "void", "builds": BOTH, "fakes": MOUSE_FAKES, "observe": MOUSE_SEEN,
     "inputs": mouse_inputs(), "by_build": {"t6mp_msstore": dict(STORE_MOUSE, inputs=store_mouse_inputs() + store_mouse_edges())}},
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
    {"name": "CG_IsSprinting", "ret": "bool", "builds": BOTH, "fakes": FAKES, "observe": SEEN,
     # the Store build inlines CG_GetPredictedPlayerState and Dvar_GetInt, and takes no argument (ignores rcx)
     "by_build": {"t6mp_msstore": {"fakes": {}, "observe": {}}},
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
    {"name": "CG_GetPredictedPlayerState", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": setup(cg_null=n)} for lc in (0, 1) for n in (False, True)]},
    {"name": "CG_GetLocalClientGlobals", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": setup(cg_null=n)} for lc in (0, 1) for n in (False, True)]},
    {"name": "IN_IsTalkKeyHeld", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [], "setup": setup(kb={KB["TALK"]: kbutton(active=a)})} for a in (0, 1)]},
    {"name": "CG_AirburstLazeCancelledByMovement", "ret": "int", "builds": SERVER, "inputs": laze_inputs()},
    {"name": "CL_UpdateAirburstMarkDistance", "ret": "void", "builds": BOTH, "fakes": FAKES, "observe": SEEN,
     "by_build": {"t6mp_msstore": {
         # the Store inlines the laze check: its dvars are set, and only CG_GetAirburstMarkDistance is faked (its
         # local client isn't passed: x)
         "fakes": {"CG_GetAirburstMarkDistance": lambda mu: airburst(mu)},
         "observe": {"CG_GetAirburstMarkDistance": "xqqxi"},
         "inputs": [{"args": [0, CLIENTS + 0x88, bytes(0x3C)] + [("f", v) for v in moves],
                     "setup": setup(mark=m, dvars={STRAFE_RESTRICTION: sr, CAMERA_RESTRICTION: cr})}
                    for moves, sr, cr in laze_values() for m in (None, 1234)]}},
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
    {"name": "CL_KeyMove", "ret": "void", "builds": BOTH, "fakes": FAKES, "observe": SEEN,
     # the Store inlines everything CL_KeyMove calls but CL_KeyState and CG_IsSprinting
     "by_build": {"t6mp_msstore": {"fakes": {}, "observe": {}}},
     "inputs": [{"args": [0, struct.pack("<III", 0, rng.getrandbits(32), rng.getrandbits(32)) + bytes(0x38)],
                 "setup": setup(kb={b: random_kbutton() for b in range(NUM_BUTTONS)},
                                client={("ps", PS_EFLAGS): rng.choice([0, 0x4000]),
                                        "was_in_vehicle": rng.randrange(2), "using_ads": bytes([rng.randrange(2)]),
                                        "use_count": rng.choice([0, 1, 2, 2]), "use_time": 9000,
                                        "stance_held": rng.randrange(2), "stance": rng.randrange(4),
                                        "stance_position": rng.randrange(4),
                                        # either side of the 200 and 300 ms holds
                                        "stance_time": rng.choice([9500, 9750, 9801, 9850, 9700, 9701]),
                                        "switching_to_inventory": bytes([rng.randrange(2)]),
                                        ("ps", PS_WEAPON): rng.choice([5, 7]),
                                        ("ps", PS_INVENTORY_WEAPON): rng.choice([5, 7])},
                                cg={"weapon_select": rng.choice([5, 7])},
                                ps={PS_VIEWHEIGHT: struct.pack("<f", 61.0), PS_PM_FLAGS: rng.choice([0, 0x8000])},
                                frame_time=10000, frame_msec=rng.choice([7, 16, 33, 127]),
                                dvars={STANDING: 60, STANCE_HOLD: 300, DTP_HOLD: 200})}
                for _ in range(300)]},
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
    {"name": "CL_HandleSpecialStateInput", "ret": "void", "builds": SERVER,
     "fakes": dict(FAKES, CL_HandleSpectatorInput=spectator_input), "observe": SEEN,
     "inputs": [{"args": [0, bytes(range(8))], "setup": setup(ps={PS_PM_TYPE: t, PS_OTHER_FLAGS: f}, cg_null=null)}
                for t in (0, 1, 4, 5, 6) for f in (0, 1, 2, 3, 0x400) for null in (False,)]},
    {"name": "CL_CopyDamageKickAngles", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [0, bytes(0x44)], "setup": setup(cg={CG_DAMAGE_KICK: struct.pack("<ff", x, y)})}
                for x, y in ((0.0, 0.0), (1.5, -2.25), (179.9, -180.0), (0.0055, -0.0055), (200.0, 359.0))]},
    {"name": "CL_CreateCmd", "ret": "void", "builds": BOTH, "fakes": FAKES, "observe": SEEN,
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
                for kick in (0, 1)],
     "by_build": {"t6mp_msstore": dict(STORE_CREATE_CMD, inputs=store_create_cmd_inputs())}},
    {"name": "CL_CreateNewCommands", "ret": "void", "builds": BOTH, "fakes": FAKES, "observe": SEEN,
     "by_build": {"t6mp_msstore": STORE_NEW_COMMANDS},
     "inputs": [{"args": [0],
                 "setup": setup(ui_state=u, client={CMD_NUMBER: n, VIEWANGLES: struct.pack("<f", 10.0)},
                                globals_={CLIENT_UI + 0x10: mig}, server_time=n * 7)}
                for u in (8, 9, 10) for n in (0, 126, 127, 0x7FFFFFFF) for mig in range(4)]},
    {"name": "CL_Input", "ret": "void", "builds": BOTH, "fakes": FAKES, "observe": SEEN,
     "by_build": {"t6mp_msstore": STORE_INPUT},
     "inputs": [{"args": [0],
                 "setup": setup(ui_state=u, client={CMD_NUMBER: 5}, dvars={PAUSED: pz}, zombies=z, first=f)}
                for u in (9, 10) for pz in (0, 1) for z in (0, 1) for f in (0, 1)]},
    {"name": "resetBit@?$bitarray@$0EA", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[struct.pack("<II", 0xFFFFFFFF, 0x12345678), b] for b in list(range(64)) + [64, 100]]},
    {"name": "testBit@?$bitarray@$0EA", "ret": "bool", "builds": SERVER, "fakes": FAKES,
     "observe": {"Assert_MyHandler": "siisss"},
     "inputs": [[struct.pack("<II", 0xA5A5F00F, 0x12345678), b] for b in list(range(64)) + [64]]},
]
