#pragma once
#include "bink_allocator.h"
#include "resident_frame_state.h"
#include <cstdint>
// Linkable Store video-only API subset; NOT a drop-in DLL or complete game ABI.
// BinkGetRects and the global async-worker lifecycle exports are intentionally
// absent until their original implementations are recovered.
extern "C" {
void* BinkOpen(void* input,std::uint32_t flags);
void BinkClose(void* handle);
std::uint64_t BinkDoFrame(void* handle);
int BinkDoFrameAsync(void* handle,std::uint32_t first,std::uint32_t second);
int BinkDoFrameAsyncWait(void* handle,std::int32_t timeout);
void BinkNextFrame(void* handle);
std::uint32_t BinkPause(void* handle,std::uint32_t value);
void BinkGetFrameBuffersInfo(const void* handle,void* info);
void BinkRegisterFrameBuffers(void* handle,void* info);
char* BinkGetError();
void BinkSetError(const char*);
void BinkGetRealtime(void* handle,t6_bink::RealtimeSnapshot* out,std::uint32_t frames);
void BinkSetSoundTrack(std::uint32_t count,const std::uint32_t* tracks);
void BinkSetMemory(t6_bink::Allocate alloc,t6_bink::Deallocate free_fn);
void BinkSetIOSize(std::uint32_t bytes);
std::uint32_t BinkControlBackgroundIO(void* handle,std::uint32_t flags);
}
