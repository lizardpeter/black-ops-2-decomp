// Direct machine-code transcriptions from pinned Microsoft Store binkw64.dll.
// Original SHA-256: 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
// IMPORTANT: this standalone research source is not linked into BO2. Only these
// four setter/registration bodies are decoded; the decoder and initialization
// remain unknown. Do not use this as a drop-in replacement DLL.
#include <cstddef>
#include <cstdint>

namespace t6_bink {
using Allocate = void* (*)(std::size_t);
using Deallocate = void (*)(void*);

// Original global RVAs noted beside each state variable. Original default
// allocator callbacks have yet to be reconstructed; the setter itself is exact.
Allocate g_allocate = nullptr;                    // 0x542A8
Deallocate g_deallocate = nullptr;                // 0x542B0
std::uint32_t g_io_size = UINT32_MAX;             // 0x570E0
std::uint32_t g_sound_track_count = 0;            // 0x570B0
std::uint32_t g_sound_tracks[32] = {};            // 0x57250
}

// RVA 0x1DA30: mov [global_alloc], rcx; mov [global_free], rdx; ret
extern "C" void BinkSetMemory(t6_bink::Allocate alloc, t6_bink::Deallocate free_fn) {
    t6_bink::g_allocate = alloc;
    t6_bink::g_deallocate = free_fn;
}

// RVA 0x16840: mov dword ptr [g_io_size], ecx; ret
extern "C" void BinkSetIOSize(std::uint32_t size) {
    t6_bink::g_io_size = size;
}

// RVA 0x16870: unsigned clamp to 32 and copy that many 32-bit track IDs.
extern "C" void BinkSetSoundTrack(std::uint32_t count, const std::uint32_t* tracks) {
    if (count > 32U) count = 32U;
    t6_bink::g_sound_track_count = count;
    for (std::uint32_t i = 0; i < count; ++i) {
        t6_bink::g_sound_tracks[i] = tracks[i];
    }
}

// RVA 0x17950: if (rcx && rdx) *(void**)(rcx + 0xD0) = rdx.
extern "C" void BinkRegisterFrameBuffers(void* handle, void* frame_buffers) {
    if (handle && frame_buffers) {
        auto* member = reinterpret_cast<void**>(static_cast<std::uint8_t*>(handle) + 0xD0);
        *member = frame_buffers;
    }
}
