// Original XCurl 0x18F68, 0x19344, 0x1925C, 0x19514, 0x19614.
// Disabled ETW; OS synchronization and request start/progress are explicit boundaries.
#include "engine_scheduler.h"
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(P p,unsigned o){T v;std::memcpy(&v,p+o,sizeof(v));return v;}
template<class T>void put(P p,unsigned o,T v){std::memcpy(p+o,&v,sizeof(v));}
P ptr(P p,unsigned o){return get<P>(p,o);}
void increment(P p,unsigned o,std::uint32_t n){put(p,o,get<std::uint32_t>(p,o)+n);}
void move_back(P h,P n){put(ptr(n,0),8,ptr(n,8));put(ptr(n,8),0,ptr(n,0));put(n,0,h);put(n,8,ptr(h,8));put(ptr(h,8),0,n);put(h,8,n);}
}
namespace {
void advance_pending(void* multi,void* engine,const SchedulerServices* s,bool publish_before_start){
    auto* m=static_cast<P>(multi);auto* g=static_cast<P>(engine);auto c=s->context;
    s->enter(g,c);auto enabled=g[0x49];s->leave(g,c);
    auto* head=m+0x40;std::uint32_t count=0;
    s->enter(m+0x10,c);if(!m[0xc0]&&enabled)for(auto* e=ptr(head,0);e&&e!=head;e=ptr(e,0))++count;s->leave(m+0x10,c);
    if(!count)return;
    s->enter(g,c);auto current=get<std::uint32_t>(g,0x40),limit=get<std::uint32_t>(g,0x44);
    if(current>=limit)count=0;
    else if(current+count<=limit)put(g,0x40,current+count);
    else{count=limit-current;put(g,0x40,limit);}
    s->leave(g,c);std::uint32_t started=0;
    s->enter(m+0x10,c);if(!m[0xc0]){
        while(started<count){
            auto* e=ptr(head,0);if(!e||e==head)break;
            if(publish_before_start){
                // A failed GDK submit can complete inline. Make the easy
                // visible to A6B0 and count it before invoking CF90.
                move_back(m+0x50,e);
                if(!get<std::uint32_t>(m,0x3c))s->reset_event(ptr(m,0xb0),c);
                increment(m,0x3c,1);++started;s->start_easy(e,c);
            }else{s->start_easy(e,c);++started;move_back(m+0x50,e);}
        }
        if(!publish_before_start){
            if(started)s->reset_event(ptr(m,0xb0),c);
            increment(m,0x3c,started);
        }
    }s->leave(m+0x10,c);
    if(count!=started){s->enter(g,c);increment(g,0x40,started-count);s->leave(g,c);}
}
}
extern "C" void xcurl_research_advance_pending(void* m,void* g,const SchedulerServices* s){advance_pending(m,g,s,false);}
extern "C" void xcurl_bound_advance_pending(void* m,void* g,const SchedulerServices* s){advance_pending(m,g,s,true);}
extern "C" void xcurl_research_advance_active(void* multi,const SchedulerServices* s){
    auto* m=static_cast<P>(multi);auto c=s->context;increment(m,0xc4,1);auto* head=m+0x50;
    for(;;){P selected=nullptr;s->enter(m+0x10,c);
        for(auto* e=ptr(head,0);e&&e!=head;e=ptr(e,0)){
            auto epoch=get<std::uint32_t>(m,0xc4);s->enter(e+0x10,c);auto previous=get<std::uint32_t>(e,0x358);put(e,0x358,epoch);s->leave(e+0x10,c);
            if(previous!=epoch){selected=e;break;}
        }
        s->leave(m+0x10,c);if(!selected)return;s->progress_easy(selected,c);
    }
}
extern "C" void xcurl_research_abort_all(void* multi,const SchedulerServices* s){
    auto* m=static_cast<P>(multi);auto c=s->context;s->enter(m+0x10,c);m[0xc0]=1;auto* head=m+0x50;
    for(auto* e=ptr(head,0);e&&e!=head;e=ptr(e,0)){s->cancel_easy(e,0x80070800U,0x59,c);s->after_cancel(e,c);}s->leave(m+0x10,c);
}
extern "C" std::uint32_t xcurl_research_multi_wait(void* multi,std::uint32_t timeout,std::uint32_t* count,const SchedulerServices* s){
    auto* m=static_cast<P>(multi);auto c=s->context;s->enter(m+0x10,c);increment(m,0x98,1);s->leave(m+0x10,c);
    auto result=s->wait_one(ptr(m,0xa0),timeout,c);if(count)*count=result!=0x102;
    s->enter(m+0x10,c);increment(m,0x98,UINT32_MAX);s->leave(m+0x10,c);return 0;
}
extern "C" std::uint32_t xcurl_research_multi_wakeup(void* multi,const SchedulerServices* s){
    auto* m=static_cast<P>(multi);auto c=s->context;s->enter(m+0x10,c);auto result=get<std::uint32_t>(m,0x98)&&!get<std::uint32_t>(m,0x9c)?9U:0U;s->leave(m+0x10,c);
    if(!result)s->set_event(ptr(m,0xa8),c);return result;
}
