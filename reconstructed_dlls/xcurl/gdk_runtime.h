#pragma once
#include "gdk_interfaces.h"
#include <cstdint>
namespace t6_xcurl {
struct GdkRuntime {
    std::uint8_t initialized=0;
    std::uint8_t restricted=0;
    std::uint8_t reserved[6]{};
    void* module=nullptr;
    void* exports[5]{};
};
struct GdkRuntimeServices {
    void (*lock)(void*);
    void (*unlock)(void*);
    void* (*load)(const char*,std::uint32_t,void*);
    void* (*resolve)(void*,const char*,void*);
    std::uint32_t (*initialize)(void*,std::uint64_t,std::uint64_t,std::uint64_t,void*);
    std::uint32_t (*query)(void*,const void*,const void*,void**,void*);
    void (*report)(void*,std::uint32_t,const char*,void*);
    std::uint8_t (*debug_flags)(void*);
    void (*debug_break)(void*);
    void* context;
    void (*uninitialize)(void*,void*);
    void (*unload)(void*,void*);
};
struct GdkQueryAdapter {
    GdkRuntime* runtime;
    const GdkRuntimeServices* services;
};
}
extern "C" {
std::uint32_t xcurl_research_gdk_load(t6_xcurl::GdkRuntime*,const t6_xcurl::GdkRuntimeServices*);
std::uint32_t xcurl_research_gdk_initialize(t6_xcurl::GdkRuntime*,std::uint64_t,const t6_xcurl::GdkRuntimeServices*);
void xcurl_research_gdk_shutdown(t6_xcurl::GdkRuntime*,const t6_xcurl::GdkRuntimeServices*);
std::uint32_t xcurl_research_gdk_query(t6_xcurl::GdkRuntime*,const void*,const void*,void**,const t6_xcurl::GdkRuntimeServices*);
std::uint32_t xcurl_research_gdk_query_adapter(const void*,const void*,void**,void*);
}
