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
