#pragma once
#ifdef _WIN32
#include "winhttp_services.h"
#include "url_prepare.h"
#include "url_parse.h"
namespace t6_xcurl {
// Win32 composition of the original URL coordinator and the game-required
// flag-zero parser. All referenced host/request/GDK objects outlive this
// binding. Install before admitting a transfer and drain before destruction.
class Win32UrlBinding {
public:
    Win32UrlBinding(WinHttpHost&,RequestServices&,const GdkQueryServices&);
    ~Win32UrlBinding(){deactivate();}
    Win32UrlBinding(const Win32UrlBinding&)=delete;
    Win32UrlBinding& operator=(const Win32UrlBinding&)=delete;
    bool activate();
    void deactivate();
    const UrlServices& crack_services() const{return crack_;}
    const PrepareServices& prepare_services() const{return prepare_;}
    std::uint32_t prepare(const char*,void*,void*);
private:
    WinHttpHost& host_;
    RequestServices& request_;
    const GdkQueryServices& gdk_;
    UrlObjectServices objects_;
    UrlServices crack_;
    UrlPreparationServices preparation_;
    PrepareServices prepare_;
    static Win32UrlBinding& self(void*);
    static void* allocate(std::size_t,std::uint32_t,void*);
    static void release(void*,std::uint32_t,void*);
    static char* duplicate(const char*,void*);
    static void* create(void*);
    static void destroy(void*,void*);
    static std::uint32_t set(void*,int,const char*,unsigned,void*);
    static std::uint32_t get(void*,int,char**,unsigned,void*);
    static std::uint32_t crack(const char16_t*,void*,void*);
    static std::uint32_t error(void*);
    static void crack_diag(void*,std::uint32_t,const char16_t*,std::uint32_t,void*);
    static char* format_warning(const char16_t*,void*);
    static void report(std::uint32_t,const char*,void*);
    static void prepare_diag(void*,std::uint32_t,std::uintptr_t,std::uint32_t,void*);
    static std::uint32_t prepare_url(void*,void*,void*,void*);
    static std::uint32_t default_url(const void*,std::uint32_t,void**,void*);
    static std::uint32_t parse_port(const void*,void*);
    static std::uint32_t copy_url(const void*,void*,void*);
};
}
#endif
