#pragma once
#include "upload_buffer.h"
namespace t6_xcurl {
// GDK asynchronous result APIs and not-yet-connected setup helpers.
// Already recovered helpers are composed through the native adapter rather
// than raw original function addresses. Callers must provide each boundary.
struct StartServices {
    const RequestServices* request;
    const BufferServices* buffer;
    std::uint32_t (*async_status)(void*,void*);
    std::uint32_t (*async_size)(void*,std::uint64_t*,void*);
    std::uint32_t (*async_result)(void*,std::uint64_t,void*,void**,void*);
    std::uint32_t (*crack)(const char16_t*,void*,void*,void*);
    std::uint32_t (*open)(void*,void*,void*,void*);
    std::uint32_t (*register_request)(void*,void*);
    std::uint32_t (*options)(void*,const void*,std::uint32_t,void*);
    std::uint32_t (*headers)(void*,void*,void*);
    std::uint32_t (*certificate)(void*,void*,void*);
    void (*diagnose)(void*,std::uint32_t,std::uint32_t,void*);
};
}
extern "C" void xcurl_research_start_request(void*,void*,const t6_xcurl::StartServices*);
extern "C" void xcurl_research_start_worker(void*,const t6_xcurl::StartServices*);
