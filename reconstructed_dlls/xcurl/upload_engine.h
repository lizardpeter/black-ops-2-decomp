#pragma once
#include "transfer_callbacks.h"
namespace t6_xcurl {
struct UploadServices {
    const RequestServices* request;
    std::uint32_t(*write)(void*,const void*,std::uint32_t,void*);
    // Original 0x10420: append custom terminal chunk/trailers.
    std::uint32_t(*trailers)(void*,std::uint64_t*,void*);
    void(*diagnose)(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*);
};
}
extern "C" void xcurl_research_upload_continue(void*,const t6_xcurl::UploadServices*);
extern "C" void xcurl_research_upload_worker(void*,const t6_xcurl::UploadServices*);

extern "C" void xcurl_bound_upload_worker(void*,const t6_xcurl::UploadServices*);
