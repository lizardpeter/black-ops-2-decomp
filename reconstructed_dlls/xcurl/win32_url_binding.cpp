#include "win32_url_binding.h"
#ifdef _WIN32
#include "allocation.h"
#include <cstdlib>
#include <cstring>
namespace t6_xcurl {
Win32UrlBinding::Win32UrlBinding(WinHttpHost& h,RequestServices& r,const GdkQueryServices& g)
    :host_(h),request_(r),gdk_(g),objects_{allocate,release,duplicate,&h},
     crack_{crack,error,crack_diag,format_warning,report,release,&h},
     preparation_{create,set,get,destroy,allocate,release,prepare_diag,&crack_,&h},
     prepare_{&r,prepare_url,default_url,parse_port,prepare_diag,copy_url,&g,nullptr,CallbackBinding::start_token()}{}
bool Win32UrlBinding::activate(){if(host_.urls&&host_.urls!=this)return false;host_.urls=this;return true;}
void Win32UrlBinding::deactivate(){if(host_.urls==this)host_.urls=nullptr;}
Win32UrlBinding& Win32UrlBinding::self(void* c){return *static_cast<WinHttpHost*>(c)->urls;}
void* Win32UrlBinding::allocate(std::size_t n,std::uint32_t t,void*){return xcurl_research_heap_allocate(n,t);}
void Win32UrlBinding::release(void* p,std::uint32_t t,void*){xcurl_research_heap_free(p,t);}
char* Win32UrlBinding::duplicate(const char* p,void* c){
    auto n=std::strlen(p)+1;auto* out=static_cast<char*>(allocate(n,5,c));if(out)std::memcpy(out,p,n);return out;
}
void* Win32UrlBinding::create(void* c){return xcurl_research_url_create(&self(c).objects_);}
void Win32UrlBinding::destroy(void* p,void* c){xcurl_research_url_destroy(p,&self(c).objects_);}
std::uint32_t Win32UrlBinding::set(void* p,int part,const char* text,unsigned flags,void* c){
    if(flags)return 9;return xcurl_research_url_set_http(p,part,text,&self(c).objects_);
}
std::uint32_t Win32UrlBinding::get(void* p,int part,char** out,unsigned flags,void* c){
    return xcurl_research_url_get_flags(p,part,out,flags,&self(c).objects_);
}
std::uint32_t Win32UrlBinding::crack(const char16_t* text,void* parts,void*){
    static_assert(sizeof(URL_COMPONENTS)==0x68);
    return ::WinHttpCrackUrl(reinterpret_cast<LPCWSTR>(text),0,0,static_cast<URL_COMPONENTS*>(parts));
}
std::uint32_t Win32UrlBinding::error(void*){return ::GetLastError();}
void Win32UrlBinding::crack_diag(void* e,std::uint32_t site,const char16_t*,std::uint32_t code,void* c){prepare_diag(e,site,0,code,c);}
char* Win32UrlBinding::format_warning(const char16_t* text,void* c){
    constexpr char prefix[]="WARNING: Insecure HTTP request \"";
    auto n=::WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,reinterpret_cast<LPCWSTR>(text),-1,nullptr,0,nullptr,nullptr);
    if(n<=0)return nullptr;
    auto* out=static_cast<char*>(allocate(sizeof(prefix)+static_cast<std::size_t>(n)+1,5,c));if(!out)return nullptr;
    std::memcpy(out,prefix,sizeof(prefix)-1);
    if(!::WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,reinterpret_cast<LPCWSTR>(text),-1,out+sizeof(prefix)-1,n,nullptr,nullptr)){release(out,5,c);return nullptr;}
    out[sizeof(prefix)+n-2]='"';out[sizeof(prefix)+n-1]=0;return out;
}
void Win32UrlBinding::report(std::uint32_t code,const char* text,void* c){xcurl_research_gdk_error_report(code,text,&self(c).gdk_);}
void Win32UrlBinding::prepare_diag(void* e,std::uint32_t site,std::uintptr_t,std::uint32_t code,void* c){
    auto& h=*static_cast<WinHttpHost*>(c);if(h.diagnostic)h.diagnostic(e,site,code,h.diagnostic_context);
}
std::uint32_t Win32UrlBinding::prepare(const char* text,void* e,void* out){
    if(host_.urls!=this||!out)return 0x8000ffffU;
    return xcurl_research_prepare_url(text,e,out,&preparation_);
}
std::uint32_t Win32UrlBinding::prepare_url(void* text,void* e,void* out,void* c){return self(c).prepare(static_cast<const char*>(text),e,out);}
std::uint32_t Win32UrlBinding::default_url(const void* object,std::uint32_t part,void** out,void* c){
    return get(const_cast<void*>(object),static_cast<int>(part),reinterpret_cast<char**>(out),1,c);
}
std::uint32_t Win32UrlBinding::parse_port(const void* text,void*){return static_cast<std::uint32_t>(std::strtoul(static_cast<const char*>(text),nullptr,10));}
std::uint32_t Win32UrlBinding::copy_url(const void* text,void* out,void* c){
    return xcurl_research_copy_utf16(static_cast<const char16_t*>(text),static_cast<void**>(out),allocate,c);
}
}
#endif
