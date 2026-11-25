#pragma once
#include "http_setup.h"
namespace t6_xcurl {
// All internal references in this translation are dispatched directly, not
// through an original-machine address. vtable_identity is an explicit layout
// boundary until the native Windows ABI adapter supplies the actual vtable.
struct SessionServices {
    Lock enter,leave;
    OptionAllocate allocate;
    OptionFree release;
    void* (*open_session)(const char16_t*,std::uint32_t,void*);
    std::uint32_t (*set_option)(void*,std::uint32_t,std::uint32_t,void*);
    LastError error;
    EventAction close;
    Diagnostic diagnostic;
    const void* vtable_identity;
    void* context;
};
}
extern "C" {
std::uint32_t xcurl_research_session_create(void*,void**,const t6_xcurl::SessionServices*);
std::uint32_t xcurl_research_session_addref(void*);
std::uint32_t xcurl_research_session_release(void*,const t6_xcurl::SessionServices*);
std::uint32_t xcurl_research_cache_entry_create(std::uint32_t,const char16_t*,std::uint32_t,void*,void**,const t6_xcurl::SessionServices*);
void xcurl_research_cache_entry_destroy(void*,const t6_xcurl::SessionServices*);
std::uint32_t xcurl_research_session_acquire(void*,void*,std::uint32_t,std::uint32_t,void**,const t6_xcurl::SessionServices*);
// Production integration variant: preserve the original cache key and layout,
// but close failed secure sessions and reject inconsistent host success codes.
std::uint32_t xcurl_bound_session_acquire(void*,void*,std::uint32_t,std::uint32_t,void**,const t6_xcurl::SessionServices*);
void xcurl_research_session_cache_clear(void*,const t6_xcurl::SessionServices*);
}
