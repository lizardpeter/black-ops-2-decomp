#pragma once
#include "header_validation.h"
#include "resident_frame_state.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace t6_bink {
// 0x17F40 through 0x185**: original allocation-size and cache-line
// distribution arithmetic for a resident BIKi with zero sound tracks
// and the external frame-buffer flag 0x400. The original allocation is
// a 0x5B8 handle, aligned to 0x5C0, followed by seven timing arrays,
// one dummy 0x1C0 sound structure, an empty track-ID list, and the
// zero-track 0x104-byte initialization region. No OS object ownership
// is modeled by this layout function.
struct ResidentBootstrapLayout {
    std::uint32_t history_count=0;
    std::array<std::uint32_t,7> history_offsets{};
    std::uint32_t dummy_audio_offset=0,track_ids_offset=0;
    std::uint32_t init_region_offset=0,allocation_bytes=0,accounted_bytes=0;
};
bool resident_bootstrap_layout(const HeaderPrefix& h,ResidentBootstrapLayout& out,
                               bool explicit_no_tracks=false);
// Validated Store subset. Caller owns the allocation and input bytes;
// the index and packet pointers borrow the resident input. Only observed
// handle fields are initialized, and the original three timer calls
// are preserved. OS mutex/other handle lifetime remains separate.
// Returns false without constructing a usable handle on failure.
bool init_resident_external(void* handle,std::size_t capacity,
    const std::uint8_t* data,std::size_t bytes,std::uint32_t flags,
    ResidentTimer timer,void* timer_context);
}
extern "C" std::uint32_t bink_research_resident_bootstrap_size(const std::uint8_t*,std::size_t,std::uint32_t flags);
extern "C" int bink_research_init_resident_external(void*,std::size_t,
    const std::uint8_t*,std::size_t,std::uint32_t,t6_bink::ResidentTimer,void*);
