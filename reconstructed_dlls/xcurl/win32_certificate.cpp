#include "win32_certificate.h"
#ifdef _WIN32
#include "certificate.h"
#include <wincrypt.h>
#include <cstring>
#include <string>
#include <new>
namespace t6_xcurl {
namespace {
std::uint32_t last_failure(){auto n=::GetLastError();return n?static_cast<std::uint32_t>(HRESULT_FROM_WIN32(n)):0x80004005U;}
template<class T>T read(const void* p,unsigned off){T v;std::memcpy(&v,static_cast<const char*>(p)+off,sizeof(v));return v;}
}
std::uint32_t win32_select_client_certificate(void* request,const char16_t* selector,WinHttpHost& host){
    if(!selector)return 0;
    auto* easy=read<void*>(request,0x30);
    auto diag=[&](unsigned site,std::uint32_t code){if(host.diagnostic)host.diagnostic(easy,site,code,host.diagnostic_context);};
    CertificateSelector parsed{};
    if(xcurl_research_parse_certificate(selector,&parsed)||!parsed.store_length){
        diag(0x21130,0x80092004U);return 0x80092004U;
    }
    // The original selector is a SHA-1 certificate thumbprint, not a
    // certificate trust override or a server certificate pin.
    BYTE hash[20]{};DWORD bytes=sizeof(hash);
    if(!::CryptStringToBinaryW(reinterpret_cast<LPCWSTR>(parsed.thumbprint),40,CRYPT_STRING_HEXRAW,hash,&bytes,nullptr,nullptr)||bytes!=sizeof(hash)){
        diag(0x21168,0x80092004U);return 0x80092004U;
    }
    std::u16string name;
    try{name.assign(parsed.store,parsed.store_length);}catch(const std::bad_alloc&){return 0x8007000eU;}
    HCERTSTORE store=::CertOpenStore(CERT_STORE_PROV_SYSTEM_W,0,0,parsed.location|CERT_STORE_OPEN_EXISTING_FLAG|CERT_STORE_READONLY_FLAG,reinterpret_cast<LPCWSTR>(name.c_str()));
    if(!store){auto status=last_failure();diag(0x210f8,status);return status;}
    CRYPT_HASH_BLOB blob{bytes,hash};
    PCCERT_CONTEXT cert=::CertFindCertificateInStore(store,X509_ASN_ENCODING|PKCS_7_ASN_ENCODING,0,CERT_FIND_HASH,&blob,nullptr);
    std::uint32_t status=0;
    if(!cert)status=0x80092004U;
    else{
        // WinHTTP expects the actual CERT_CONTEXT, not its pointer's address.
        // Do not copy the original unchecked option call into production.
        static_assert(sizeof(CERT_CONTEXT)==40);
        if(!::WinHttpSetOption(read<HINTERNET>(request,0x50),WINHTTP_OPTION_CLIENT_CERT_CONTEXT,const_cast<PCERT_CONTEXT>(cert),sizeof(CERT_CONTEXT)))status=last_failure();
        ::CertFreeCertificateContext(cert);
    }
    ::CertCloseStore(store,0);
    if(status)diag(0x21168,status);
    return status;
}
}
#endif
