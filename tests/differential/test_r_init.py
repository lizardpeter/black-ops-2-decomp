"""gfx_d3d/r_init.cpp (Store only: the server's renderer has no device code)."""
import struct

from harness import OBJECTS

STORE = ["t6mp_msstore"]
R_AA_SAMPLES = 0x1427E7158  # const dvar_t *
DVAR = OBJECTS + 0x1000     # a dvar_t (x64: type +0x18, current +0x20)
TEXT = OBJECTS + 0x1100
DX_MULTISAMPLE = 0x14282E538


def aa(dvar_type, value):
    """r_aaSamples as a dvar of the given type (1 bool, 2 float, 6 int, 7 enum, 8 string), or none."""
    def run(w):
        if dvar_type is None:
            w.u64(R_AA_SAMPLES, 0)
            return
        w.u64(R_AA_SAMPLES, DVAR)
        w.write(DVAR + 0x18, struct.pack("<i", dvar_type))
        if dvar_type == 1:
            w.write(DVAR + 0x20, bytes([value]) + bytes(7))
        elif dvar_type == 2:
            w.write(DVAR + 0x20, struct.pack("<f", value) + bytes(4))
        elif dvar_type in (6, 7):
            w.write(DVAR + 0x20, struct.pack("<iI", value, 0xDEAD))
        else:
            w.write(TEXT, value.encode() + b"\0")
            w.u64(DVAR + 0x20, TEXT)
        w.write(DX_MULTISAMPLE, struct.pack("<II", 0x77, 0x88))
    return run


def window(hwnd, width, height):
    return struct.pack("<QiB3xiiiiii", hwnd, 60, 1, 5, 6, 1280, 720, width, height) + bytes(4)


AA = [(None, 0), (6, 0), (6, 2), (6, 4), (6, 8), (6, 16), (6, 17), (6, -1), (7, 16), (1, 1), (2, 16.9), (2, 4.0),
      (8, "16"), (8, " 8x"), (8, "")]

CASES = [
    {"name": "sub_14071E360", "ret": "void", "builds": STORE,
     "inputs": [{"args": [bytes([0xCC]) * 0x48, window(hwnd, wid, hei)], "setup": aa(t, v)}
                for t, v in AA for hwnd, wid, hei in ((0x50A0B0C0D, 2560, 1440), (0, 0, 0), (1, -1, 7))]},
]


# ---- R_CreateDeviceInternal (14071FEA0) ---------------------------------------------------------------------------
# Every COM call goes through com.py's model; NVAPI, TXAA and the dvar functions are faked, the Win32 and D3D11/DXGI
# imports modelled. Everything they create or write lands in journaled, compared writes.
import random  # noqa: E402

from unicorn import x86_const as x86  # noqa: E402

from com import ComModel  # noqa: E402

DX = 0x1427F1D80
F = {"device": 8, "context": 0x10, "constBuffer": 0x18, "vendorId": 0x38, "nativeValid": 0x3C, "nativeW": 0x40,
     "fsW": 0x48, "nvInitialized": 0x50, "stereoActivated": 0x51, "stereoHandle": 0x58, "adapterIndex": 0x480,
     "needsShaderWarming": 0x488, "swapChain": 0x3C7F8}
R_SHADER_WARMING, R_AA_SAMPLES_MAX, R_TXAA_SUPPORTED = 0x1427E7220, 0x1427E7150, 0x1427E6EF8
WARM_DVAR, TXAA_DVAR, DVAR_TEXT = OBJECTS + 0x2000, OBJECTS + 0x2100, OBJECTS + 0x2200
LOG = OBJECTS + 0x3000
DEV = {}

D3D = {("IDXGIFactory", 12 * 8): "ip", ("IDXGIFactory", 8 * 8): "qi", ("IDXGIFactory", 2 * 8): "",
       ("IDXGIAdapter", 6 * 8): "pp", ("IDXGIAdapter", 2 * 8): "", ("IUnknown", 6 * 8): "pp", ("IUnknown", 2 * 8): "",
       ("ID3D11Device", 0): "pp", ("ID3D11Device", 2 * 8): "", ("ID3D11Device", 30 * 8): "iip",
       ("ID3D11Device", 3 * 8): "ppp", ("ID3D11DeviceContext", 2 * 8): "", ("ID3D11DeviceContext", 7 * 8): "iip",
       ("ID3D11DeviceContext", 16 * 8): "iip", ("IDXGISwapChain", 15 * 8): "p", ("IDXGISwapChain", 11 * 8): "pp",
       ("IDXGIOutput", 7 * 8): "p", ("IDXGIOutput", 2 * 8): ""}
