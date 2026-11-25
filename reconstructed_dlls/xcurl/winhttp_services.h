#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#include "session_cache.h"
#include "header_query.h"
#include "request_headers.h"
#include "upload_engine.h"
#include "task_queue_binding.h"
#include "transfer_binding.h"
#include "handle_binding.h"
#include "callback_binding.h"
namespace t6_xcurl {
class WinHttpPipeline;
class Win32UrlBinding;
// Diagnostic and status callback are supplied by the caller. These services
// bind original OS boundaries and optionally the recovered GDK task queue.
struct WinHttpHost {
    WINHTTP_STATUS_CALLBACK status_callback;
    Diagnostic diagnostic;
    void* diagnostic_context;
    TaskQueueBinding* tasks=nullptr;
    WinHttpPipeline* pipeline=nullptr;
    Win32UrlBinding* urls=nullptr;
};
SessionServices winhttp_session_services(WinHttpHost*);
HttpSetupServices winhttp_setup_services(WinHttpHost*);
HeaderQueryServices winhttp_header_query_services(WinHttpHost*);
AddHeaderServices winhttp_add_header_services(const RequestServices*);
UploadServices winhttp_upload_services(const RequestServices*,std::uint32_t(*)(void*,std::uint64_t*,void*),void(*)(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*));
void bind_winhttp_request_services(RequestServices&,WinHttpHost*);
MultiHostServices win32_multi_host_services(void* context);
WINHTTP_STATUS_CALLBACK win32_reconstructed_status_callback();
EasyLifecycleServices win32_easy_lifecycle_services(const RequestServices*,const GdkQueryServices*);
MultiLifecycleServices win32_multi_lifecycle_services(const SchedulerServices*,const RequestServices*);
}
#endif
