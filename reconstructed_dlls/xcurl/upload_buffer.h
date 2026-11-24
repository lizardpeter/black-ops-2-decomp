#pragma once
#include "transfer_callbacks.h"
namespace t6_xcurl {
struct BufferServices {
    const RequestServices* request;
    void* (*reallocate)(void*,std::size_t,std::uint32_t,void*);
    void (*diagnose)(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*);
};
}
extern "C" std::uint32_t xcurl_research_allocate_upload(void*,void**,std::uint64_t,const t6_xcurl::BufferServices*);
extern "C" std::uint32_t xcurl_research_buffer_upload(void*,void**,std::uint64_t*,std::uint64_t*,const t6_xcurl::BufferServices*);
