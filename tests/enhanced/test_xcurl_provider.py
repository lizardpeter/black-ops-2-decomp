"""P0016: reject XCurl initialization with the native failure result."""

import struct

from unicorn import x86_const as x86

BUILDS = ["t6mp_msstore", "t6zm_msstore"]
PATCH = "P0016"
EXE_BASE = 0x140000000


def check_xcurl_provider_redirect(ctx):
    machine = ctx.m
    original = ctx.original("XCurlCreateProvider")
    replacement = ctx.enhanced("XCurlCreateProvider")
    table = ctx.recon.exports["t6_redirects"]
    count = struct.unpack("<Q", machine.mu.mem_read(table, 8))[0]
    redirects = dict(
        struct.unpack("<QQ", machine.mu.mem_read(table + 8 + 32 * index, 16))
        for index in range(count)
    )
    assert redirects[original - EXE_BASE] == replacement

    def fail_curl_init(_, mu):
        assert mu.reg_read(x86.UC_X86_REG_ECX) == 3
        mu.reg_write(x86.UC_X86_REG_EAX, 2)  # CURLE_FAILED_INIT

    ctx.imports({"xcurl.dll!curl_global_init": (fail_curl_init, 1)})
    result_buffer = bytes([0xA5]) * 0x40
    native_return, native_buffers, _, _ = machine.call(original, [result_buffer], "uint64")
    assert [name.lower() for name in machine.imports_called] == ["xcurl.dll!curl_global_init"]
    assert struct.unpack_from("<I", native_buffers[0])[0] == 0x80004005

    local_return, local_buffers, _, _ = machine.call(replacement, [result_buffer], "uint64")
    assert (local_return, local_buffers) == (native_return, native_buffers)
    assert not machine.imports_called


CHECKS = [check_xcurl_provider_redirect]