DUMPS = {("IDXGIAdapter", 6 * 8): {0: 16}, ("IUnknown", 6 * 8): {0: 16}, ("ID3D11Device", 0): {0: 16},
         ("ID3D11Device", 3 * 8): {0: 24}, ("ID3D11DeviceContext", 7 * 8): {2: 32},
         ("ID3D11DeviceContext", 16 * 8): {2: 32}}


def m_():
    return DEV["m"]


def stack_arg(mu, i):
    """Argument i (from 0) at a function's entry, x64."""
    if i < 4:
        return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[i])
    return struct.unpack("<Q", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RSP) + 8 + 8 * i, 8)))[0]


def out(va, value, fmt="<Q"):
    if va:
        m_().program_write(va, struct.pack(fmt, value))


def log(*values):
    n = struct.unpack("<I", bytes(m_().mu.mem_read(LOG, 4)))[0]
    m_().program_write(LOG + 8 + 16 * n, struct.pack("<QQ", *(list(values) + [0])[:2]))
    m_().program_write(LOG, struct.pack("<I", n + 1))


def create_device(mu, first):
    """D3D11CreateDeviceAndSwapChain's and NVAPI's outputs: swap chain (arg 8), device (9), feature level (10),
    context (11)."""
    p = DEV["p"]
    model = DEV["model"]
    if p["create_ok"]:
        out(stack_arg(mu, 8), model.new("IDXGISwapChain", mu) if p["swapchain"] else 0)
        out(stack_arg(mu, 9), model.new("ID3D11Device", mu))
        out(stack_arg(mu, 10), 0xB000, "<I")
        out(stack_arg(mu, 11), model.new("ID3D11DeviceContext", mu))


def D3D11CreateDeviceAndSwapChain(m, mu):
    create_device(mu, True)
    log(0xD3D, stack_arg(mu, 6))
    mu.reg_write(x86.UC_X86_REG_RAX, 0 if DEV["p"]["create_ok"] else 0x887A0004)


def CreateDXGIFactory1(m, mu):
    p = DEV["p"]
    if p["factory_hr"] >= 0:
        out(stack_arg(mu, 1), DEV["model"].new("IDXGIFactory", mu))
    mu.reg_write(x86.UC_X86_REG_RAX, p["factory_hr"] & 0xFFFFFFFF)


def GetSystemMetrics(m, mu):
    log(0x5E, stack_arg(mu, 0) & 0xFFFFFFFF)
    mu.reg_write(x86.UC_X86_REG_RAX, DEV["p"]["metrics"][stack_arg(mu, 0) & 1] & 0xFFFFFFFF)


def GetMonitorInfoA(m, mu):
    p = DEV["p"]
    log(0x30, stack_arg(mu, 0))
    info = stack_arg(mu, 1)
    log(0x31, struct.unpack("<I", bytes(mu.mem_read(info, 4)))[0])
    if p["monitor_ok"]:
        mu.mem_write(info + 4, struct.pack("<4i", *p["rect"]))
    mu.reg_write(x86.UC_X86_REG_RAX, 1 if p["monitor_ok"] else 0)


def ShowWindow(m, mu):
    log(0x5A, stack_arg(mu, 0))
    log(0x5B, stack_arg(mu, 1) & 0xFFFFFFFF)
    mu.reg_write(x86.UC_X86_REG_RAX, 1)


DEVICE_IMPORTS = {"d3d11.dll!D3D11CreateDeviceAndSwapChain": (D3D11CreateDeviceAndSwapChain, 12),
                  "dxgi.dll!CreateDXGIFactory1": (CreateDXGIFactory1, 2),
                  "user32.dll!GetSystemMetrics": (GetSystemMetrics, 1),
                  "user32.dll!GetMonitorInfoA": (GetMonitorInfoA, 2), "user32.dll!ShowWindow": (ShowWindow, 2)}


def nv_create(mu):
    create_device(mu, True)
    out(stack_arg(mu, 12), DEV["p"]["nv_level"], "<i")
    return DEV["p"]["nv_status"]


