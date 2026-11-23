// Original Bink x64 leaf-function transcriptions (no independent decoder yet).
// Original binkw64.dll SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
// Only exports with directly inspected bytes; not a replacement DLL.
#include <cstdint>
#include <cstddef>

namespace t6_bink {
// Original Bink globals live at the RVAs listed below. Their original startup
// values and other writers still need to be traced. These source equivalents
// are not relocations of the original data region.
char g_error[256] = {};                       // RVA 0x31860
std::uint32_t g_frame_rate_numerator = 0;   // RVA 0x570E4
std::uint32_t g_frame_rate_denominator = 0; // RVA 0x311C0
std::uint64_t g_file_offset = 0;            // RVA 0x57128
std::uint32_t g_simulate = 0;               // RVA 0x570DC

static std::uint32_t read_u32(const void* handle, std::size_t offset) {
    return *reinterpret_cast<const std::uint32_t*>(
        static_cast<const std::uint8_t*>(handle) + offset);
}
static void write_u32(void* handle, std::size_t offset, std::uint32_t value) {
    *reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(handle) + offset) = value;
}
} // namespace t6_bink

// RVA 0x14C00: copy up to 254 non-NUL characters, NUL terminate; input must
// be non-null (the original dereferences RCX unconditionally).
extern "C" void BinkSetError(const char* message) {
    std::size_t i = 0;
    for (; i < 254 && message[i] != '\0'; ++i) {
        t6_bink::g_error[i] = message[i];
    }
    t6_bink::g_error[i] = '\0';
}

// RVA 0x14C40: returns the shared error storage at RVA 0x31860.
extern "C" char* BinkGetError() {
    return t6_bink::g_error;
}

// RVA 0x16820: stores RCX/EDX as two uint32 globals.
extern "C" void BinkSetFrameRate(std::uint32_t numerator, std::uint32_t denominator) {
    t6_bink::g_frame_rate_numerator = numerator;
    t6_bink::g_frame_rate_denominator = denominator;
}

// RVA 0x16830: writes entire 64-bit RCX.
extern "C" void BinkSetFileOffset(std::uint64_t offset) {
    t6_bink::g_file_offset = offset;
}

// RVA 0x16860: stores uint32 ECX.
extern "C" void BinkSetSimulate(std::uint32_t simulate) {
    t6_bink::g_simulate = simulate;
}

// RVA 0x1DA40: toggles bit 7 of handle->flags (offset +0x20).
extern "C" void BinkSetWillLoop(void* handle, std::uint32_t enabled) {
    if (!handle) return;
    std::uint32_t flags = t6_bink::read_u32(handle, 0x20);
    flags = enabled ? flags | 0x80U : flags & ~0x80U;
    t6_bink::write_u32(handle, 0x20, flags);
}

// RVA 0x1D820: stores EDX at handle+0x134 if non-null; returns EDX even if null.
extern "C" std::uint32_t BinkSetVideoOnOff(void* handle, std::uint32_t state) {
    if (handle) t6_bink::write_u32(handle, 0x134, state);
    return state;
}

// RVA 0x1D320: dereference a uint32 table at handle+0x1D0; no index bounds check.
extern "C" std::uint32_t BinkGetTrackID(const void* handle, std::uint32_t index) {
    if (!handle) return 0;
    const auto* entries = *reinterpret_cast<const std::uint32_t* const*>(
        static_cast<const std::uint8_t*>(handle) + 0x1D0);
    return entries[index];
}

// RVA 0x1D2E0: dereference a uint32 table at handle+0x1C8; no index bounds check.
extern "C" std::uint32_t BinkGetTrackType(const void* handle, std::uint32_t index) {
    if (!handle) return 0;
    const auto* entries = *reinterpret_cast<const std::uint32_t* const*>(
        static_cast<const std::uint8_t*>(handle) + 0x1C8);
    return entries[index];
}
