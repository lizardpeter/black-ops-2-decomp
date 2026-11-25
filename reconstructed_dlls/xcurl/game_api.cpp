#include "game_api.h"
#include "task_queue_binding.h"
#include <atomic>
#include <cstdarg>
#include <cstring>
#include <new>
namespace {
std::atomic<t6_xcurl::GameApi*> owner{nullptr};
using WriteFn=std::size_t(*)(char*,std::size_t,std::size_t,void*);
using DebugFn=int(*)(void*,int,char*,std::size_t,void*);
using TrailerFn=int(*)(curl_slist**,void*);
template<class T>std::uint64_t bits(T p){return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(p));}
}
namespace t6_xcurl {
GameApi::GameApi(ApiBinding& a,HandleBinding& h,TransferBinding& t,TaskQueueBinding* reservations):api_(a),handles_(h),transfers_(t),reservations_(reservations){if(reservations_)reservations_->enable_reservations();}
bool GameApi::install(){
    if(!easies_.empty()||!multis_.empty())return false;
    GameApi* empty=nullptr;return owner.compare_exchange_strong(empty,this);
}
bool GameApi::uninstall(){
    if(!easies_.empty()||!multis_.empty()||api_.global.references||(reservations_&&!reservations_->reservations_empty()))return false;
    auto* expected=this;return owner.compare_exchange_strong(expected,nullptr);
}
void GameApi::profile(std::uint32_t id){xcurl_research_record_api(&api_.profile,id,api_.profile_lock,api_.enter,api_.leave,api_.tick,api_.context);}
std::uint32_t GameApi::record(std::uint32_t id,std::uint32_t code){if(code&&api_.error)api_.error(code,id,api_.context);return code;}
std::uint32_t GameApi::global_init(){return bound_global_init(api_);}
void GameApi::global_cleanup(){
    if(api_.global.references==1&&(!easies_.empty()||!multis_.empty())){profile(3);record(3,2);return;}
    bound_global_cleanup(api_);
}
void* GameApi::easy_init(){
    auto* e=handles_.easy_init();if(!e)return nullptr;
    try {easies_.insert(e);}catch(const std::bad_alloc&){handles_.easy_cleanup(e);record(6,27);return nullptr;}
    return e;
}
void GameApi::easy_cleanup(void* e){
    if(!e){handles_.easy_cleanup(nullptr);return;}
    if(!easies_.contains(e)){profile(9);record(9,43);return;}
    if(handles_.try_easy_cleanup(e)){if(reservations_)reservations_->discard(e);easies_.erase(e);}
}
std::uint32_t GameApi::setopt(void* e,int key,std::uint64_t value){
    if(e&&!easies_.contains(e)){profile(7);return record(7,43);}
    return bound_easy_setopt(api_,e,key,value);
}
std::uint32_t GameApi::getinfo(void* e,int key,void* out){
    if(e&&!easies_.contains(e)){profile(10);return record(10,43);}
    if(!out){profile(10);return record(10,43);}
    return bound_easy_getinfo(api_,e,key,out);
}
void* GameApi::multi_init(){
    auto* m=handles_.multi_init();if(!m)return nullptr;
    try {multis_.insert(m);}catch(const std::bad_alloc&){handles_.multi_cleanup(m);record(13,3);return nullptr;}
    return m;
}
std::uint32_t GameApi::multi_cleanup(void* m){
    if(!multis_.contains(m)){profile(20);return record(20,1);}
    const auto status=handles_.multi_cleanup(m);multis_.erase(m);return status;
}
std::uint32_t GameApi::multi_add(void* m,void* e){
    profile(14);if(!multis_.contains(m))return record(14,1);if(!easies_.contains(e))return record(14,2);
    bool created=false;if(reservations_&&!reservations_->reserve(e,created))return record(14,3);
    auto status=transfers_.add(m,e);if(status&&created)reservations_->discard(e);return record(14,status);
}
std::uint32_t GameApi::multi_remove(void* m,void* e){
    profile(15);auto status=!multis_.contains(m)?1:!easies_.contains(e)?2:transfers_.remove(m,e);
    if(!status&&reservations_)reservations_->discard(e);return record(15,status);
}
std::uint32_t GameApi::multi_perform(void* m,int* running){
    profile(19);if(!multis_.contains(m))return record(19,1);
    std::uint32_t n=0;auto status=transfers_.perform(m,running?&n:nullptr);
    if(!status&&running)std::memcpy(running,&n,sizeof(n));
    return record(19,status);
}
std::uint32_t GameApi::multi_poll(void* m,int timeout,int* count){
    profile(17);if(!multis_.contains(m))return record(17,1);
    std::uint32_t n=0;auto status=transfers_.poll(m,static_cast<std::uint32_t>(timeout),count?&n:nullptr);
    if(!status&&count)std::memcpy(count,&n,sizeof(n));
    return record(17,status);
}
void* GameApi::multi_info(void* m,int* remaining){
    profile(21);if(!multis_.contains(m)||!remaining)return nullptr;
    std::uint32_t n=0;auto* msg=transfers_.info_read(m,&n);std::memcpy(remaining,&n,sizeof(n));return msg;
}
}
extern "C" int curl_global_init(long){auto* a=owner.load();return a?a->global_init():2;}
extern "C" void curl_global_cleanup(){if(auto* a=owner.load())a->global_cleanup();}
extern "C" void* curl_easy_init(){auto* a=owner.load();return a?a->easy_init():nullptr;}
extern "C" void curl_easy_cleanup(void* p){if(auto* a=owner.load())a->easy_cleanup(p);}
extern "C" int curl_easy_setopt(void* p,int key,...){
    auto* a=owner.load();if(!a)return 2;
    std::uint64_t value=0;
    va_list args;va_start(args,key);
    // Win64 ABI: the retail va_list points at eight-byte argument slots.
    // Select the real argument class for every currently handled option.
    switch(key){
    case 3:case 13:case 14:case 41:case 42:case 44:case 45:case 46:case 47:case 52:
    case 54:case 60:case 64:case 68:case 78:case 80:case 81:case 84:case 98:case 136:
    case 155:case 156:case 157:case 158:case 161:case 207:case 280:
        value=static_cast<std::uint32_t>(va_arg(args,long));break;
    case 30115:case 30120:value=static_cast<std::uint64_t>(va_arg(args,std::int64_t));break;
    case 10002:case 10015:case 10016:case 10018:case 10025:case 10036:case 10102:case 10165:
        value=bits(va_arg(args,const char*));break;
    case 10023:value=bits(va_arg(args,curl_slist*));break;
    case 10001:case 10009:case 10010:case 10029:case 10037:case 10095:case 10103:case 10282:case 10284:
        value=bits(va_arg(args,void*));break;
    case 20011:case 20012:case 20079:value=bits(va_arg(args,WriteFn));break;
    case 20094:value=bits(va_arg(args,DebugFn));break;
    case 20283:value=bits(va_arg(args,TrailerFn));break;
    default:break; // unsupported/unknown options do not consume the argument
    }
    va_end(args);
    // Guard two unsupported null-string domains in the owned dispatcher.
    if(!value&&(key==10102||key==10165))return 43;
    return a->setopt(p,key,value);
}
extern "C" int curl_easy_getinfo(void* p,int key,...){
    auto* a=owner.load();if(!a)return 2;
    void* out=nullptr;
    va_list args;va_start(args,key);
    switch(static_cast<unsigned>(key)&0xf00000){
    case 0x100000:out=va_arg(args,char**);break;
    case 0x200000:out=va_arg(args,std::int32_t*);break;
    case 0x300000:out=va_arg(args,double*);break;
    case 0x600000:out=va_arg(args,std::int64_t*);break;
    default:out=va_arg(args,void*);break;
    }
    va_end(args);return a->getinfo(p,key,out);
}
extern "C" void* curl_multi_init(){auto* a=owner.load();return a?a->multi_init():nullptr;}
extern "C" int curl_multi_cleanup(void* m){auto* a=owner.load();return a?a->multi_cleanup(m):1;}
extern "C" int curl_multi_add_handle(void* m,void* e){auto* a=owner.load();return a?a->multi_add(m,e):1;}
extern "C" int curl_multi_remove_handle(void* m,void* e){auto* a=owner.load();return a?a->multi_remove(m,e):1;}
extern "C" int curl_multi_perform(void* m,int* n){auto* a=owner.load();return a?a->multi_perform(m,n):1;}
extern "C" int curl_multi_poll(void* m,void*,unsigned,int timeout,int* n){auto* a=owner.load();return a?a->multi_poll(m,timeout,n):1;}
extern "C" t6_xcurl::GameMessage* curl_multi_info_read(void* m,int* n){auto* a=owner.load();return a?static_cast<t6_xcurl::GameMessage*>(a->multi_info(m,n)):nullptr;}
