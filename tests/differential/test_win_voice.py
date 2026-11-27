"""win32/win_voice.cpp.

The Windows mixer (winmm.dll) is modelled in the emulated thread's TLS block, which the harness journals and compares
like the images' globals: each test starts from a set of mixers and checks what both runs leave in them.
"""
import random
import struct

from unicorn import x86_const as x86

from harness import TLS_BLOCK

rng = random.Random(2012)
STATE = {}
SERVER = ["CoDMPServer_PC"]  # the setups write the server's globals at their addresses

# CoDMPServer_PC globals (MAP) and statics (analysis/CoDMPServer_PC/pdb/statics.tsv)
G = {"winvoice_mic_mute": 0x07ACD71C, "winvoice_mic_reclevel": 0x07ACD614, "winvoice_save_voice": 0x07ACD774,
     "winvoice_mic_scaler": 0x07ACD720, "winvoice_loopback": 0x07ACD724, "winvoice_voice_level": 0x07ACD610,
     "g_voice_initialized": 0x07ACD880, "g_voice_record_initialized": 0x07ACD884,
     "g_decode_frame_size": 0x01CF5D90, "g_current_bandwidth_setting": 0x01CF5DE4,
     "s_clientSamples": 0x07ACD728, "s_clientTalkTime": 0x07ACD778, "s_waveInLineInfo": 0x07ACD7C0,
     "wavein_current_reclevel": 0x07ACD60C, "recording": 0x07ACD878, "playing": 0x07ACD879,
     "currentRecordingSample": 0x07ACD87C}
# t6mp_msstore: the saved line table (Mixer_InitWaveInLineInfo 14049F270 clears 0xA0 bytes there)
STORE_G = {"s_waveInLineInfo": 0x144211720}
DVARS = {"winvoice_mic_mute": 0x7001, "winvoice_mic_reclevel": 0x7002, "winvoice_save_voice": 0x7003,
         "winvoice_mic_scaler": 0x7004, "winvoice_loopback": 0x7005, "winvoice_voice_level": 0x7006}

# -- the mixer model ---------------------------------------------------------------------------------------------
# MODEL: u32 mixer count, u32 wave-in device count; then per mixer (0x20 bytes from MODEL + 0x100): has a wave-in
# line, its line id, volume, mute, and failure flags for mixerGetDevCaps, mixerOpen and mixerGetLineControls.
MODEL = TLS_BLOCK + 0x30000
MIXERS = MODEL + 0x100
FIELDS = ("has_line", "line", "volume", "mute", "caps_fail", "open_fail", "controls_fail")
VOLUME, MUTE = 0x50030001, 0x20010002
HANDLE = 0x4000


def u32(mu, va):
    return struct.unpack("<I", bytes(mu.mem_read(va, 4)))[0]


def arg(mu, i):
    """Argument i at the import's entry: stdcall on x86, Win64 registers (the first four: all the mixer API takes)."""
    if STATE.get("x64"):
        return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[i])
    return u32(mu, mu.reg_read(x86.UC_X86_REG_ESP) + 4 + 4 * i)


def ptr(mu, va):
    """A pointer field: 4 bytes on x86, 8 on x64."""
    return struct.unpack("<Q", bytes(mu.mem_read(va, 8)))[0] if STATE.get("x64") else u32(mu, va)


def field(mixer, name):
    return MIXERS + 0x20 * mixer + 4 * FIELDS.index(name)


def result(mu, value):
    mu.reg_write(x86.UC_X86_REG_RAX if STATE.get("x64") else x86.UC_X86_REG_EAX, value)


def mixer_of(mu, handle):
    return handle - HANDLE


def mixerGetNumDevs(m, mu):
    result(mu, u32(mu, MODEL))


def waveInGetNumDevs(m, mu):
    result(mu, u32(mu, MODEL + 4))


def mixerGetDevCapsA(m, mu):
    result(mu, 1 if u32(mu, field(arg(mu, 0), "caps_fail")) else 0)


def mixerOpen(m, mu):
    mixer = arg(mu, 1)
    if mixer >= u32(mu, MODEL) or u32(mu, field(mixer, "open_fail")):
        return result(mu, 2)  # MMSYSERR_BADDEVICEID
    mu.mem_write(arg(mu, 0), struct.pack("<Q" if STATE.get("x64") else "<I", HANDLE + mixer))
    result(mu, 0)


def mixerClose(m, mu):
    result(mu, 0)


