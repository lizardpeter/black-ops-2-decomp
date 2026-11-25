#include "gdk_runtime_win32.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace t6_xcurl {
namespace {
struct Host {
    SRWLOCK lock=SRWLOCK_INIT;
    GdkRuntime runtime{};
    GdkRuntimeServices services{};
    GdkQueryAdapter adapter{};
    GdkQueryServices query{};
    Host(){
        services={lock_exclusive,unlock_exclusive,load_library,resolve_export,initialize_runtime,
                  query_runtime,report_runtime,debug_flags,debug_break,this,uninitialize_runtime,unload_library};
        adapter={&runtime,&services};
        query={xcurl_research_gdk_query_adapter,&adapter};
    }
    static void lock_exclusive(void* c){::AcquireSRWLockExclusive(&static_cast<Host*>(c)->lock);}
    static void unlock_exclusive(void* c){::ReleaseSRWLockExclusive(&static_cast<Host*>(c)->lock);}
    static void* load_library(const char* name,std::uint32_t flags,void*){return ::LoadLibraryExA(name,nullptr,flags);}
    static void* resolve_export(void* module,const char* name,void*){return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(module),name));}
    static std::uint32_t initialize_runtime(void* fn,std::uint64_t a,std::uint64_t b,std::uint64_t flags,void*){
        if(a>0xffffffffULL){using Fn=HRESULT(WINAPI*)(std::uint64_t,std::uint64_t,std::uint64_t);return static_cast<std::uint32_t>(reinterpret_cast<Fn>(fn)(a,b,flags));}
        using Fn=HRESULT(WINAPI*)(std::uint32_t,std::uint32_t);return static_cast<std::uint32_t>(reinterpret_cast<Fn>(fn)(static_cast<std::uint32_t>(a),static_cast<std::uint32_t>(b)));
    }
    static std::uint32_t query_runtime(void* fn,const void* cls,const void* iid,void** out,void*){
        using Fn=HRESULT(WINAPI*)(const void*,const void*,void**);return static_cast<std::uint32_t>(reinterpret_cast<Fn>(fn)(cls,iid,out));
    }
    static void report_runtime(void* fn,std::uint32_t status,const char* message,void*){
        using Fn=void(WINAPI*)(std::uint32_t,const char*);reinterpret_cast<Fn>(fn)(status,message);
    }
    static std::uint8_t debug_flags(void*){return *reinterpret_cast<volatile std::uint8_t*>(0x7ffe02d4);}
    static void debug_break(void*){::DebugBreak();}
    static void uninitialize_runtime(void* fn,void*){using Fn=void(WINAPI*)();reinterpret_cast<Fn>(fn)();}
    static void unload_library(void* module,void*){::FreeLibrary(static_cast<HMODULE>(module));}
};
Host& host(){static Host instance;return instance;}
}
const GdkQueryServices* win32_gdk_query_services(){return &host().query;}
std::uint32_t win32_gdk_initialize(){auto& h=host();return xcurl_research_gdk_initialize(&h.runtime,0,&h.services);}
void win32_gdk_shutdown(){auto& h=host();xcurl_research_gdk_shutdown(&h.runtime,&h.services);}
}
#endif
