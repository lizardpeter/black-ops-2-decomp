#pragma once
#include "global_lifecycle.h"
#include "engine_scheduler.h"
#include "request_prepare.h"
namespace t6_xcurl {
// Non-owning source-link adapter. All referenced services must outlive it.
// Create before engine init if its scheduler is used by EngineServices.
// The owner serializes engine teardown against all outstanding multi work.
struct MultiHostServices {
    WaitOne wait_one;
    WaitMany wait_many;
    EventAction release_mutex,reset_event;
    // Original 0xCEFC remains an explicit, unported cancellation boundary.
    EngineStep after_cancel;
    void* context;
};
class TransferBinding {
public:
    TransferBinding(GlobalLifecycle&,const PrepareServices&,const MultiHostServices&);
    TransferBinding(const TransferBinding&)=delete;
    TransferBinding& operator=(const TransferBinding&)=delete;
    const SchedulerServices& scheduler() const {return scheduler_;}
    std::uint32_t add(void*,void*);
    std::uint32_t remove(void*,void*);
    std::uint32_t perform(void*,std::uint32_t*);
    std::uint32_t poll(void*,std::uint32_t,std::uint32_t*);
    std::uint32_t wait(void*,std::uint32_t,std::uint32_t*);
    std::uint32_t wakeup(void*);
    void* info_read(void*,std::uint32_t*);
private:
    GlobalLifecycle& global_;
    const PrepareServices& prepare_;
    const MultiHostServices& host_;
    SchedulerServices scheduler_;
    MultiServices multi_;
    static TransferBinding& self(void*);
    static void enter(void*,void*);
    static void leave(void*,void*);
    static void start(void*,void*);
    static void progress(void*,void*);
    static void cancel(void*,std::uint32_t,std::uint32_t,void*);
    static void after_cancel(void*,void*);
    static void reset(void*,void*);
    static void event(void*,void*);
    static std::uint32_t wait_one(void*,std::uint32_t,void*);
    static std::uint32_t wait_many(std::uint32_t,const void*,std::uint32_t,void*);
    static void release_mutex(void*,void*);
    static void pending(void*,void*);
    static void active(void*,void*);
    static std::uint8_t done(void*,void*);
    static void adjust(std::uint32_t,void*);
    static void release(void*,std::uint32_t,void*);
};
}
