// Dispatcher control-flow tests. The internal functions below are TEST
// FIXTURES: they are not decoder implementations or differential verification.
#include <cassert>
#include <cstdint>
#include "bink_small.cpp"
#include "bink_leaf.cpp"
#include "bink_dispatch.cpp"

namespace {
std::uint32_t observed_flags = 0;
const void* observed_name = nullptr;
int open_calls = 0;
int decode_calls = 0;
}
namespace t6_bink::internal {
void* open_decoder(void* name, const OpenOptions* o, std::uint32_t flags) {
    ++open_calls;
    observed_name = name;
    observed_flags = flags;
    assert(o->file_offset == UINT64_C(0x1122334455667788));
    assert(o->frame_rate_number == 24000);
    assert(o->frame_rate_denominator == 1001);
    assert(o->io_size == 4096);
    assert(o->simulate == 4);
    assert(o->track_count == 3);
    assert(o->track_ids == g_sound_tracks);
    assert(o->track_ids[0] == 7 && o->track_ids[1] == 8 && o->track_ids[2] == 9);
    assert(o->secondary_offset == 0x12345678);
    // The original does not initialize padding: do not assert gap.
    assert(g_file_offset == 0);
    assert(g_secondary_file_offset == 0);
    assert(g_frame_rate_numerator == UINT32_MAX);
    assert(g_io_size == UINT32_MAX);
    assert(g_simulate == UINT32_MAX);
    return reinterpret_cast<void*>(0x12340);
}
std::uint64_t decode_frame_plane(void* handle, std::uint32_t flags) {
    ++decode_calls;
    assert(handle == reinterpret_cast<void*>(0x12340));
    assert(flags == 0x303U);
    return UINT64_C(0x123456789ABCDEF0);
}
}
int main() {
    BinkSetFileOffset(UINT64_C(0x1122334455667788));
    BinkSetFrameRate(24000, 1001);
    BinkSetIOSize(4096);
    BinkSetSimulate(4);
    std::uint32_t track_ids[] = {7, 8, 9};
    BinkSetSoundTrack(3, track_ids);
    t6_bink::g_secondary_file_offset = 0x12345678;
    auto* instance = BinkOpen(reinterpret_cast<void*>(0x23450), 0x42);
    assert(instance == reinterpret_cast<void*>(0x12340));
    assert(open_calls == 1 && observed_flags == 0x42);
    assert(observed_name == reinterpret_cast<void*>(0x23450));
    assert(BinkDoFrame(instance) == UINT64_C(0x123456789ABCDEF0));
    assert(decode_calls == 1);
}
