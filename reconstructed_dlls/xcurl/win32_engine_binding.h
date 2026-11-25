#pragma once
#ifdef _WIN32
#include "winhttp_services.h"
#include "gdk_runtime_win32.h"
#include <atomic>
namespace t6_xcurl {
// Application notification entrypoints and actual Win32 primitives.
// Telemetry creation/destruction are deliberately injected until the
// remaining original provider setup and flush functions are reconstructed.
struct EnginePlatformHost {
    std::uint32_t(*telemetry_create)(void**,void*);
    EventAction telemetry_destroy;
    void* context;
};
class Win32EngineBinding {
public:
    Win32EngineBinding(const GdkQueryServices&,const SessionServices&,const SchedulerServices&,const EnginePlatformHost&);
    ~Win32EngineBinding();
    Win32EngineBinding(const Win32EngineBinding&)=delete;
    Win32EngineBinding& operator=(const Win32EngineBinding&)=delete;
    bool activate();
    bool deactivate(); // only after engine and all registrations are gone
    const EngineServices& services() const{return services_;}
    void bind_api(ApiBinding&,std::uint32_t limit);
    static void connectivity_entry(void*,const void*);
    static void app_state_entry(std::uint8_t,void*);
private:
    EnginePlatformHost host_;
    EngineServices services_;
    CRITICAL_SECTION profile_lock_{};
    HMODULE app_module_=nullptr;
    using AppCallback=void(WINAPI*)(BOOLEAN,void*);
    using RegisterApp=ULONG(WINAPI*)(AppCallback,void*,void**);
    using UnregisterApp=void(WINAPI*)(void*);
    RegisterApp register_app_=nullptr;
    UnregisterApp unregister_app_=nullptr;
    unsigned registrations_=0;
    static std::atomic<Win32EngineBinding*> active_;
    static Win32EngineBinding& self(void*);
    static void init_cs(void*,void*);
    static void delete_cs(void*,void*);
    static void enter(void*,void*);
    static void leave(void*,void*);
    static void set_event(void*,void*);
    static void close_handle(void*,void*);
    static void release_mutex(void*,void*);
    static std::uint32_t wait_one(void*,std::uint32_t,void*);
    static void* create_mutex(void*);
    static std::uint32_t last_error(void*);
    static std::uint32_t telemetry_create(void**,void*);
    static void telemetry_destroy(void*,void*);
    static std::uint32_t register_app(void*,void*,void**,void*);
    static void unregister_app(void*,void*);
    static void* allocate(std::size_t,std::uint32_t,void*);
    static void release(void*,std::uint32_t,void*);
    static std::uint32_t initialize(void*);
    static std::uint32_t frequency(std::uint64_t*,void*);
    static std::uint64_t tick(void*);
    static void shutdown(void*);
};
}
#endif
