#pragma once
#include <cstdint>

namespace t6_bink {
// Original file I/O subobject at HBINK+0x1F0, size at least 0x110.
// Host file seek/read are the only OS-boundary substitutions here.
// The ring itself, callback slots, flags and 32-bit counters retain the
// original offsets. Null optional callbacks have original semantics.
using FileSeek = void (*)(void*,std::uint64_t);
using FileRead = std::uint32_t (*)(void*,void*,std::uint32_t);
using FileFill = std::int64_t (*)(void*,void*);
}
extern "C" std::uint32_t bink_research_file_io_reserve(std::uint32_t bytes);
extern "C" int bink_research_file_set_buffer(void* io,void* memory,std::uint32_t bytes,
    std::uint32_t file_end);
extern "C" std::int64_t bink_research_file_read_header(void* io,std::uint32_t offset,
    void* dest,std::uint32_t bytes,t6_bink::FileSeek seek,t6_bink::FileRead read,void* context);
// Sequential refill branch at 0x20AA0. Reject unresolved loop/realign
// transition and invalid ring before host I/O. 0x20000 free bytes required.
extern "C" std::int64_t bink_research_file_refill(void* io,t6_bink::FileRead read,void* context);
// Full control state transitions, including high-bit callback dispatch.
extern "C" std::uint32_t bink_research_file_control(void* io,std::uint32_t flags,
    t6_bink::FileFill fill,void* context);
