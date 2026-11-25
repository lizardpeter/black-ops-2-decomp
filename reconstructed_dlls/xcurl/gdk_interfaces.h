#pragma once
#include <cstdint>
namespace t6_xcurl {
struct GdkQueryServices {
    // Models QueryApiImpl after the original lazy-initialization path has run.
    std::uint32_t (*query)(const void* class_id,const void* interface_id,void** out,void* context);
    void* context;
};
}
extern "C" {
std::uint32_t xcurl_research_gdk_async_get_status(void*,std::uint8_t,const t6_xcurl::GdkQueryServices*);
void xcurl_research_gdk_async_cancel(void*,const t6_xcurl::GdkQueryServices*);
void xcurl_research_gdk_error_report(std::uint32_t,const char*,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_network_get_connectivity_hint(void*,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_network_security_async(const char16_t*,void*,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_network_security_result(void*,std::uint64_t,std::uint64_t*,void*,void**,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_network_security_result_size(void*,std::uint64_t*,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_network_register_connectivity(void*,void*,void*,void*,const t6_xcurl::GdkQueryServices*);
std::uint8_t xcurl_research_gdk_network_unregister_connectivity(std::uint64_t,std::uint8_t,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_network_verify_certificate(void*,const void*,const t6_xcurl::GdkQueryServices*);
void xcurl_research_gdk_task_queue_close(void*,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_task_queue_create(std::uint32_t,std::uint32_t,void**,const t6_xcurl::GdkQueryServices*);
std::uint8_t xcurl_research_gdk_task_queue_dispatch(void*,std::uint32_t,std::uint32_t,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_task_queue_submit(void*,std::uint32_t,void*,void*,const t6_xcurl::GdkQueryServices*);
std::uint32_t xcurl_research_gdk_task_queue_terminate(void*,std::uint8_t,void*,void*,const t6_xcurl::GdkQueryServices*);
void xcurl_research_gdk_thread_assert_not_time_sensitive(const t6_xcurl::GdkQueryServices*);
}
