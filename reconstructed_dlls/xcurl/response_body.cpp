// Original XCurl RVAs AFFC, BCAC, 11FA8, B800, BDB0, C4A0. ETW disabled.
#include "response_body.h"
#include <algorithm>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<P>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
P ptr(void* p,unsigned o){return get<P>(p,o);}
bool failed(std::uint32_t x){return (x&0x80000000U)!=0;}
}
extern "C" std::uint32_t xcurl_research_reserve_body_buffer(void* easy,void** out,const BodyServices* b){
    auto* e=static_cast<P>(easy);auto* s=b->request;auto c=s->context;std::uint32_t status=0;
    s->enter(e+0x10,c);auto mask=get<std::uint32_t>(e,0x338);
    if((mask&3)!=3){unsigned i=(mask&1)?1:0;put(e,0x338,mask|(1U<<i));auto offset=0x340+8*i;auto* p=ptr(e,offset);if(!p){p=static_cast<P>(s->allocate(0x2000,14,c));put(e,offset,p);}if(p)*out=p;else status=0x8007000eU;}
    s->leave(e+0x10,c);return status;
}
extern "C" std::uint32_t xcurl_research_queue_read(void* easy,const BodyServices* b){
    auto* e=static_cast<P>(easy);auto* s=b->request;auto c=s->context;
    s->enter(e+0x10,c);auto status=b->submit(ptr(e,0x350),e,0,c);if(!failed(status))xcurl_research_easy_notify(e,s);s->leave(e+0x10,c);return status;
}
extern "C" std::uint32_t xcurl_research_schedule_read(void* request,void* easy,const BodyServices* b){
    auto* r=static_cast<P>(request);auto* s=b->request;auto c=s->context;
    s->enter(r,c);bool active=get<std::uint32_t>(r,0x28)==2;auto old=get<std::uint32_t>(r,0xd8);put(r,0xd8,old+1);s->leave(r,c);
    std::uint32_t status=0;if(active){void* buffer=nullptr;if(old<2)status=xcurl_research_reserve_body_buffer(easy,&buffer,b);if(!failed(status)){if(buffer)status=xcurl_research_http_read(r,buffer,r[0xb8]?0x2000U:std::min(get<std::uint32_t>(r,0xdc),0x2000U),s);else status=xcurl_research_queue_read(easy,b);}}
    s->enter(r,c);put(r,0xd8,get<std::uint32_t>(r,0xd8)-1);s->leave(r,c);return status;
}
extern "C" void xcurl_research_deferred_read(void* easy,const BodyServices* b){
    auto* e=static_cast<P>(easy);auto* s=b->request;auto status=xcurl_research_schedule_read(e+0x230,e,b);if(failed(status)){s->diagnostic(e,0x208e8,status,s->context);xcurl_research_easy_cancel(e,status,56,s);xcurl_research_request_stop(e+0x230,s);}
}
extern "C" std::uint32_t xcurl_research_publish_body(void* easy,void* data,std::uint32_t length,const BodyServices* b){
    auto* e=static_cast<P>(easy);auto* s=b->request;auto c=s->context;
    s->enter(e+0x10,c);
    // Precondition: data is one of the two owned buffers. Original invalid-pointer path writes outside the object.
    unsigned i=data==ptr(e,0x340)?0:1;
    auto* task=e+0x318+i*16;put(task,0,e);put(task,8,i);put(task,12,length);
    auto status=b->submit(ptr(e,0x350),task,1,c);if(!failed(status))xcurl_research_easy_notify(e,s);s->leave(e+0x10,c);return status;
}
extern "C" void xcurl_research_body_worker(void* task,const BodyServices* b){
    auto* e=ptr(task,0);auto* s=b->request;auto c=s->context;
    s->enter(e+0x188,c);auto status=get<std::uint32_t>(e,0x228);s->leave(e+0x188,c);
    if(!failed(status)){status=b->deliver(e,ptr(e,0x340+8*get<std::uint32_t>(task,8)),get<std::uint32_t>(task,12),c);if(failed(status)){xcurl_research_easy_cancel(e,status,23,s);xcurl_research_request_stop(e+0x230,s);}}
    s->enter(e+0x10,c);put(e,0x338,get<std::uint32_t>(e,0x338)&~(1U<<(get<std::uint32_t>(task,8)&31)));s->leave(e+0x10,c);
}
// Original 0xAB3C. Callback invocation at 0xB9B4 is a separate boundary.
extern "C" std::uint32_t xcurl_research_deliver_body(void* easy,const void* data,std::uint32_t length,const DeliveryServices* d){
    auto* e=static_cast<P>(easy);auto* s=d->request;auto c=s->context;
    s->enter(e+0x50,c);auto limit=get<std::uint32_t>(e,0xac);auto* callback=ptr(e,0x110);auto* userdata=ptr(e,0x108);bool debug=e[0x176]!=0;s->leave(e+0x50,c);
    auto* bytes=static_cast<const std::uint8_t*>(data);
    // Nonzero chunk limit is required for progress, unless callback returns an error.
    while(length){auto chunk=std::min(limit,length);if(debug)s->debug_data(e,3,bytes,chunk,c);
        auto count=d->write(callback,bytes,chunk,userdata,c);put(e,0x33c,get<std::uint32_t>(e,0x33c)+static_cast<std::uint32_t>(count));
        if(count==0x10000001ULL){d->diagnose(e,0x20958,0,0,c);return 0x800704c7U;}
        if(count!=chunk){d->diagnose(e,0x20988,count,chunk,c);return 0x800700eaU;}
        bytes+=chunk;length-=chunk;
    }
    return 0;
}