def mixerGetLineInfoA(m, mu):
    mixer, line, flags = mixer_of(mu, arg(mu, 0)), arg(mu, 1), arg(mu, 2)
    if not u32(mu, field(mixer, "has_line")):
        return result(mu, 1025)  # MIXERR_INVALLINE
    if flags == 3:  # by component type: the wave-in destination
        mu.mem_write(line + 12, struct.pack("<I", u32(mu, field(mixer, "line"))))
        return result(mu, 0)
    result(mu, 0 if u32(mu, line + 12) == u32(mu, field(mixer, "line")) else 1025)


def mixerGetLineControlsA(m, mu):
    mixer, mxlc = mixer_of(mu, arg(mu, 0)), arg(mu, 1)
    if u32(mu, field(mixer, "controls_fail")):
        return result(mu, 1026)  # MIXERR_INVALCONTROL
    control = u32(mu, mxlc + 8)
    # MIXERLINECONTROLSA.pamxctrl: offset 20 on x86, 24 on x64 (aligned)
    mu.mem_write(ptr(mu, mxlc + (24 if STATE.get("x64") else 20)) + 4,
                 struct.pack("<I", (mixer << 8) | (1 if control == VOLUME else 2)))
    result(mu, 0)


def control_field(mu, mxcd):
    cid = u32(mu, mxcd + 4)
    return field(cid >> 8, "volume" if cid & 0xFF == 1 else "mute")


def details(mu, mxcd):
    """MIXERCONTROLDETAILS.paDetails: offset 20 on x86, 32 on x64 (after the pointer-sized union)."""
    return ptr(mu, mxcd + (32 if STATE.get("x64") else 20))


def mixerGetControlDetailsA(m, mu):
    mxcd = arg(mu, 1)
    mu.mem_write(details(mu, mxcd), struct.pack("<I", u32(mu, control_field(mu, mxcd))))
    result(mu, 0)


def mixerSetControlDetails(m, mu):
    mxcd = arg(mu, 1)
    m.program_write(control_field(mu, mxcd), bytes(mu.mem_read(details(mu, mxcd), 4)))
    result(mu, 0)


def GetDesktopWindow(m, mu):
    result(mu, 0x0D35)


IMPORTS = {f"winmm.dll!{f.__name__}": (f, n) for f, n in [
    (mixerGetNumDevs, 0), (waveInGetNumDevs, 0), (mixerGetDevCapsA, 3), (mixerOpen, 5), (mixerClose, 1),
    (mixerGetLineInfoA, 3), (mixerGetLineControlsA, 3), (mixerGetControlDetailsA, 3), (mixerSetControlDetails, 3)]}
IMPORTS["user32.dll!GetDesktopWindow"] = (GetDesktopWindow, 0)


def mixers(n=None, wavein=1):
    """A random set of mixers: (count, wave-in device count, [per-mixer field values])."""
    n = rng.randrange(0, 6) if n is None else n
    out = []
    for i in range(n):
        out.append({"has_line": int(rng.random() < 0.8), "line": rng.randrange(0x10000, 0x20000),
                    "volume": rng.choice([0, 0x1234, 0x8000, 0xFFFF]), "mute": rng.randrange(2),
                    "caps_fail": int(rng.random() < 0.1), "open_fail": int(rng.random() < 0.1),
                    "controls_fail": int(rng.random() < 0.1)})
    return n, wavein, out


def write_mixers(w, model):
    n, wavein, entries = model
    w.write(MODEL, struct.pack("<II", n, wavein))
    for i, e in enumerate(entries):
        w.write(MIXERS + 0x20 * i, struct.pack("<7I", *(e[k] for k in FIELDS)))


def write_lines(w, lines, table=None):
    """s_waveInLineInfo: up to 8 (isValid, mixerID, dstWaveInLineID, oldRecLevel, oldMuteState)."""
    data = b"".join(struct.pack("<5I", *e) for e in lines)
    w.write(table or G["s_waveInLineInfo"], data.ljust(0xA0, b"\0"))


def store_mixer_setup(model, lines):
    """The Store build: only the mixer model and the line table (its other voice globals aren't identified)."""
    def run(w):
        STATE.clear()
        STATE["x64"] = True
        write_mixers(w, model)
        write_lines(w, lines, STORE_G["s_waveInLineInfo"])
    return run


