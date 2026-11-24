// Original Store 0xB9B4, 0xC360, 0xBBA8, 0xBF6C, 0xB18C/B4C0; ETW disabled.
#include "transfer_callbacks.h"
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<P>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
P ptr(void* p,unsigned o){return get<P>(p,o);}
bool failed(std::uint32_t n){return (n&0x80000000U)!=0;}
std::uint32_t queue(void* easy,unsigned id,const HeaderServices* h){
    auto* e=static_cast<P>(easy);auto* s=h->request;auto c=s->context;
    s->enter(e+0x10,c);auto status=h->submit(ptr(e,0x350),e,id,c);
    if(!failed(status))xcurl_research_easy_notify(e,s);
    s->leave(e+0x10,c);return status;
}
}
extern "C" std::uint64_t xcurl_research_invoke_write(TransferCallback callback,const void* data,std::uint64_t size,void* userdata){
    return callback?callback(const_cast<void*>(data),1,size,userdata):size;
}
extern "C" std::uint64_t xcurl_research_invoke_read(void* easy,void* data,std::uint64_t size,const RequestServices* s){
    auto* e=static_cast<P>(easy);auto c=s->context;s->enter(e+0x50,c);
    auto cb=get<TransferCallback>(e,0x120);auto ud=ptr(e,0x118);s->leave(e+0x50,c);
    return cb?cb(data,1,size,ud):0;
}
extern "C" std::uint32_t xcurl_research_queue_headers(void* easy,const HeaderServices* h){return queue(easy,0,h);}
extern "C" std::uint32_t xcurl_research_queue_upload(void* easy,const HeaderServices* h){return queue(easy,1,h);}
extern "C" void xcurl_research_header_worker(void* easy,const HeaderServices* h){
    auto* e=static_cast<P>(easy);auto* s=h->request;auto c=s->context;
    s->enter(e+0x188,c);
    auto* buffer=ptr(e,0x1d0);auto length=get<std::uint64_t>(e,0x1c8);
    put<P>(e,0x1d0,nullptr);put(e,0x1c8,UINT64_C(0));
    auto cb=get<TransferCallback>(e,0x130);auto ud=ptr(e,0x128);auto status=get<std::uint32_t>(e,0x228);
    if(!cb&&e[0x175]){cb=get<TransferCallback>(e,0x110);ud=ptr(e,0x108);}
    bool debug=e[0x176]!=0;s->leave(e+0x188,c);
    // The original uses an allocation and explicit byte count, not strlen.
    // 0x1ABB0 publishes UTF-8 byte count excluding its terminator; the
    // bounded scan uses buffer+length-1 as its sentinel.
    if(!failed(status)&&buffer&&length){
        auto* end=buffer+length-1;
        for(auto* p=buffer;p<end;){
            auto* nl=static_cast<P>(std::memchr(p,'\n',static_cast<std::size_t>(end-p)));
            if(!nl)nl=end; // matches original bounded search returning end
            auto count=static_cast<std::uint64_t>(nl-p+1);
            if(debug)s->debug_data(e,1,p,count,c);
            auto result=xcurl_research_invoke_write(cb,p,count,ud);
            if(result==0x10000001ULL||result!=count){
                if(result==0x10000001ULL){h->diagnose(e,0x20878,0,0,c);status=0x800704c7U;}
                else{h->diagnose(e,0x208a8,result,count,c);status=0x800700eaU;}
                xcurl_research_easy_cancel(e,status,23,s);xcurl_research_request_stop(e+0x230,s);break;
            }
            p=nl+1;
        }
    }
    if(buffer)s->release(buffer,8,c);
}
