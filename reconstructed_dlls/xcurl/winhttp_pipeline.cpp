#include "winhttp_pipeline.h"
#ifdef _WIN32
#include "allocation.h"
#include "win32_certificate.h"
#include "upload_trailers.h"
#include <array>
#include <cstring>
namespace t6_xcurl {
namespace {
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
std::uint32_t acquire(void* cache,void* easy,std::uint32_t tls,std::uint32_t scheme,void** out,void* c){
    return xcurl_bound_session_acquire(cache,easy,tls,scheme,out,static_cast<const SessionServices*>(c));
}
}
WinHttpPipeline::WinHttpPipeline(GlobalLifecycle& g,WinHttpHost& h,RequestServices& r,TaskQueueBinding& t,
                                 const GdkQueryServices& d,const UrlServices& url,const CertificateServices* cert,Trailers trailer)
    :global_(g),host_(h),request_(r),tasks_(t),url_(url),certificate_(cert),trailers_(trailer),
     sessions_(winhttp_session_services(&h)),setup_(winhttp_setup_services(&h)),
     query_(winhttp_header_query_services(&h)),add_(winhttp_add_header_services(&r)),
     buffer_{&r,reallocate,diag64},upload_(winhttp_upload_services(&r,append_trailers,diag64)),
     delivery_{&r,write_callback,diag_mixed},receive_{&query_,&r,scheme,diag_mixed},
     headers_{&r,nullptr,diag64},body_{&r,nullptr,deliver},
     start_{&r,&buffer_,&d,crack,open,register_request,options,add_headers,select_certificate,diag},
     status_{&r,&d,redirect,queue_upload,receive_headers,queue_headers,publish_data,schedule_read,query_available,diag_status},
     callbacks_(t,start_,status_,headers_,body_,upload_){}
bool WinHttpPipeline::activate(){
    if(active_)return true;
    if(host_.pipeline||!callbacks_.activate())return false;
    host_.pipeline=this;host_.tasks=&tasks_;host_.status_callback=win32_reconstructed_status_callback();
    bind_winhttp_request_services(request_,&host_);active_=true;return true;
}
void WinHttpPipeline::deactivate(){
    // Explicit owner contract: all queues and WinHTTP handles drained first.
    if(active_){callbacks_.deactivate();host_.pipeline=nullptr;host_.tasks=nullptr;host_.status_callback=nullptr;active_=false;}
}
WinHttpPipeline& WinHttpPipeline::self(void* c){return *static_cast<WinHttpHost*>(c)->pipeline;}
std::uint32_t WinHttpPipeline::crack(const char16_t* text,void* easy,void* parts,void* c){auto& b=self(c);return xcurl_research_crack_url(text,easy,parts,&b.url_);}
std::uint32_t WinHttpPipeline::scheme(const char16_t* text,void* easy,std::uint32_t* out,void* c){
    alignas(8) std::array<std::uint8_t,0x68> parts{};auto status=crack(text,easy,parts.data(),c);
    if(!(status&0x80000000U))*out=get<std::uint32_t>(parts.data(),0x14);return status;
}
std::uint32_t WinHttpPipeline::open(void* req,void* easy,void* parts,void* c){
    auto& b=self(c);return xcurl_research_http_open_with_session(req,easy,parts,b.global_.engine,acquire,&b.sessions_,&b.setup_);
}
std::uint32_t WinHttpPipeline::register_request(void* req,void* c){return xcurl_research_http_register(req,&self(c).request_);}
std::uint32_t WinHttpPipeline::options(void* req,const void* opt,std::uint32_t scheme,void* c){return xcurl_research_http_options(req,opt,scheme,&self(c).setup_);}
std::uint32_t WinHttpPipeline::add_headers(void* req,void* list,void* c){return xcurl_research_add_request_headers(req,list,&self(c).add_);}
std::uint32_t WinHttpPipeline::select_certificate(void* req,void* selector,void* c){
    auto& b=self(c);if(!selector)return 0;
    if(!b.certificate_)return win32_select_client_certificate(req,static_cast<const char16_t*>(selector),b.host_);
    return xcurl_research_select_certificate(req,static_cast<const char16_t*>(selector),b.certificate_);
}
std::uint32_t WinHttpPipeline::redirect(const void* text,void* slot,void* c){
    auto& b=self(c);return xcurl_research_copy_utf16(static_cast<const char16_t*>(text),static_cast<void**>(slot),b.request_.allocate,b.request_.context);
}
std::uint32_t WinHttpPipeline::queue_upload(void* e,void* c){return xcurl_research_queue_upload(e,&self(c).callbacks_.headers());}
std::uint32_t WinHttpPipeline::receive_headers(void* r,void* e,std::uint32_t* code,void* c){return xcurl_research_receive_headers(r,e,code,&self(c).receive_);}
std::uint32_t WinHttpPipeline::queue_headers(void* e,void* c){return xcurl_research_queue_headers(e,&self(c).callbacks_.headers());}
std::uint32_t WinHttpPipeline::publish_data(void* e,const void* data,std::uint32_t n,void* c){
    if(!data||n>0x2000||(data!=get<void*>(e,0x340)&&data!=get<void*>(e,0x348)))return 0x80070057U;
    return xcurl_research_publish_body(e,const_cast<void*>(data),n,&self(c).callbacks_.body());
}
std::uint32_t WinHttpPipeline::schedule_read(void* r,void* e,void* c){return xcurl_research_schedule_read(r,e,&self(c).callbacks_.body());}
std::uint32_t WinHttpPipeline::query_available(void* handle,void*){return ::WinHttpQueryDataAvailable(handle,nullptr);}
std::uint32_t WinHttpPipeline::deliver(void* e,const void* data,std::uint32_t n,void* c){return xcurl_research_deliver_body(e,data,n,&self(c).delivery_);}
std::uint64_t WinHttpPipeline::write_callback(void* cb,const void* data,std::uint64_t n,void* user,void*){
    return xcurl_research_invoke_write(reinterpret_cast<TransferCallback>(cb),data,n,user);
}
std::uint32_t WinHttpPipeline::append_trailers(void* r,std::uint64_t* n,void* c){auto& b=self(c);return b.trailers_?b.trailers_(r,n,c):bound_append_upload_trailers(r,n,&b.request_);}
void* WinHttpPipeline::reallocate(void* p,std::size_t n,std::uint32_t t,void*){return xcurl_research_heap_reallocate(p,n,t);}
void WinHttpPipeline::diag(void* e,std::uint32_t site,std::uint32_t value,void* c){auto& r=self(c).request_;r.diagnostic(e,site,value,r.context);}
void WinHttpPipeline::diag64(void* e,std::uint32_t site,std::uint64_t a,std::uint64_t b,void* c){winhttp_report_diagnostic(e,site,a,b,0,static_cast<std::uint32_t>(a),static_cast<WinHttpHost*>(c));}
void WinHttpPipeline::diag_mixed(void* e,std::uint32_t site,std::uint64_t a,std::uint32_t b,void* c){winhttp_report_diagnostic(e,site,a,b,b,b?b:static_cast<std::uint32_t>(a),static_cast<WinHttpHost*>(c));}
void WinHttpPipeline::diag_status(void* e,std::uint32_t site,std::uint64_t a,std::uint64_t b,std::uint32_t h,void* c){winhttp_report_diagnostic(e,site,a,b,h,h?h:static_cast<std::uint32_t>(a),static_cast<WinHttpHost*>(c));}
}
#endif
