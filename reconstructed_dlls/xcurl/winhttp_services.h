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
namespace t6_xcurl {
// Diagnostic and status callback are supplied by the caller. These services
// bind only original OS boundaries; no unfinished queue/engine callbacks are filled.
struct WinHttpHost {
    WINHTTP_STATUS_CALLBACK status_callback;
    Diagnostic diagnostic;
    void* diagnostic_context;
};
SessionServices winhttp_session_services(WinHttpHost*);
HttpSetupServices winhttp_setup_services(WinHttpHost*);
HeaderQueryServices winhttp_header_query_services(WinHttpHost*);
AddHeaderServices winhttp_add_header_services(const RequestServices*);
UploadServices winhttp_upload_services(const RequestServices*,std::uint32_t(*)(void*,std::uint64_t*,void*),void(*)(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*));
void bind_winhttp_request_services(RequestServices&,WinHttpHost*);
}
#endif
