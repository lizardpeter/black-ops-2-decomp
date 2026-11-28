"""P0010: exercise the shipped game entry points through the DLL redirect table."""

import struct

from unicorn import x86_const as x86

from test_mouse import read

BUILDS = ["t6mp_msstore"]
PATCH = "P0010"
EXE = 0x140000000


def check_vendor_function_redirects(ctx):
    table = ctx.recon.exports["t6_redirects"]
    mu = ctx.m.mu
    count = struct.unpack("<Q", bytes(mu.mem_read(table, 8)))[0]
    redirects = dict(
        struct.unpack("<QQ", bytes(mu.mem_read(table + 8 + 32 * index, 16)))
        for index in range(count)
    )
    ctx.imports({
        "kernel32.dll!GetModuleHandleExA": (model_enhanced_module, 0),
        "kernel32.dll!GetCommandLineW": (lambda _, mu: mu.reg_write(x86.UC_X86_REG_RAX, 0), 0),
        "shell32.dll!CommandLineToArgvW": (lambda _, mu: mu.reg_write(x86.UC_X86_REG_RAX, 0), 2),
    })

    def addr(family):
        return bytes(128) + struct.pack("<HH", family, 0)

    error = ctx.original("Com_Error")
    complete = ctx.original("BdCompleteWinHttpRequest")
    curl_result = ctx.original("XCurlResult")
    destroy = ctx.original("HttpDestroyEasyRequest")
    free = ctx.original("HttpFree")
    for function, args, expected, expected_imports in (
        ("BdGetHostByName", [b"service.invalid\0", bytes(132), 1], 0, []),
        ("BdSendTo", [9, addr(2), 1234, b"test", 4], -5, []),
        ("BdSendTo", [9, addr(23), 1234, b"test", 4], -5, []),
        ("BdSendTo", [-1, addr(2), 1234, b"test", 4], -10, []),
        ("GdkLoadRuntime", [], 0, ["kernel32.dll!GetModuleHandleExA", "kernel32.dll!GetCommandLineW",
                                   "shell32.dll!CommandLineToArgvW"]),
        ("Sys_ReportUnhandledException", [0], 1, []),
        ("Sys_ProbeTimeServer", [b"127.0.0.1\0"], 0, []),
        ("BdConnectStream", [9, bytes(4), 443], -5, []),
        ("WinInetOpenConnection", [b"https://example.invalid/\0"], 0, []),
        ("BdBeginWinHttpRequest", [0, 0x5566], None, []),
        ("XCurlAddRequest", [0, struct.pack("<B7xQ", 1, 0x7788)], -2147467259, []),
        ("WinHttpInitializeRequest", [0], -2147012867, []),
    ):
        va = ctx.original(function)
        replacement = ctx.enhanced(function)
        assert redirects[va - EXE] == replacement
        original = bytes(mu.mem_read(va, 14))
        try:
            mu.mem_write(va, b"\xff\x25" + bytes(4) + struct.pack("<Q", replacement))
            mu.ctl_remove_cache(va, va + 14)
            fallbacks = {error: lambda mu: 0, complete: lambda mu: 0,
                         curl_result: lambda mu: 0x80004005, destroy: lambda mu: 0, free: lambda mu: 0}
            observe = {error: "is", complete: "qi", curl_result: "i", destroy: "q", free: "q"}
            result = ctx.m.call(va, args, "void" if expected is None else "int", fakes=fallbacks, observe=observe)
            assert result[0] == expected, (function, result[0])
            assert [name.lower() for name in ctx.m.imports_called] == [name.lower() for name in expected_imports]
            if function == "Sys_ReportUnhandledException":
                assert result[3] == [(error, 0, b"Unhandled exception caught\n")]
            if function == "BdBeginWinHttpRequest":
                assert result[3] == [(complete, 0x5566, 0x80072EFD)]
            if function == "XCurlAddRequest":
                assert result[3] == [(curl_result, 4), (destroy, 0x7788), (free, 0x7788)]
        finally:
            mu.mem_write(va, original)
            mu.ctl_remove_cache(va, va + 14)


def model_enhanced_module(machine, mu):
    assert mu.reg_read(x86.UC_X86_REG_ECX) == 4
    module = machine.images[1].base
    machine.program_write(mu.reg_read(x86.UC_X86_REG_R8), struct.pack("<Q", module))
    mu.reg_write(x86.UC_X86_REG_RAX, 1)


def check_gdk_initializes_local_save(ctx):
    save_path, save_ready = 0x143ED0420, 0x143E33FF1
    # the save folder is in the game's folder: <folder of the exe>\save
    exe = r"C:\Games\Black Ops II\t6mp.exe"
    directory = r"C:\Games\Black Ops II\save"
    filesystem = []

    def module_name(_, mu):
        assert mu.reg_read(x86.UC_X86_REG_RCX) == 0  # the exe's own path
        destination = mu.reg_read(x86.UC_X86_REG_RDX)
        mu.mem_write(destination, (exe + "\0").encode("utf-16le"))
        mu.reg_write(x86.UC_X86_REG_RAX, len(exe))

    def create(_, mu):
        pointer = mu.reg_read(x86.UC_X86_REG_RCX)
        filesystem.append(bytes(mu.mem_read(pointer, len(directory) * 2)).decode("utf-16le"))
        mu.reg_write(x86.UC_X86_REG_RAX, 1)

    def encode(machine, mu):
        pointer = mu.reg_read(x86.UC_X86_REG_RSP)
        buffer = struct.unpack("<Q", bytes(mu.mem_read(pointer + 0x28, 8)))[0]
        path = (directory + "\0").encode("utf-8")
        machine.program_write(buffer, path)
        mu.reg_write(x86.UC_X86_REG_RAX, len(path))

    ctx.imports({
        "kernel32.dll!GetModuleFileNameW": (module_name, 0),
        "kernel32.dll!CreateDirectoryW": (create, 0),
        "kernel32.dll!WideCharToMultiByte": (encode, 0),
    })
    init = ctx.enhanced("InitializeApiImplEx")
    status, _, writes, _ = ctx.m.call(init, [0, 0, 0])
    assert status == 0, status
    assert filesystem == [directory]
    assert read(writes, save_ready, 1) == b"\x01"
    assert read(writes, save_path, len(directory) + 1) == (directory + "\0").encode("utf-8")

    ctx.imports({"kernel32.dll!GetModuleFileNameW":
                 (lambda _, mu: mu.reg_write(x86.UC_X86_REG_RAX, 0), 0)})
    status, _, writes, _ = ctx.m.call(init, [0, 0, 0], "uint")
    assert status == 0x80004005 and save_ready not in writes


CHECKS = [check_vendor_function_redirects, check_gdk_initializes_local_save]
