// Original Store XCurl 0xA544, 0x113B8, 0xFFA4, 0xE544,
// disabled ETW, pinned SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Evidence: instruction runs 45545280844 and 45545348071.
#include "easy_lifecycle.h"
#include "handle_lifecycle.h"
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
}
extern "C" std::uint8_t xcurl_research_request_is_active(void* request,const RequestServices* r){
    r->enter(request,r->context);const auto state=get<std::uint32_t>(request,0x28);
    const auto active=state-1U<=4U;r->leave(request,r->context);return active;
}
extern "C" void xcurl_research_easy_destroy_engine(void* easy,const EasyLifecycleServices* s){
    auto* e=static_cast<P>(easy);auto* r=s->request;auto c=r->context;
    if(xcurl_research_request_is_active(e+0x230,r))r->diagnostic(e,0x206e0,0,c);
    xcurl_research_easy_reset_transfer(e,r);
    if(auto* queue=get<void*>(e,0x350)){
        auto status=xcurl_research_gdk_task_queue_terminate(queue,0,nullptr,nullptr,s->gdk);
        if(!(status&0x80000000U)){
            for(;;){
                auto work=xcurl_research_gdk_task_queue_dispatch(queue,0,0,s->gdk);
                auto completion=xcurl_research_gdk_task_queue_dispatch(queue,1,0,s->gdk);
                if(!(work|completion))break;
            }
        }
        xcurl_research_gdk_task_queue_close(queue,s->gdk);put<void*>(e,0x350,nullptr);
    }
    if(auto* event=get<void*>(e,0x360)){s->close_handle(event,s->context);put<void*>(e,0x360,nullptr);}
    s->delete_cs(e+0x230,s->context);s->delete_cs(e+0x188,s->context);
    xcurl_research_options_destroy(e+0x78,r->release,c);
    s->delete_cs(e+0x50,s->context);s->delete_cs(e+0x10,s->context);
}
