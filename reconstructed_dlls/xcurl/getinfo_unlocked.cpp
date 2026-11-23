// Reconstructed *inner* info selector of the Store XCurl request engine.
// Original XCurl.dll SHA256:
// 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551
//
// Directly transcribed from FUN_18000AD58, called by curl_easy_getinfo.
// This isolates only its option dispatch and handle field reads. The original
// EnterCriticalSection/LeaveCriticalSection and ETW profiling are NOT removed
// from the intended final implementation; they remain unresolved dependencies.
// No libcurl source or runtime implementation is used.
//
// Return code 0 = success, 0x30 = original unknown/unimplemented info option.
// The caller must pass a valid handle of at least 0x22C bytes, an output buffer
// large enough for the option's type, and an aligned pointer to that buffer.
#include <cstdint>
#include <cstring>

namespace t6_xcurl::internal {
template <typename T>
static T field(const std::uint8_t* easy, unsigned offset) {
    T value{};
    std::memcpy(&value, easy + offset, sizeof(value));
    return value;
}
template <typename T>
static void store(void* p, T value) {
    std::memcpy(p, &value, sizeof(value));
}
} // namespace t6_xcurl::internal

extern "C" std::uint32_t xcurl_easy_getinfo_unlocked(
        const void* handle, int option, void** output_pointer) {
    using namespace t6_xcurl::internal;
    auto* h = static_cast<const std::uint8_t*>(handle);
    void* out = *output_pointer;
    switch (static_cast<std::uint32_t>(option)) {
        // CURlINFO_STRING-like pointer results (64-bit on this Store build)
        case 0x100001: store(out, field<std::uint64_t>(h, 0x208)); return 0;
        case 0x100012: store(out, field<std::uint64_t>(h, 0x218)); return 0;
        case 0x100015: store(out, field<std::uint64_t>(h, 0x148)); return 0;
        case 0x100031: store(out, field<std::uint64_t>(h, 0x210)); return 0;

        // 32-bit info results
        case 0x200002: store(out, field<std::uint32_t>(h, 0x1E0)); return 0;
        case 0x20000B: store(out, field<std::uint32_t>(h, 0x1F0)); return 0;
        case 0x20000D: store(out, field<std::uint32_t>(h, 0x224)); return 0;
        case 0x200019: store(out, field<std::uint32_t>(h, 0x228)); return 0;
        case 0x20002E: store(out, field<std::uint32_t>(h, 0x1DC)); return 0;
        case 0x200030: store(out, field<std::uint32_t>(h, 0x1D8)); return 0;

        // Original uses cvtsi2sd on signed 64-bit integers.
        case 0x300008:
        case 0x30000F:
            store(out, static_cast<double>(field<std::int64_t>(h, 0x1F8)));
            return 0;
        case 0x300010:
            store(out, static_cast<double>(field<std::int64_t>(h, 0x200)));
            return 0;

        // Original writes 64-bit integer values without conversion.
        case 0x600008:
        case 0x60000F:
            store(out, field<std::uint64_t>(h, 0x1F8));
            return 0;
        case 0x600010:
            store(out, field<std::uint64_t>(h, 0x200)); return 0;
        case 0x600039:
            store(out, static_cast<std::int64_t>(field<std::int32_t>(h, 0x1D8)));
            return 0;
        default:
            return 0x30;
    }
}