def stereo_handle(mu):
    if DEV["p"]["stereo_status"] == 0:
        out(stack_arg(mu, 1), 0x57E0)
    return DEV["p"]["stereo_status"]


def stereo_active(mu):
    p = DEV["p"]
    if p["stereo_writes"]:
        mu.mem_write(stack_arg(mu, 1), bytes([p["stereo_on"]]))
    return 0 if p["stereo_writes"] else -1


def set_variant(mu):
    dvar, value, source = stack_arg(mu, 0), stack_arg(mu, 1), stack_arg(mu, 2)
    type_ = struct.unpack("<i", bytes(mu.mem_read(dvar + 0x18, 4)))[0]
    raw = struct.unpack("<Q", bytes(mu.mem_read(value, 8)))[0]
    m_().observed.append(("Dvar_SetVariant", dvar, raw & 0xFF if type_ == 1 else m_().read_cstr(raw), source & 0xFF))


DEVICE_FAKES = {
    "_Dvar_RegisterBool": lambda mu: DEV["p"]["warm_dvar"],
    "NvAPI_Initialize": lambda mu: DEV["p"]["nv_init"],
    "NvAPI_D3D11_CreateDeviceAndSwapChain": nv_create,
    "NvAPI_Stereo_CreateHandleFromIUnknown": stereo_handle,
    "NvAPI_Stereo_IsActivated": stereo_active,
    "TxaaOpenDX": lambda mu: DEV["p"]["txaa"],
    "Dvar_SetIntFromSource": lambda mu: None,
    "Dvar_SetVariant": set_variant,
    "R_SetFullscreenOutput": lambda mu: None,
}
DEVICE_SEEN = {"_Dvar_RegisterBool": "sbis", "NvAPI_Initialize": "", "NvAPI_D3D11_CreateDeviceAndSwapChain": "qiqiqiqq",
               "NvAPI_Stereo_CreateHandleFromIUnknown": "qq", "NvAPI_Stereo_IsActivated": "q", "TxaaOpenDX": "qqq",
               "Dvar_SetIntFromSource": "qii", "R_SetFullscreenOutput": ""}


def device_setup(p):
    def run(w):
        DEV.clear()
        DEV.update(m=w.m, p=p)
        model = ComModel(D3D, dumps=DUMPS)
        model.install(w.m)
        DEV["model"] = model

        def out_obj(kind):
            return lambda call: (call.out_object(len(call.args) - 1, kind), 0)[1]

        model.results[("IDXGIFactory", 12 * 8)] = lambda call: (
            call.out_object(1, "IDXGIAdapter") if p["enum_hr"] >= 0 else None, p["enum_hr"])[1]
        model.results[("ID3D11Device", 0)] = out_obj("IUnknown")
        model.results[("IUnknown", 6 * 8)] = out_obj("IDXGIAdapter")
        model.results[("IDXGIAdapter", 6 * 8)] = lambda call: (call.out_object(1, "IDXGIFactory"), p["parent_hr"])[1]
        model.results[("ID3D11Device", 30 * 8)] = lambda call: (call.write(call.arg(2), struct.pack("<I", p["levels"])), 0)[1]
        model.results[("ID3D11Device", 3 * 8)] = out_obj("ID3D11Resource")
        model.results[("IDXGISwapChain", 15 * 8)] = out_obj("IDXGIOutput")
        model.results[("IDXGIOutput", 7 * 8)] = lambda call: (
            call.mu.mem_write(call.arg(0), bytes(0x40) + struct.pack("<4iIIQ", 0, 0, 1, 1, 1, 0, 0x30A0)), 0)[1]
        model.results[("IDXGISwapChain", 11 * 8)] = lambda call: (
            call.mu.mem_write(call.arg(0), struct.pack("<i", p["fs_state"])), 0)[1]
        w.write(DX + F["device"], struct.pack("<QQ", model.new("ID3D11Device"), model.new("ID3D11DeviceContext")))
        w.write(DX + F["constBuffer"], bytes([0xEE]) * 32)
        w.write(DX + F["vendorId"], struct.pack("<I", p["vendor"]))
        w.write(DX + F["nativeValid"], struct.pack("<B3x6i", 0x7, 11, 12, 13, 14, 0, 0)[:0x14])
        w.write(DX + F["nvInitialized"], bytes([p["nv_was"], 0x5]))
        w.write(DX + F["stereoHandle"], struct.pack("<Q", 0x1111))
        w.write(DX + F["adapterIndex"], struct.pack("<i", p["adapter"]))
        w.write(DX + F["needsShaderWarming"], bytes([p["warming_was"]]))
        w.write(DX + F["swapChain"], struct.pack("<Q", 0x2222))
        w.write(LOG, struct.pack("<I", 0))
        # r_shaderWarming as _Dvar_RegisterBool returns it: a bool, or a string the UI set
        if p["warm_dvar"]:
            w.write(WARM_DVAR + 0x18, struct.pack("<i", p["warm_type"]))
            if p["warm_type"] == 1:
                w.write(WARM_DVAR + 0x20, bytes([p["warm_value"]]) + bytes(7))
            else:
                w.write(DVAR_TEXT, p["warm_text"].encode() + b"\0")
                w.u64(WARM_DVAR + 0x20, DVAR_TEXT)
        w.u64(R_AA_SAMPLES_MAX, 0x6000)
        if p["txaa_dvar"]:
            w.u64(R_TXAA_SUPPORTED, TXAA_DVAR)
            w.u64(TXAA_DVAR, TEXT if p["txaa_named"] else 0)
            w.write(TEXT, b"r_txaaSupported\0")
            w.write(TXAA_DVAR + 0x18, struct.pack("<i", p["txaa_type"]))
        else:
            w.u64(R_TXAA_SUPPORTED, 0)
    return run


