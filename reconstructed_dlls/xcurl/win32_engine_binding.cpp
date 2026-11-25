// Native binding for original engine OS calls and callbacks 0x16960/0x16B60.
// Registration import and argument order from 0x16C04 and 0x165D0.
// ETW provider lifecycle remains an explicit host service.
#include "win32_engine_binding.h"
#ifdef _WIN32
#include "allocation.h"
#include <exception>
namespace t6_xcurl {
std::atomic<Win32EngineBinding*> Win32EngineBinding::active_{nullptr};
Win32EngineBinding::Win32EngineBinding(const GdkQueryServices& g,const SessionServices& s,const SchedulerServices& sched,const EnginePlatformHost& h)
    :host_(h),services_{&g,&s,&sched,init_cs,delete_cs,enter,leave,set_event,close_handle,release_mutex,wait_one,create_mutex,last_error,
     telemetry_create,telemetry_destroy,register_app,unregister_app,reinterpret_cast<void*>(connectivity_entry),
     reinterpret_cast<void*>(app_state_entry),allocate,release,initialize,frequency,this}{
    init_cs(&profile_lock_,nullptr);
}
Win32EngineBinding::~Win32EngineBinding(){
    // Never unload the notification provider with a live callback context.
    if(!deactivate())std::terminate();
    delete_cs(&profile_lock_,nullptr);
}
bool Win32EngineBinding::activate(){
    if(!host_.telemetry_create||!host_.telemetry_destroy)return false;
    Win32EngineBinding* expected=nullptr;return active_.compare_exchange_strong(expected,this)||expected==this;
}
bool Win32EngineBinding::deactivate(){
    if(registrations_)return false;
    auto* expected=this;active_.compare_exchange_strong(expected,nullptr);
    if(app_module_){::FreeLibrary(app_module_);app_module_=nullptr;register_app_=nullptr;unregister_app_=nullptr;}
    return true;
}
void Win32EngineBinding::bind_api(ApiBinding& api,std::uint32_t limit){
    api.engine=&services_;api.engine_limit=limit;api.profile_lock=&profile_lock_;
    api.enter=enter;api.leave=leave;api.tick=tick;api.shutdown_runtime=shutdown;
}
void Win32EngineBinding::connectivity_entry(void* engine,const void*){
    if(auto* b=active_.load();b&&engine)xcurl_research_engine_connectivity(engine,&b->services_);
}
void Win32EngineBinding::app_state_entry(std::uint8_t quiesced,void* engine){
    if(auto* b=active_.load();b&&engine)xcurl_research_engine_app_state(quiesced,engine,&b->services_);
}
Win32EngineBinding& Win32EngineBinding::self(void* c){return *static_cast<Win32EngineBinding*>(c);}
void Win32EngineBinding::init_cs(void* p,void*){if(!::InitializeCriticalSectionEx(static_cast<CRITICAL_SECTION*>(p),0,0))std::terminate();}
void Win32EngineBinding::delete_cs(void* p,void*){::DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(p));}
void Win32EngineBinding::enter(void* p,void*){::EnterCriticalSection(static_cast<CRITICAL_SECTION*>(p));}
void Win32EngineBinding::leave(void* p,void*){::LeaveCriticalSection(static_cast<CRITICAL_SECTION*>(p));}
void Win32EngineBinding::set_event(void* p,void*){::SetEvent(p);}
void Win32EngineBinding::close_handle(void* p,void*){::CloseHandle(p);}
void Win32EngineBinding::release_mutex(void* p,void*){::ReleaseMutex(p);}
std::uint32_t Win32EngineBinding::wait_one(void* p,std::uint32_t timeout,void*){return ::WaitForSingleObject(p,timeout);}
void* Win32EngineBinding::create_mutex(void*){return ::CreateMutexW(nullptr,FALSE,nullptr);}
std::uint32_t Win32EngineBinding::last_error(void*){return ::GetLastError();}
std::uint32_t Win32EngineBinding::telemetry_create(void** out,void* c){auto& h=self(c).host_;return h.telemetry_create(out,h.context);}
void Win32EngineBinding::telemetry_destroy(void* p,void* c){auto& h=self(c).host_;h.telemetry_destroy(p,h.context);}
std::uint32_t Win32EngineBinding::register_app(void* cb,void* engine,void** out,void* c){
    auto& b=self(c);if(active_.load()!=&b)return 0x8000ffffU;
    if(!b.app_module_){
        b.app_module_=::LoadLibraryExW(L"api-ms-win-core-psm-appnotify-l1-1-0.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!b.app_module_)return HRESULT_FROM_WIN32(::GetLastError());
        b.register_app_=reinterpret_cast<RegisterApp>(::GetProcAddress(b.app_module_,"RegisterAppStateChangeNotification"));
        b.unregister_app_=reinterpret_cast<UnregisterApp>(::GetProcAddress(b.app_module_,"UnregisterAppStateChangeNotification"));
        if(!b.register_app_||!b.unregister_app_){::FreeLibrary(b.app_module_);b.app_module_=nullptr;b.register_app_=nullptr;b.unregister_app_=nullptr;return 0x8007007fU;}
    }
    *out=nullptr;auto result=b.register_app_(reinterpret_cast<AppCallback>(cb),engine,out);
    // Native API returns Win32 error codes. Preserve HRESULT failure semantics
    // expected by the recovered engine instead of accepting positive errors.
    if(result)return result&0x80000000U?result:HRESULT_FROM_WIN32(result);
    if(!*out)return 0x80004005U;
    ++b.registrations_;return 0;
}
void Win32EngineBinding::unregister_app(void* p,void* c){
    auto& b=self(c);if(p&&b.unregister_app_){b.unregister_app_(p);if(b.registrations_)--b.registrations_;}
}
void* Win32EngineBinding::allocate(std::size_t n,std::uint32_t t,void*){return xcurl_research_heap_allocate(n,t);}
void Win32EngineBinding::release(void* p,std::uint32_t t,void*){xcurl_research_heap_free(p,t);}
std::uint32_t Win32EngineBinding::initialize(void* c){if(active_.load()!=&self(c))return 0x8000ffffU;return win32_gdk_initialize();}
std::uint32_t Win32EngineBinding::frequency(std::uint64_t* out,void*){LARGE_INTEGER n{};auto ok=::QueryPerformanceFrequency(&n);*out=static_cast<std::uint64_t>(n.QuadPart);return ok;}
std::uint64_t Win32EngineBinding::tick(void*){LARGE_INTEGER n{};::QueryPerformanceCounter(&n);return static_cast<std::uint64_t>(n.QuadPart);}
void Win32EngineBinding::shutdown(void*){win32_gdk_shutdown();}
}
#endif
