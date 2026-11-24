#pragma once
#include <cstdint>
namespace t6_xcurl {
struct CertificateSelector {
    std::uint32_t location;
    const char16_t* store;
    std::uint64_t store_length;
    const char16_t* thumbprint;
};
struct CertificateServices {
    void* (*open)(std::uint32_t,const char16_t*,void*);
    std::uint32_t (*decode)(const char16_t*,std::uint32_t,std::uint8_t*,std::uint32_t*,void*);
    void* (*find)(void*,const std::uint8_t*,std::uint32_t,void*);
    // The original passes the address of a certificate pointer and length 40;
    // this service records that ABI shape, not a validated native WinHTTP binding.
    void (*set_certificate)(void*,void*,std::uint32_t,void*);
    void (*free_certificate)(void*,void*);
    void (*close_store)(void*,void*);
    std::uint32_t (*error)(void*);
    void (*diagnose)(void*,std::uint32_t,std::uint32_t,const char16_t*,std::uint32_t,void*);
    void* context;
};
}
extern "C" std::uint32_t xcurl_research_parse_certificate(const char16_t*,t6_xcurl::CertificateSelector*);
extern "C" std::uint32_t xcurl_research_select_certificate(void*,const char16_t*,const t6_xcurl::CertificateServices*);
