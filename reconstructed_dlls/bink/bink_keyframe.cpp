// Original Bink keyframe seek routine recovered from binkw64.dll.
// SHA-256: 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
// Export BinkGetKeyFrame at RVA 0x1B2B0; this function has NO callees.
//
// Reconstructed from the original x64 branch/indexing behavior, not FFmpeg or
// other decoder source. Bink frame numbers here are 1-based. The calling
// contract requires an initialized handle and a valid frame in [1, Frames].
// An invalid non-null handle/index can fault in the original and is not fixed
// silently by this fidelity implementation.
#include <cstdint>
#include <cstring>

namespace t6_bink {
static std::uint32_t seek_read32(const void* base, unsigned offset) {
    std::uint32_t v;
    std::memcpy(&v, static_cast<const std::uint8_t*>(base) + offset, 4);
    return v;
}
static const std::uint32_t* seek_offsets(const void* handle) {
    const std::uint32_t* p = nullptr;
    std::memcpy(&p, static_cast<const std::uint8_t*>(handle) + 0x1A0,
                sizeof(p));
    return p;
}
} // namespace t6_bink

extern "C" std::uint32_t BinkGetKeyFrame(const void* handle,
                                           std::uint32_t frame,
                                           std::uint8_t mode) {
    if (!handle) return 0;
    const auto* offsets = t6_bink::seek_offsets(handle);
    // Bit 7 means ignore a keyframe at the current position.
    if ((mode & 0x80U) == 0 && (offsets[frame - 1] & 1U) != 0)
        return frame;

    if ((mode & 0x7FU) == 0) {
        // Seek backward; frame 1 is the implicit fallback.
        std::int64_t back = std::int64_t(std::int32_t(frame - 2U));
        while (back > 0) {
            if (offsets[back] & 1U) break;
            --back;
        }
        return std::uint32_t(back + 1);
    }

    if ((mode & 0x7FU) == 1) {
        const auto count = t6_bink::seek_read32(handle, 8);
        for (std::uint32_t candidate = frame; candidate < count; ++candidate) {
            if (offsets[candidate] & 1U) return candidate + 1U;
        }
        return 0;
    }

    if ((mode & 0x7FU) == 2) {
        const auto count = t6_bink::seek_read32(handle, 8);
        std::int64_t back = std::int64_t(std::int32_t(frame - 2U));
        std::uint32_t forward = frame;
        for (;;) {
            if (back >= 0 && (offsets[back] & 1U) != 0)
                return std::uint32_t(back + 1);
            if (forward < count) {
                const auto bits = offsets[forward++];
                if (bits & 1U) return forward;
            } else if (back < 0) {
                return 0;
            }
            --back;
        }
    }
    return 0; // Unsupported mode, unless current-frame fast-path matched.
}