def device_inputs(n=400, seed=0x1FEA0):
    r = random.Random(seed)
    out_ = []
    for _ in range(n):
        p = {"vendor": r.choice([0x10DE, 0x10DE, 0x1002, 0x8086]), "warming_was": r.randrange(2),
             "warm_dvar": r.choice([WARM_DVAR, WARM_DVAR, 0]), "warm_type": r.choice([1, 1, 8]),
             "warm_value": r.randrange(2), "warm_text": r.choice(["0", "1", "7", "x"]),
             "metrics": r.choice([(1920, 1080), (0, 0), (2560, -1), (3840, 2160)]),
             "factory_hr": r.choice([0, 0, 0, 0, -0x7FFFBFFB]), "enum_hr": r.choice([0, 0, 0, 0, -0x7785FFFE]),
             "adapter": r.choice([0, 1]), "nv_was": r.randrange(2), "nv_init": r.choice([0, 0, -1]),
             "nv_status": r.choice([0, 0, 0, -1]), "nv_level": r.choice([1, 2, 0, -1]), "create_ok": r.random() < 0.9,
             "swapchain": r.random() < 0.9, "stereo_status": r.choice([0, -1]), "stereo_writes": r.randrange(2),
             "stereo_on": r.choice([0, 1, 2]), "levels": r.choice([0, 1, 16, 17, 32]), "txaa": r.choice([0, 0, 1]),
             "txaa_dvar": r.random() < 0.85, "txaa_named": r.random() < 0.85, "txaa_type": r.choice([1, 8]),
             "parent_hr": r.choice([0, 0, -5]), "monitor_ok": r.randrange(2),
             "rect": r.choice([(0, 0, 1920, 1080), (-2560, 100, 0, 1540), (5, 5, 5, 5)]),
             "fs_state": r.randrange(2)}
        if not p["swapchain"]:
            p["fs_state"] = 0  # GetFullscreenState on a null swap chain faults in both: kept out
        out_.append(p)
    return out_


def window_parms(hwnd, fullscreen):
    return struct.pack("<QiB3xiiiiii", hwnd, 60, fullscreen, 0, 0, 1280, 720, 1920, 1080) + bytes(4)


CASES += [
    {"name": "R_CreateDeviceInternal", "ret": "int", "builds": STORE, "imports": DEVICE_IMPORTS,
     "fakes": DEVICE_FAKES, "observe": DEVICE_SEEN,
     "inputs": [{"args": [0x5150, r.randrange(1 << 31) if (r := random.Random(i)) else 0, bytes([0xCC]) * 0x48,
                          window_parms(0x5150 + (i & 1), i % 3 == 0)], "setup": device_setup(p)}
                for i, p in enumerate(device_inputs())]},
]
