#pragma once
#include "response_body.h"
namespace t6_xcurl {
// Store callbacks use the platform x64 ABI, including size_t count.
using TransferCallback=std::uint64_t(*)(void*,std::uint64_t,std::uint64_t,void*);
struct HeaderServices {
    const RequestServices* request;
    // IDs: 0 = header worker B4C0, 1 = upload continuation C9C0.
    std::uint32_t(*submit)(void*,void*,std::uint32_t,void*);
    void(*diagnose)(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*);
};
}
extern "C" {
std::uint64_t xcurl_research_invoke_write(t6_xcurl::TransferCallback,const void*,std::uint64_t,void*);
std::uint64_t xcurl_research_invoke_read(void*,void*,std::uint64_t,const t6_xcurl::RequestServices*);
std::uint32_t xcurl_research_queue_headers(void*,const t6_xcurl::HeaderServices*);
std::uint32_t xcurl_research_queue_upload(void*,const t6_xcurl::HeaderServices*);
void xcurl_research_header_worker(void*,const t6_xcurl::HeaderServices*);
}
