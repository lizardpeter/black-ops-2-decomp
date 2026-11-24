// Machine-code reconstruction of the two original Bink entry dispatchers.
// Original binkw64.dll SHA-256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// BinkOpen at RVA 0x19B80 (pack one-shot globals and call original 0x17970).
// BinkDoFrame at RVA 0x1B0C0 (tail jump to original BinkDoFramePlane at 0x1A930).
// NOT a decoder. The actual internal targets remain unresolved and are only
// declared, never faked or substituted in the production source.
#include <cstddef>
#include <cstdint>
#include "bink_open_options.h"

namespace t6_bink {
extern std::uint32_t g_frame_rate_numerator;       // 0x570E4
extern std::uint32_t g_frame_rate_denominator;     // 0x311C0
extern std::uint32_t g_io_size;                    // 0x570E0
extern std::uint32_t g_simulate;                   // 0x570DC
extern std::uint32_t g_sound_track_count;          // 0x570B0
extern std::uint32_t g_sound_tracks[32];           // 0x57250
extern std::uint64_t g_file_offset;               // 0x57128
std::uint64_t g_secondary_file_offset = 0;        // 0x57120

// Exact field positions of the 48-byte stack record observed in the original
// dispatcher. The gap at +0x1C was uninitialized in the original assembly;
// not assumed meaningful until the full initializer at RVA 0x17970 is recovered.
namespace internal {
// Must be recovered from original DLL: FUN_180017970 and BinkDoFramePlane.
void* open_decoder(void* name, const OpenOptions* options, std::uint32_t flags);
std::uint64_t decode_frame_plane(void* handle, std::uint32_t flags);
} // namespace internal
} // namespace t6_bink

extern "C" void* BinkOpen(void* name, std::uint32_t flags) {
    // The original stores each of these on its stack, then resets the one-shot
    // options BEFORE calling FUN_180017970.
    const t6_bink::OpenOptions opts = {
        t6_bink::g_file_offset,
        t6_bink::g_frame_rate_numerator,
        t6_bink::g_frame_rate_denominator,
        t6_bink::g_io_size,
        t6_bink::g_simulate,
        t6_bink::g_sound_track_count,
        0, // unobserved padding: no claim about contents in original
        t6_bink::g_sound_tracks,
        t6_bink::g_secondary_file_offset,
    };
    t6_bink::g_frame_rate_numerator = UINT32_MAX;
    t6_bink::g_io_size = UINT32_MAX;
    t6_bink::g_simulate = UINT32_MAX;
    t6_bink::g_file_offset = 0;
    t6_bink::g_secondary_file_offset = 0;
    return t6_bink::internal::open_decoder(name, &opts, flags);
}

extern "C" std::uint64_t BinkDoFrame(void* handle) {
    return t6_bink::internal::decode_frame_plane(handle, 0x303U);
}
