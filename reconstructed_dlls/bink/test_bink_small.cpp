// Unit tests of initial machine-code transcriptions; NOT differential DLL tests.
#include <cassert>
#include <cstddef>
#include <cstdint>
#include "bink_small.cpp"

static void* allocator(std::size_t) { return nullptr; }
static void deallocator(void*) {}

int main() {
    BinkSetIOSize(12345);
    assert(t6_bink::g_io_size == 12345);

    BinkSetMemory(allocator, deallocator);
    assert(t6_bink::g_allocate == allocator);
    assert(t6_bink::g_deallocate == deallocator);

    std::uint32_t tracks[40];
    for (std::uint32_t i = 0; i < 40; ++i) tracks[i] = i + 10;
    BinkSetSoundTrack(40, tracks);
    assert(t6_bink::g_sound_track_count == 32);
    for (std::uint32_t i = 0; i < 32; ++i) assert(t6_bink::g_sound_tracks[i] == tracks[i]);
    BinkSetSoundTrack(0, nullptr);
    assert(t6_bink::g_sound_track_count == 0);
    BinkSetSoundTrack(2, tracks);
    assert(t6_bink::g_sound_track_count == 2);
    assert(t6_bink::g_sound_tracks[0] == tracks[0] && t6_bink::g_sound_tracks[1] == tracks[1]);

    alignas(void*) unsigned char handle[512] = {};
    int buffer_token;
    BinkRegisterFrameBuffers(handle, &buffer_token);
    assert(*reinterpret_cast<void**>(handle + 0xD0) == &buffer_token);
    BinkRegisterFrameBuffers(handle, nullptr);
    assert(*reinterpret_cast<void**>(handle + 0xD0) == &buffer_token);
    BinkRegisterFrameBuffers(nullptr, &buffer_token);
    return 0;
}