def lines_for(model):
    """The table Mixer_InitWaveInLineInfo would build, with made-up old settings."""
    out = []
    for i, e in enumerate(model[2]):
        if e["has_line"] and not e["caps_fail"] and not e["open_fail"]:
            out.append((1, i, e["line"], rng.choice([0, 0x4000, 0xFFFF]), rng.randrange(2)))
    return out[:8]


# -- the voice back end (groupvoice/) ----------------------------------------------------------------------------

def counter(key, start):
    def fn(mu):
        STATE[key] = STATE.get(key, start) + 1
        return STATE[key]
    return fn


def dvar_value(kind):
    def fn(mu):
        return STATE["dvars"].get(arg(mu, 0), 0)
    return fn


def setup(**kw):
    """Global values (by name in G), a mixer model, a saved line table, and Python-side STATE for fakes."""
    model, lines, values, dvars = kw.pop("model", None), kw.pop("lines", None), kw.pop("globals", {}), kw.pop("dvars", {})

    def run(w):
        STATE.clear()
        STATE.update(kw)
        STATE["dvars"] = {DVARS[k]: v for k, v in dvars.items()}
        for name, va in DVARS.items():
            w.u32(G[name], va)
        if model:
            write_mixers(w, model)
        if lines is not None:
            write_lines(w, lines)
        for name, v in values.items():
            if isinstance(v, bytes):
                w.write(G[name], v)
            elif name in ("recording", "playing"):
                w.u8(G[name], v)
            else:
                w.u32(G[name], v)
    return run


def samples(present=None):
    present = present if present is not None else [rng.random() < 0.6 for _ in range(18)]
    return struct.pack("<18I", *(0x9000 + i if p else 0 for i, p in enumerate(present)))


BACKEND_FAKES = {
    "_Dvar_RegisterBool": lambda mu: DVARS[bytes(mu.mem_read(arg(mu, 0), 32)).split(b"\0")[0].decode()],
    "_Dvar_RegisterInt": lambda mu: DVARS[bytes(mu.mem_read(arg(mu, 0), 32)).split(b"\0")[0].decode()],
    "_Dvar_RegisterFloat": lambda mu: DVARS[bytes(mu.mem_read(arg(mu, 0), 32)).split(b"\0")[0].decode()],
    "?Dvar_GetInt@@YAHPBUdvar_t@@@Z": dvar_value("int"), "Dvar_GetBool": dvar_value("bool"),
    "Record_Init": lambda mu: STATE.get("record", 1), "Sound_Init": lambda mu: STATE.get("sound", 1),
    "Decode_Init": lambda mu: 1, "Encode_Init": lambda mu: 1, "Sound_NewSample": counter("sample", 0x9000),
    "Sound_SetVolume": lambda mu: None, "Sound_DestroySample": lambda mu: 1, "Sound_StopSample": lambda mu: 1,
    "Sound_SampleFrame": lambda mu: None, "Sound_Frame": lambda mu: None, "Record_Stop": lambda mu: 1,
    "Record_DestroySample": lambda mu: 1, "Record_Shutdown": lambda mu: None, "Encode_Shutdown": lambda mu: 1,
    "Decode_Shutdown": lambda mu: None, "Sound_Shutdown": lambda mu: None,
    "IN_IsTalkKeyHeld": lambda mu: STATE.get("talk", 0), "Sys_Milliseconds": lambda mu: STATE.get("now", 0),
    "Assert_MyHandler": lambda mu: 1,
}
BACKEND_SEEN = {
    "_Dvar_RegisterBool": "sbis", "_Dvar_RegisterInt": "siiiis", "_Dvar_RegisterFloat": "sfffis",
    "Record_Init": "bq", "Sound_Init": "q", "Decode_Init": "i", "Encode_Init": "i", "Sound_SetVolume": "qi",
    "Sound_DestroySample": "q", "Sound_StopSample": "q", "Sound_SampleFrame": "q", "Sound_Frame": "",
    "Record_Stop": "q", "Record_DestroySample": "q", "Record_Shutdown": "", "Encode_Shutdown": "",
    "Decode_Shutdown": "", "Sound_Shutdown": "", "Assert_MyHandler": "siiss",
}


def init_inputs():
    out = []
    for _ in range(40):
        model = mixers(rng.randrange(0, 11), rng.choice([0, 1, 2]))
        out.append({"args": [], "model": model, "setup": setup(model=model, record=rng.randrange(2), sound=rng.randrange(2),
                                               dvars={"winvoice_mic_reclevel": rng.choice([0, 0x8000, 0xFFFF]),
                                                      "winvoice_mic_mute": rng.randrange(2),
                                                      "winvoice_voice_level": rng.randrange(256)})})
    return out


