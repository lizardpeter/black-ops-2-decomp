#pragma once
#ifdef _WIN32
#include "winhttp_services.h"
#include "url_crack.h"
#include "certificate.h"
#include "response_headers.h"
namespace t6_xcurl {
// Composes the recovered native WinHTTP request-start and status chains.
// URL reporting and optional client-certificate/trailer services remain
// explicit until their complete OS ABI has been verified.
class WinHttpPipeline {
public:
    using Trailers=std::uint32_t(*)(void*,std::uint64_t*,void*);
    WinHttpPipeline(GlobalLifecycle&,WinHttpHost&,RequestServices&,TaskQueueBinding&,
                    const GdkQueryServices&,const UrlServices&,const CertificateServices*,Trailers=nullptr);
    ~WinHttpPipeline(){deactivate();}
    WinHttpPipeline(const WinHttpPipeline&)=delete;
    WinHttpPipeline& operator=(const WinHttpPipeline&)=delete;
    bool activate();
    void deactivate();
    const SessionServices& sessions() const{return sessions_;}
    const StartServices& start() const{return start_;}
    const StatusServices& status() const{return status_;}
    const UrlServices& url() const{return url_;}
private:
    GlobalLifecycle& global_;
    WinHttpHost& host_;
    RequestServices& request_;
    TaskQueueBinding& tasks_;
    const UrlServices& url_;
    const CertificateServices* certificate_;
    Trailers trailers_;
    SessionServices sessions_;
    HttpSetupServices setup_;
    HeaderQueryServices query_;
    AddHeaderServices add_;
    BufferServices buffer_;
    UploadServices upload_;
    DeliveryServices delivery_;
    ReceiveHeaderServices receive_;
    HeaderServices headers_;
    BodyServices body_;
    StartServices start_;
    StatusServices status_;
    CallbackBinding callbacks_;
    bool active_=false;
    static WinHttpPipeline& self(void*);
    static std::uint32_t crack(const char16_t*,void*,void*,void*);
    static std::uint32_t scheme(const char16_t*,void*,std::uint32_t*,void*);
    static std::uint32_t open(void*,void*,void*,void*);
    static std::uint32_t register_request(void*,void*);
    static std::uint32_t options(void*,const void*,std::uint32_t,void*);
    static std::uint32_t add_headers(void*,void*,void*);
    static std::uint32_t select_certificate(void*,void*,void*);
    static std::uint32_t redirect(const void*,void*,void*);
    static std::uint32_t queue_upload(void*,void*);
    static std::uint32_t receive_headers(void*,void*,std::uint32_t*,void*);
    static std::uint32_t queue_headers(void*,void*);
    static std::uint32_t publish_data(void*,const void*,std::uint32_t,void*);
    static std::uint32_t schedule_read(void*,void*,void*);
    static std::uint32_t query_available(void*,void*);
    static std::uint32_t deliver(void*,const void*,std::uint32_t,void*);
    static std::uint64_t write_callback(void*,const void*,std::uint64_t,void*,void*);
    static std::uint32_t append_trailers(void*,std::uint64_t*,void*);
    static void* reallocate(void*,std::size_t,std::uint32_t,void*);
    static void diag(void*,std::uint32_t,std::uint32_t,void*);
    static void diag64(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*);
    static void diag_mixed(void*,std::uint32_t,std::uint64_t,std::uint32_t,void*);
    static void diag_status(void*,std::uint32_t,std::uint64_t,std::uint64_t,std::uint32_t,void*);
};
}
#endif
