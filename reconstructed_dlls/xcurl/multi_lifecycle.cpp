// Store XCurl SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Original instruction ranges 0x184A8..0x1874F, 0x18110..0x184A6,
// and 0xCEFC..0xCF8F (disabled ETW). Engine is the original 0x28C10
// global pointer, passed explicitly in the source-link reconstruction.
#include "multi_lifecycle.h"
#include <bit>
#include <cstring>
#include <initializer_list>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
P ptr(const void* p,unsigned o){return get<P>(p,o);}
bool failed(std::uint32_t status){return (status&0x80000000U)!=0;}
std::uint32_t hr(std::uint32_t e){return std::bit_cast<std::int32_t>(e)>0?(e&0xffffU)|0x80070000U:e;}
void unlink(P n){put(ptr(n,0),8,ptr(n,8));put(ptr(n,8),0,ptr(n,0));put(n,0,n);put(n,8,n);}
void append(P head,P node){put(node,0,head);put(node,8,ptr(head,8));put(ptr(head,8),0,node);put(head,8,node);}
void detach(P head,const RequestServices* r){
    for(auto* e=ptr(head,0);e!=head;e=ptr(head,0)){
        unlink(e);r->enter(e+0x10,r->context);put(e,0x358,0U);put<P>(e,0x310,nullptr);r->leave(e+0x10,r->context);
    }
}
void close_at(P m,unsigned o,const MultiLifecycleServices* s){
    if(auto* h=ptr(m,o)){s->close_handle(h,s->context);put<P>(m,o,nullptr);}
}
}
extern "C" void xcurl_research_easy_stop(void* easy,const RequestServices* r){ // CEFC
    xcurl_research_request_stop(static_cast<P>(easy)+0x230,r);
}
extern "C" std::uint32_t xcurl_research_multi_start_engine(void* multi,void* engine,const MultiLifecycleServices* s){
    auto* m=static_cast<P>(multi);auto* g=static_cast<P>(engine);auto* q=s->scheduler;auto c=s->context;
    std::uint32_t status=0;
    auto* mutex=s->create_mutex(c);put(m,0xb8,mutex);if(!mutex)status=hr(s->last_error(c));
    for(unsigned i=0;i<2&&!failed(status);++i){
        auto* event=s->create_event(0,0,c);put(m,0xa0+8*i,event);if(!event)status=hr(s->last_error(c));
    }
    if(!failed(status)){
        auto* event=s->create_event(1,1,c);put(m,0xb0,event);if(!event)status=hr(s->last_error(c));
    }
    if(failed(status)){xcurl_research_multi_destroy_engine(m,g,s);return 4;}
    q->enter(g,q->context);append(g+0x30,m);
    if(g[0x48])xcurl_research_abort_all(m,q);
    q->leave(g,q->context);return 0;
}
extern "C" std::uint32_t xcurl_research_multi_destroy_engine(void* multi,void* engine,const MultiLifecycleServices* s){
    auto* m=static_cast<P>(multi);auto* g=static_cast<P>(engine);auto* q=s->scheduler;auto* r=s->request;auto c=s->context;auto qc=q->context;
    q->wait_one(ptr(m,0xb8),UINT32_MAX,qc);
    q->enter(m+0x10,qc);detach(m+0x40,r);
    auto* head=m+0x50;
    for(auto* e=ptr(head,0);e&&e!=head;e=ptr(e,0)){
        q->cancel_easy(e,0x80004004U,0x2a,qc);q->after_cancel(e,qc);
    }
    q->leave(m+0x10,qc);
    if(auto* idle=ptr(m,0xb0)){
        do{
            xcurl_research_advance_active(m,q);
            if(!q->wait_one(idle,0,qc))break;
            q->wait_one(ptr(m,0xa0),UINT32_MAX,qc);
        }while(true);
    }
    q->enter(m+0x10,qc);detach(m+0x60,r);q->leave(m+0x10,qc);
    q->wait_one(ptr(g,0x28),UINT32_MAX,qc);q->enter(g,qc);unlink(m);q->leave(g,qc);s->release_mutex(ptr(g,0x28),c);
    close_at(m,0xb0,s);
    q->enter(m+0x10,qc);head=m+0x78;
    while(auto* node=ptr(head,0)){if(node==head)break;unlink(node);r->release(node,12,r->context);}
    put<P>(m,0x70,nullptr);q->leave(m+0x10,qc);
    s->release_mutex(ptr(m,0xb8),c);
    for(auto o:{0xa0U,0xa8U,0xb8U})close_at(m,o,s);
    return 0;
}