def shutdown_inputs():
    out = []
    for _ in range(30):
        model = mixers()
        out.append({"args": [], "setup": setup(model=model, lines=lines_for(model), globals={
            "g_voice_initialized": rng.randrange(2), "g_voice_record_initialized": 1,
            "s_clientSamples": samples(), "recording": rng.randrange(2), "currentRecordingSample": 0x9999})})
    return out


def playback_inputs():
    out = []
    for _ in range(30):
        model = mixers()
        out.append({"args": [], "setup": setup(model=model, lines=lines_for(model), globals={
            "g_voice_initialized": rng.choice([0, 1, 1]), "s_clientSamples": samples(),
            "wavein_current_reclevel": rng.choice([0x8000, 0xFFFF])},
            dvars={"winvoice_mic_reclevel": rng.choice([0x8000, 0xFFFF, 0x1000])})})
    return out


def mixer_set_inputs():
    out = []
    for _ in range(40):
        model = mixers(rng.randrange(0, 9))
        lines = lines_for(model)
        if lines and rng.random() < 0.3:  # a saved line whose mixer has since gone or changed
            lines[0] = (1, rng.randrange(12), rng.randrange(0x10000, 0x20000), 0x1111, 1)
        out.append({"args": [rng.choice([0, 1, 0x7FFF, 0xFFFF])], "setup": setup(model=model, lines=lines)})
    return out


def mixer_get_inputs():
    out = []
    for _ in range(40):
        n, wavein, entries = model = mixers(rng.randrange(1, 6))
        mixer = rng.randrange(n + 1)
        line = entries[mixer]["line"] if mixer < n and rng.random() < 0.8 else 0x55555
        out.append({"args": [mixer, line], "setup": setup(model=model)})
    return out


def incoming_inputs():
    out = []
    for _ in range(40):
        talker = rng.choice([0, 3, 17, rng.randrange(18)])
        present = [rng.random() < 0.7 for _ in range(18)]
        size = rng.choice([0, 1, 20, 38, 100, 333])
        out.append({"args": [0, talker, rng.randbytes(size + 1), size],
                    "setup": setup(now=rng.randrange(1, 10**6), decoded=rng.choice([0, 160, 320, -1]),
                                   file=rng.choice([0, 0x5150]),
                                   dvars={"winvoice_save_voice": rng.randrange(2)},
                                   globals={"g_voice_initialized": rng.choice([0, 1, 1, 1]), "playing": rng.randrange(2),
                                            "s_clientSamples": samples(present),
                                            "g_decode_frame_size": rng.choice([20, 38, 50]),
                                            "s_clientTalkTime": struct.pack("<18I", *range(18))})})
    return out


def talking_inputs():
    out = []
    for client in [0, 5, 17, 18, -1, 100]:
        for now in [0, 299, 300, 301, 1000]:
            out.append({"args": [client], "setup": setup(now=now, globals={
                "s_clientTalkTime": struct.pack("<18I", *[0] * 9 + [700] * 9)})})
    return out


def xuid_inputs():
    out = []
    for _ in range(40):
        out.append({"args": [0, rng.choice([0, 1]), ("q", rng.getrandbits(64))],
                    "setup": setup(connected=rng.choice([0, 1, 1]), local=rng.randrange(2), talk=rng.randrange(2),
                                   client=rng.choice([0, 7, 17, 18, -1]), now=rng.choice([100, 900]),
                                   globals={"g_voice_initialized": rng.randrange(2),
                                            "g_voice_record_initialized": rng.randrange(2),
                                            "s_clientTalkTime": struct.pack("<18I", *[0] * 9 + [700] * 9)})})
    return out


