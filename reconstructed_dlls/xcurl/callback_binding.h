#pragma once
#include "task_queue_binding.h"
#include "request_start.h"
#include "http_status.h"
#include "upload_engine.h"
#include <atomic>
namespace t6_xcurl {
// Exactly one process-wide callback binding, mirroring the original global
// engine. Activate before issuing work; drain all GDK and WinHTTP callbacks
// before deactivation/destruction. No callback outlives its service objects.
class CallbackBinding {
public:
    CallbackBinding(TaskQueueBinding&,const StartServices&,const StatusServices&,
                    const HeaderServices&,const BodyServices&,const UploadServices&);
    ~CallbackBinding(){deactivate();}
    CallbackBinding(const CallbackBinding&)=delete;
    CallbackBinding& operator=(const CallbackBinding&)=delete;
    bool activate();
    void deactivate();
    const HeaderServices& headers() const {return headers_;}
    const BodyServices& body() const {return body_;}
    static void* start_token();
    static void start_entry(void* async);
    static void status_entry(std::uintptr_t request,std::uint32_t status,const void* info,std::uint32_t length);
private:
    TaskQueueBinding& tasks_;
    const StartServices& start_;
    const StatusServices& status_;
    HeaderServices headers_;
    BodyServices body_;
    const UploadServices& upload_;
    static std::atomic<CallbackBinding*> active_;
    static std::uint32_t submit_header(void*,void*,std::uint32_t,void*);
    static std::uint32_t submit_body(void*,void*,std::uint32_t,void*);
};
}
