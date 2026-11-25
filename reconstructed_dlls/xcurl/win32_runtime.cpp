#include "win32_runtime.h"
#ifdef _WIN32
#include <exception>
#include "telemetry_lifecycle.h"
#include "allocation.h"
namespace t6_xcurl {
namespace {
void* alloc(std::size_t n,std::uint32_t t,void*){return xcurl_research_heap_allocate(n,t);}
void release(void* p,std::uint32_t t,void*){xcurl_research_heap_free(p,t);}
std::uint32_t tick32(void*){return ::GetTickCount();}
const TelemetryServices local{alloc,release,tick32,nullptr};
std::uint32_t create(void** out,void*){return xcurl_research_telemetry_create(out,&local);}
void destroy(void* p,void*){xcurl_research_telemetry_destroy_local(p,&local);}
EnginePlatformHost telemetry(const EnginePlatformHost& h){if(!h.telemetry_create&&!h.telemetry_destroy)return {create,destroy,nullptr};return h;}
}
Win32Runtime::Win32Runtime(const Win32RuntimeConfig& c)
    :gdk_(*win32_gdk_query_services()),
     host_{nullptr,c.diagnostic,c.diagnostic_context},
     tasks_(api_.global,request_,gdk_),
     urls_(host_,request_,gdk_),
     pipeline_(api_.global,host_,request_,tasks_,gdk_,urls_.crack_services(),c.certificate,c.trailers),
     os_(win32_multi_host_services(nullptr)),
     transfers_(api_.global,urls_.prepare_services(),os_),
     engine_(gdk_,pipeline_.sessions(),transfers_.scheduler(),telemetry(c.telemetry)),
     easy_(win32_easy_lifecycle_services(&request_,&gdk_)),
     multi_(win32_multi_lifecycle_services(&transfers_.scheduler(),&request_)),
     handles_(api_,easy_,multi_),facade_(api_,handles_,transfers_,&tasks_){
    engine_.bind_api(api_,c.engine_limit);
    api_.error=c.api_error;api_.active_log=c.active_log;api_.context=c.diagnostic_context;
}
Win32Runtime::~Win32Runtime(){
    // Destroying a live runtime would invalidate callbacks and handle memory.
    // Require explicit cleanup by its application owner, not a hidden teardown.
    if(!uninstall())std::terminate();
}
bool Win32Runtime::install(){
    if(installed_)return true;
    if(!urls_.activate())return false;
    if(!pipeline_.activate()){urls_.deactivate();return false;}
    if(!engine_.activate()){pipeline_.deactivate();urls_.deactivate();return false;}
    if(!facade_.install()){
        engine_.deactivate();pipeline_.deactivate();urls_.deactivate();return false;
    }
    installed_=true;return true;
}
bool Win32Runtime::uninstall(){
    if(!installed_)return true;
    if(api_.global.engine||api_.global.references)return false;
    if(!facade_.uninstall())return false;
    if(!engine_.deactivate())std::terminate(); // no live engine can own a registration here
    pipeline_.deactivate();urls_.deactivate();installed_=false;return true;
}
}
#endif