def decode_sample(mu):
    """Decode_Sample(data, size, out, frameSize): writes a recognisable frame into out and returns STATE["decoded"]."""
    n = STATE["decoded"]
    if n > 0:
        data = struct.pack("<I", arg(mu, 0))  # the input position, as content
        mu.mem_write(arg(mu, 2), (data * (n // 4 + 1))[:n])
    return n


CASES = [
    {"name": "Voice_Init", "ret": "bool", "builds": SERVER, "inputs": init_inputs(), "imports": IMPORTS,
     "fakes": BACKEND_FAKES, "observe": BACKEND_SEEN},
    {"name": "Voice_Shutdown", "ret": "void", "builds": SERVER, "inputs": shutdown_inputs(), "imports": IMPORTS,
     "fakes": BACKEND_FAKES, "observe": BACKEND_SEEN},
    {"name": "Voice_StopClientSamples", "ret": "void", "builds": SERVER,
     "inputs": [{"args": [], "setup": setup(globals={"g_voice_initialized": i, "s_clientSamples": samples()})}
                for i in (0, 1, 1, 1)],
     "fakes": BACKEND_FAKES, "observe": BACKEND_SEEN},
    {"name": "Voice_GetLocalVoiceData", "ret": "int", "inputs": [[0, 0, 0], [1, 2, 3]]},
    {"name": "Voice_IsLocalClientTalking", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [0, 0], "setup": setup(talk=t, globals={"g_voice_initialized": a, "g_voice_record_initialized": b})}
                for a in (0, 1) for b in (0, 1) for t in (0, 1)],
     "fakes": BACKEND_FAKES, "observe": {"IN_IsTalkKeyHeld": ""}},
    {"name": "Voice_IsHeadsetPresent", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [0, 0], "setup": setup(globals={"g_voice_initialized": v})} for v in (0, 1, 2)]},
    {"name": "Voice_Playback", "ret": "void", "builds": SERVER, "inputs": playback_inputs(), "imports": IMPORTS,
     "fakes": BACKEND_FAKES, "observe": BACKEND_SEEN},
    {"name": "Voice_IncomingVoiceData_Platform", "ret": "int", "builds": SERVER, "inputs": incoming_inputs(),
     "fakes": {**BACKEND_FAKES, "Decode_Sample": decode_sample, "Sound_UpdateSample": lambda mu: 1,
               "fopen": lambda mu: STATE["file"], "fwrite": lambda mu: 1, "fclose": lambda mu: 0},
     "observe": {"Decode_Sample": "qixi", "Sound_UpdateSample": "qxi", "fopen": "ss", "fwrite": "xiiq",
                 "fclose": "q", "Assert_MyHandler": "siiss"}},
    {"name": "Voice_IsXuidTalking", "ret": "bool", "builds": SERVER, "inputs": xuid_inputs(),
     "fakes": {**BACKEND_FAKES, "Live_Base_IsConnected": lambda mu: STATE["connected"],
               "Live_XUIDIsLocalPlayer": lambda mu: STATE["local"],
               "Com_ControllerIndex_GetLocalClientNum": lambda mu: 0,
               "Session_FindRegisteredUser": lambda mu: STATE["client"]},
     "observe": {"Live_Base_IsConnected": "i", "Live_XUIDIsLocalPlayer": "ii", "Session_FindRegisteredUser": "qii",
                 "Com_ControllerIndex_GetLocalClientNum": "i", "IN_IsTalkKeyHeld": "", "Sys_Milliseconds": ""}},
    {"name": "Voice_IsClientTalking", "ret": "bool", "builds": SERVER, "inputs": talking_inputs(),
     "fakes": BACKEND_FAKES, "observe": {"Assert_MyHandler": "siiss", "Sys_Milliseconds": ""}},
    {"name": "Mixer_InitWaveInLineInfo", "ret": "void", "imports": IMPORTS,
     "builds": SERVER + ["t6mp_msstore"],
     "by_build": {"t6mp_msstore": {"inputs": [
         {"args": [], "setup": store_mixer_setup(mixers(rng.randrange(0, 9), rng.choice([0, 1, 3])), [(1, 9, 9, 9, 9)] * 3)}
         for _ in range(60)]}},
     "inputs": [{"args": [], "setup": setup(model=mixers(rng.randrange(0, 9), rng.choice([0, 1, 3])),
                                            lines=[(1, 9, 9, 9, 9)] * 3)} for _ in range(40)]},
    {"name": "Mixer_SetWaveInRecordLevels", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": mixer_set_inputs()},
    {"name": "Mixer_RestoreWaveInRecordLevels", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [], "setup": i["setup"]} for i in mixer_set_inputs()]},
    {"name": "Mixer_SetWaveInMuteStates", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": mixer_set_inputs()},
    {"name": "Mixer_RestoreWaveInMuteStates", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [], "setup": i["setup"]} for i in mixer_set_inputs()]},
    {"name": "Mixer_GetRecordLevel", "ret": "int", "builds": SERVER, "imports": IMPORTS, "inputs": mixer_get_inputs()},
    {"name": "Mixer_GetMuteState", "ret": "int", "builds": SERVER, "imports": IMPORTS, "inputs": mixer_get_inputs()},
]
