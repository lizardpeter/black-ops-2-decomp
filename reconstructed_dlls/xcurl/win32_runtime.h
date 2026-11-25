#pragma once
#ifdef _WIN32
#include "game_api.h"
#include "win32_url_binding.h"
#include "win32_engine_binding.h"
#include "winhttp_pipeline.h"
namespace t6_xcurl {
// Single source-link owner for the native Store XCurl reconstruction.
// No modeled Gaming Services calls in this runtime: it uses the actual
// system GDK loader. All API calls/lifetime transitions must be serialized
// by the enclosing application; callback workers must drain before teardown.
struct Win32RuntimeConfig {
    // Empty selects the local, non-publishing telemetry object lifecycle.
    // Supply both callbacks to replace that policy; partial overrides fail.
    EnginePlatformHost telemetry{};
    std::uint32_t engine_limit=12;
    Diagnostic diagnostic=nullptr;
    ErrorLog api_error=nullptr;
    ActiveRequestLog active_log=nullptr;
    void* diagnostic_context=nullptr;
    const CertificateServices* certificate=nullptr;
    WinHttpPipeline::Trailers trailers=nullptr;
};
class Win32Runtime {
public:
    explicit Win32Runtime(const Win32RuntimeConfig&);
    ~Win32Runtime();
    Win32Runtime(const Win32Runtime&)=delete;
    Win32Runtime& operator=(const Win32Runtime&)=delete;
    // Installation does not initialize GDK or begin a transfer. Normal
    // curl_global_init/easy_init calls perform the recovered initialization.
    bool install();
    // Fails while any API handle or global initialization reference remains.
    bool uninstall();
    bool installed() const noexcept{return installed_;}
    std::uint32_t references() const noexcept{return api_.global.references;}
    bool engine_ready() const noexcept{return api_.global.engine!=nullptr;}
    BootstrapReport startup_report() const noexcept{return api_.startup;}
private:
    const GdkQueryServices& gdk_;
    ApiBinding api_{};
    WinHttpHost host_{};
    RequestServices request_{};
    TaskQueueBinding tasks_;
    Win32UrlBinding urls_;
    WinHttpPipeline pipeline_;
    MultiHostServices os_;
    TransferBinding transfers_;
    Win32EngineBinding engine_;
    EasyLifecycleServices easy_;
    MultiLifecycleServices multi_;
    HandleBinding handles_;
    GameApi facade_;
    bool installed_=false;
};
}
#endif
