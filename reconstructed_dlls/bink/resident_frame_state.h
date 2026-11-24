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

namespace t6_bink {
// Original queue boundary 0x21F70: worker ID, 8-byte tagged handle.
using AsyncEnqueue = int (*)(std::uint32_t worker,std::uintptr_t message,void* context);
}
// Original video-only BinkDoFrameAsync (0x213F0). Callback injection
// models only worker queue submission, not successful worker execution.
// Caller owns handle, dirty map and allocator. error[256] and worker_entry
// model the original global state, including error copy on busy submit.
// Returns 0 or 1 as original, -1 for unsupported input.
extern "C" int bink_research_submit_async(void* handle,std::uint32_t first,
    std::uint32_t second,t6_bink::ResidentTimer timer,void* timer_context,
    t6_bink::RectAllocate allocate,void* allocate_context,
    t6_bink::AsyncEnqueue enqueue,void* queue_context,char* error,
    std::uintptr_t* worker_entry,std::uintptr_t entry_address);
// Returns 1 for original idle/null path, -1 for outstanding worker state.
// No synchronous substitute for the original wait loop.
extern "C" int bink_research_async_wait_idle(const void* handle);

namespace t6_bink {
struct AsyncCompletion {
    std::uintptr_t tagged_handle;
    std::int64_t result;
};
static_assert(sizeof(AsyncCompletion)==16);
// Queue receives exactly the original 16-byte completion message, with
// the original signed wait timeout. Optional queue error is borrowed.
using AsyncReceive = int (*)(std::uint32_t worker,std::int32_t timeout,
    AsyncCompletion* completion,const char** queue_error,void* context);
using AsyncNotify = void (*)(std::uint32_t worker,void* context);
}
// Original BinkDoFrameAsyncWait (0x21590), resident audio-free paths.
// Masks correspond to original globals 0x570BC and 0x570A4. Receive
// and notify boundaries correspond to 0x21BE0 and 0x21FF0. All returned
// tagged handles must be valid original-layout handle storage. The
// original drains a worker's completion queue after its first receipt;
// cross-handle completion clears the other handle's pending bit and
// notifies its worker. Returns original 0 (timeout) or 1 (completed),
// -1 on unsupported input, rather than faking completion.
extern "C" int bink_research_wait_async(void* handle,std::int32_t timeout,
    std::uint32_t active_mask,std::uint32_t running_mask,
    t6_bink::ResidentTimer timer,void* timer_context,
    t6_bink::AsyncReceive receive,t6_bink::AsyncNotify notify,
    void* queue_context,char* error);

 // Original select-frame streaming scratch alignment (0x16530) and pending
 // read setup; only audio-free, sequential/restart frames with no pending
 // prior read. The original I/O callback is executed by the stream adapter.
extern "C" int bink_research_select_stream_frame(void* handle,std::uint32_t frame,
    t6_bink::ResidentTimer timer,void* timer_context);
extern "C" int bink_research_next_stream_frame(void* handle,
    t6_bink::ResidentTimer timer,void* timer_context);
