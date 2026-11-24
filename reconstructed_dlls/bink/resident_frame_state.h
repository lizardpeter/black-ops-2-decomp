#pragma once
#include <cstdint>

namespace t6_bink {
// Narrow original-handle state routines for resident, audio-free BIKi.
// 0x16440 (resident branch) and BinkNextFrame at 0x1B120.
// Handle storage must be at least 0x5A8 bytes, with valid 1-based frame
// count and a frame_count+1 entry index at +0x1A0. The resident pointer
// +0x1D8 denotes the first compressed packet, not the file header.
// No outstanding read (+0x138), audio tracks (+0x144), or ETW callbacks.
// Busy asynchronous state is preserved exactly by NextFrame.
// Timer is an injected boundary for original RADTimerRead (0x1FF00).
// No claim of full original handle initialization or streamed I/O.
using ResidentTimer = std::uint32_t (*)(void*);
enum class ResidentStateResult : int { ok=0, busy=1, unsupported=2 };
}

extern "C" int bink_research_select_resident_frame(void* handle,std::uint32_t frame,
    t6_bink::ResidentTimer timer,void* timer_context);
extern "C" int bink_research_next_resident_frame(void* handle,
    t6_bink::ResidentTimer timer,void* timer_context);

// Original audio-free BinkPause (0x1BEF0); returns exact pause argument.
// Invalid/unsupported handles are reported as -1 via the signed status.
extern "C" std::int64_t bink_research_pause_video_only(void* handle,std::uint32_t paused,
    t6_bink::ResidentTimer timer,void* timer_context);
// Original prepare helper 0x1A260, video-only. Returns 1 when ready,
// 0 for original null/duplicate/error return, -1 for unsupported state.
extern "C" int bink_research_prepare_video_frame(void* handle,std::uint32_t now);
// Original finish helper 0x1A380, video-only. The seven caller-owned
// history arrays must each contain handle+0x510 uint32 elements. The
// caller owns the 0xD8 registered descriptor and optional dirty map.
// Returns original 0; -1 for invalid or unsupported state.
extern "C" int bink_research_finish_video_frame(void* handle,std::uint32_t now,
    t6_bink::ResidentTimer timer,void* timer_context);

namespace t6_bink {
// Exact 0x38-byte output of BinkGetRealtime (RVA 0x1C290).
// Field names deliberately describe measured sources, not inferred SDK semantics.
struct RealtimeSnapshot {
    std::uint32_t last_frame, rate_numerator, rate_denominator, sample_frames;
    std::uint32_t elapsed, history_4e8, history_4e0, history_4f8;
    std::uint32_t history_500, history_508, history_4f0;
    std::uint32_t state_298, state_29c, byte_rate;
};
static_assert(sizeof(RealtimeSnapshot)==0x38);
}
// Exact original valid-history arithmetic. Returns 0 (including null handle
// or output) or -1 if the original would divide by zero or access an invalid
// history index. Timer boundary supplied by caller. No output on rejection.
extern "C" int bink_research_get_realtime(void* handle,
    t6_bink::RealtimeSnapshot* output,std::uint32_t frames,
    t6_bink::ResidentTimer timer,void* timer_context);

namespace t6_bink {
using RectAllocate = void* (*)(std::uint32_t bytes,void* context);
using BackgroundIO = std::uint32_t (*)(void* io_state,std::uint32_t control);
}
// Original BinkGetRects 0x1C820: suppressed, lazy allocation, full-frame
// and cached branches. The pending dirty-map scan is explicitly unsupported
// (-1). The allocator is the boundary at original RVA 0x1010; it must return
// a writable block for the requested aligned size. No original allocation
// is silently replaced with a generic allocator.
extern "C" std::int64_t bink_research_get_rects(void* handle,std::uint32_t surface_type,
    t6_bink::RectAllocate allocate,void* context);
// Original BinkControlBackgroundIO 0x1DA60, with telemetry disabled.
// The callback at handle+0x220 is invoked with handle+0x1F0 and the
// original unchanged control argument. Null paths return UINT32_MAX.
extern "C" std::uint32_t bink_research_control_background_io(void* handle,std::uint32_t control);
