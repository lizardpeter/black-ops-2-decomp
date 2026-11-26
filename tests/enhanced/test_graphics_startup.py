"""Direct3D startup when the display adapter reports NVIDIA and NVAPI is unavailable."""
import struct

from unicorn import x86_const as x86

import test_r_init as ri
from test_mouse import read

BUILDS = ["t6mp_msstore"]
PATCH = "P0011"


def check_adapter_probe_falls_back_to_d3d11(ctx):
    calls = []
    device_pointer = 0x1710
    context_pointer = 0x1720
    level = 0xB000
    args = [0x1990, 0, 0, 0, 0, 0, 7, bytes(8), bytes(4), bytes(8), bytes(4)]

    def create(_, mu):
        calls.append((ri.stack_arg(mu, 0), ri.stack_arg(mu, 1) & 0xFFFFFFFF, ri.stack_arg(mu, 6) & 0xFFFFFFFF))
        mu.mem_write(ri.stack_arg(mu, 7), struct.pack("<Q", device_pointer))
        mu.mem_write(ri.stack_arg(mu, 8), struct.pack("<I", level))
        mu.mem_write(ri.stack_arg(mu, 9), struct.pack("<Q", context_pointer))
        mu.reg_write(x86.UC_X86_REG_RAX, 0)

    ctx.imports({"d3d11.dll!D3D11CreateDevice": (create, 10)})
    hooks = ctx.hooks({"NvAPI_AcquireInterface": lambda _: -2})
    original = ctx.m.call(ctx.original("NvAPI_D3D11_CreateDevice"), args, "int", fakes=hooks)
    assert isinstance(original[0], int) and original[0] & 0xFFFFFFFF == 0xFFFFFFFE, original[0]
    assert not calls, calls
    got = ctx.m.call(ctx.enhanced("NvAPI_D3D11_CreateDevice"), args, "int", fakes=hooks)
    assert got[0] == 0, got[0]
    assert calls == [(0x1990, 0, 7)], calls
    assert got[1] == [struct.pack("<Q", device_pointer), struct.pack("<I", level),
                     struct.pack("<Q", context_pointer), struct.pack("<i", 3)], got[1]


def check_swap_chain_falls_back_when_nvapi_is_unavailable(ctx):
    calls = []

    def nv_create(_):
        calls.append("nvapi")
        return -3

    def create(m, mu):
        calls.append("d3d11")
        ri.D3D11CreateDeviceAndSwapChain(m, mu)

    ri.D3D[("IUnknown", 12 * 8)] = "i"
    ctx.imports(dict(ri.DEVICE_IMPORTS, **{"d3d11.dll!D3D11CreateDeviceAndSwapChain": (create, 12)}))
    fakes = ctx.hooks(dict(ri.DEVICE_FAKES, NvAPI_D3D11_CreateDeviceAndSwapChain=nv_create))
    args = [0x5150, 0, bytes(0x48), ri.window_parms(0x5150, False)]
    params = dict(ri.device_inputs(1)[0], vendor=0x10DE, factory_hr=0, enum_hr=0, parent_hr=0,
                  create_ok=True, swapchain=True, nv_init=-2, nv_was=0)
    got = ctx.m.call(ctx.enhanced("R_CreateDeviceInternal"), args, "int", ri.device_setup(params), fakes)
    assert got[0] == 0, got[0]
    assert calls == ["nvapi", "d3d11"]
    assert read(got[2], ri.DX + 0x3C7B8, 8) == struct.pack("<ii", 1, 0)
    calls.clear()
    got = ctx.m.call(ctx.enhanced("R_CreateDeviceInternal"), args, "int",
                     ri.device_setup(dict(params, create_ok=False)), fakes)
    assert isinstance(got[0], int) and got[0] & 0xFFFFFFFF == 0x887A0004
    assert calls == ["nvapi", "d3d11"]


CHECKS = [check_adapter_probe_falls_back_to_d3d11, check_swap_chain_falls_back_when_nvapi_is_unavailable]
