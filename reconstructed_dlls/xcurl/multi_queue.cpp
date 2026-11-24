// XCurl Store: original disabled-ETW multi scheduling and intrusive lists.
// 0x19744/0x17F28, 0x19D20/0x18B44, 0x187E8, 0x18920,
// 0x198E4/0x18A28, and list helpers 0x1803C/0x18CFC.
// Windows wait primitives and unported active transfer work are boundaries.
#include "multi_queue.h"
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T> T get(P p,unsigned o){T v;std::memcpy(&v,p+o,sizeof(v));return v;}
template<class T> void set(P p,unsigned o,T v){std::memcpy(p+o,&v,sizeof(v));}
P ptr(P p,unsigned o){return get<P>(p,o);}
void inc(P p,unsigned o,std::uint32_t n){set(p,o,get<std::uint32_t>(p,o)+n);}
void unlink(P n){set(ptr(n,0),8,ptr(n,8));set(ptr(n,8),0,ptr(n,0));set(n,0,n);set(n,8,n);}
void insert(P head,P n){set(n,0,ptr(head,0));set(n,8,head);set(ptr(head,0),8,n);set(head,0,n);}
void assign_multi(P e,P m,const MultiServices& s){s.enter(e+0x10,s.context);set(e,0x358,0U);set(e,0x310,m);s.leave(e+0x10,s.context);}
bool remove_from(P m,P head,P e,const MultiServices& s){
    s.enter(m+0x10,s.context);
    auto* n=ptr(head,0);while(n!=head&&n!=e)n=ptr(n,0);
    const bool found=n==e&&n&&n!=head;
    if(found)unlink(n);
    s.leave(m+0x10,s.context);return found;
}
}
extern "C" std::uint32_t xcurl_research_multi_add(void* multi,void* easy,const MultiServices* s){
    auto* m=static_cast<P>(multi);auto* e=static_cast<P>(easy);
    if(!m)return 1;if(!e)return 2;
    s->enter(e+0x10,s->context);const auto existing=ptr(e,0x310);s->leave(e+0x10,s->context);
    if(existing)return 7;
    s->enter(m+0x10,s->context);inc(m,0x38,1);s->leave(m+0x10,s->context);
    assign_multi(e,m,*s);
    s->enter(m+0x10,s->context);insert(m+0x40,e);s->leave(m+0x10,s->context);
    s->set_event(ptr(m,0xa0),s->context);return 0;
}
extern "C" std::uint32_t xcurl_research_multi_remove(void* multi,void* easy,const MultiServices* s){
    auto* m=static_cast<P>(multi);auto* e=static_cast<P>(easy);
    if(!m)return 1;
    s->wait_one(ptr(m,0xb8),UINT32_MAX,s->context);
    if(!remove_from(m,m+0x60,e,*s)&&!remove_from(m,m+0x40,e,*s)){
        if(!remove_from(m,m+0x50,e,*s)){s->release_mutex(ptr(m,0xb8),s->context);return 2;}
        s->cancel_easy(e,0x80004004U,0x2a,s->context);
        s->after_cancel(e,s->context);
        if(!s->easy_done(e,s->context)){
            do {s->progress_easy(e,s->context);if(s->easy_done(e,s->context))break;
                s->wait_one(ptr(e,0x360),UINT32_MAX,s->context);}while(true);
        }
        s->enter(m+0x10,s->context);inc(m,0x3c,UINT32_MAX);
        if(!get<std::uint32_t>(m,0x3c))s->set_event(ptr(m,0xb0),s->context);
        s->leave(m+0x10,s->context);s->adjust_global(1,s->context);
    }
    assign_multi(e,nullptr,*s);
    s->enter(m+0x10,s->context);inc(m,0x38,UINT32_MAX);s->leave(m+0x10,s->context);
    s->release_mutex(ptr(m,0xb8),s->context);return 0;
}
extern "C" std::uint32_t xcurl_research_multi_perform_inner(void* multi,std::uint32_t* running,const MultiServices* s){
    auto* m=static_cast<P>(multi);if(!m)return 1;
    s->wait_one(ptr(m,0xb8),UINT32_MAX,s->context);s->advance_pending(m,s->context);s->advance_active(m,s->context);
    s->enter(m+0x10,s->context);
    if(running){std::uint32_t n=get<std::uint32_t>(m,0x3c);auto* head=m+0x40;
        for(auto* p=ptr(head,0);p&&p!=head;p=ptr(p,0))++n;
        *running=n;
    }
    s->leave(m+0x10,s->context);s->release_mutex(ptr(m,0xb8),s->context);return 0;
}
extern "C" std::uint32_t xcurl_research_multi_poll_inner(void* multi,std::uint32_t timeout,std::uint32_t* count,const MultiServices* s){
    auto* m=static_cast<P>(multi);if(!m)return 1;
    s->enter(m+0x10,s->context);inc(m,0x9c,1);s->leave(m+0x10,s->context);
    auto wait=s->wait_many(2,m+0xa0,timeout,s->context);
    if(count)*count=wait!=0x102;
    s->enter(m+0x10,s->context);inc(m,0x9c,UINT32_MAX);s->leave(m+0x10,s->context);return 0;
}
extern "C" void* xcurl_research_multi_info_read_inner(void* multi,std::uint32_t* remaining,const MultiServices* s){
    auto* m=static_cast<P>(multi);if(!m)return nullptr;
    s->enter(m+0x10,s->context);
    if(auto* old=ptr(m,0x70)){unlink(old);s->release(old,12,s->context);}
    auto* head=m+0x78;auto* next=ptr(head,0);if(next==head)next=nullptr;
    set(m,0x70,next);
    if(next)inc(m,0x88,UINT32_MAX);
    *remaining=get<std::uint32_t>(m,0x88);
    s->leave(m+0x10,s->context);return next?next+0x10:nullptr;
}
