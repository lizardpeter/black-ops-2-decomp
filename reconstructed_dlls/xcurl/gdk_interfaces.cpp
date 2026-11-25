// Original XCurl GDK/COM-style QueryApiImpl wrappers.
// GUIDs, QueryApiImpl order, vtable slots and Release ownership are derived
// from the Store XCurl machine code. Signatures are cross-checked with the
// public IXThreadingImpl and IXNetworkingImpl interface layouts.
#include "gdk_interfaces.h"
#include <type_traits>
namespace {
struct Guid { std::uint32_t a; std::uint16_t b,c; std::uint8_t d[8]; };
constexpr Guid kError{0x8ca467f7,0x22e8,0x4096,{0x84,0x56,0xbb,0x8a,0xa1,0x3f,0x79,0xd8}};
constexpr Guid kThreading{0x073b7dcb,0x1fcf,0x4030,{0x94,0xbe,0xe3,0xc9,0xeb,0x62,0x34,0x28}};
constexpr Guid kNetworking{0x37e56907,0x2f10,0x41e8,{0xb7,0x2f,0x36,0xed,0xb1,0x85,0x33,0x1a}};
constexpr Guid kNetworking2{0xbf2346b2,0x39af,0x4658,{0xb5,0xea,0x44,0x71,0x3c,0x7e,0x83,0xb3}};
bool failed(std::uint32_t hr){return (hr&0x80000000U)!=0;}
template<class Fn,class...Args>
auto invoke(const t6_xcurl::GdkQueryServices* s,const Guid& clsid,const Guid& iid,
            unsigned slot,Args...args) -> std::invoke_result_t<Fn,void*,Args...> {
    using R=std::invoke_result_t<Fn,void*,Args...>;
    void* object=nullptr;
    const auto hr=s->query(&clsid,&iid,&object,s->context);
    if(failed(hr)){
        if constexpr(std::is_void_v<R>) return;
        else if constexpr(std::is_same_v<R,std::uint8_t>) return 0;
        else return hr;
    }
    auto** vtable=*static_cast<void***>(object);
    auto method=reinterpret_cast<Fn>(vtable[slot]);
    if constexpr(std::is_void_v<R>){
        method(object,args...);
    }else{
        R result=method(object,args...);
        auto release=reinterpret_cast<std::uint32_t(*)(void*)>(vtable[2]);
        release(object);
        return result;
    }
    if constexpr(std::is_void_v<R>){
        auto release=reinterpret_cast<std::uint32_t(*)(void*)>(vtable[2]);
        release(object);
    }
}
using HResult=std::uint32_t;
using Bool=std::uint8_t;
}
extern "C" std::uint32_t xcurl_research_gdk_async_get_status(void* block,std::uint8_t wait,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*,Bool);
    return invoke<Fn>(s,kThreading,kThreading,3,block,wait);
}
extern "C" void xcurl_research_gdk_async_cancel(void* block,const t6_xcurl::GdkQueryServices* s){
    using Fn=void(*)(void*,void*);
    invoke<Fn>(s,kThreading,kThreading,5,block);
}
// Original 0x54f4; this interface is distinct from the loader's XErrorReport export.
extern "C" void xcurl_research_gdk_error_report(std::uint32_t status,const char* message,const t6_xcurl::GdkQueryServices* s){
    xcurl_research_gdk_thread_assert_not_time_sensitive(s);
    using Fn=void(*)(void*,std::uint32_t,const char*);
    invoke<Fn>(s,kError,kError,3,status,message);
}
extern "C" std::uint32_t xcurl_research_gdk_network_get_connectivity_hint(void* out,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*);
    return invoke<Fn>(s,kNetworking,kNetworking2,15,out);
}
extern "C" std::uint32_t xcurl_research_gdk_network_security_async(const char16_t* url,void* block,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,const char16_t*,void*);
    return invoke<Fn>(s,kNetworking,kNetworking2,11,url,block);
}
extern "C" std::uint32_t xcurl_research_gdk_network_security_result(void* block,std::uint64_t size,std::uint64_t* used,void* buffer,void** info,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*,std::uint64_t,std::uint64_t*,void*,void**);
    return invoke<Fn>(s,kNetworking,kNetworking2,13,block,size,used,buffer,info);
}
extern "C" std::uint32_t xcurl_research_gdk_network_security_result_size(void* block,std::uint64_t* size,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*,std::uint64_t*);
    return invoke<Fn>(s,kNetworking,kNetworking2,12,block,size);
}
extern "C" std::uint32_t xcurl_research_gdk_network_register_connectivity(void* queue,void* context,void* callback,void* token,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*,void*,void*,void*);
    return invoke<Fn>(s,kNetworking,kNetworking2,16,queue,context,callback,token);
}
extern "C" std::uint8_t xcurl_research_gdk_network_unregister_connectivity(std::uint64_t token,std::uint8_t wait,const t6_xcurl::GdkQueryServices* s){
    using Fn=Bool(*)(void*,std::uint64_t,Bool);
    xcurl_research_gdk_thread_assert_not_time_sensitive(s);
    return invoke<Fn>(s,kNetworking,kNetworking2,17,token,wait);
}
extern "C" std::uint32_t xcurl_research_gdk_network_verify_certificate(void* request,const void* info,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*,const void*);
    xcurl_research_gdk_thread_assert_not_time_sensitive(s);
    return invoke<Fn>(s,kNetworking,kNetworking2,14,request,info);
}
extern "C" void xcurl_research_gdk_task_queue_close(void* queue,const t6_xcurl::GdkQueryServices* s){
    using Fn=void(*)(void*,void*);
    invoke<Fn>(s,kThreading,kThreading,17,queue);
}
extern "C" std::uint32_t xcurl_research_gdk_task_queue_create(std::uint32_t work,std::uint32_t completion,void** queue,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,std::uint32_t,std::uint32_t,void**);
    xcurl_research_gdk_thread_assert_not_time_sensitive(s);
    return invoke<Fn>(s,kThreading,kThreading,12,work,completion,queue);
}
extern "C" std::uint8_t xcurl_research_gdk_task_queue_dispatch(void* queue,std::uint32_t port,std::uint32_t timeout,const t6_xcurl::GdkQueryServices* s){
    using Fn=Bool(*)(void*,void*,std::uint32_t,std::uint32_t);
    return invoke<Fn>(s,kThreading,kThreading,16,queue,port,timeout);
}
extern "C" std::uint32_t xcurl_research_gdk_task_queue_submit(void* queue,std::uint32_t port,void* context,void* callback,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*,std::uint32_t,void*,void*);
    return invoke<Fn>(s,kThreading,kThreading,18,queue,port,context,callback);
}
extern "C" std::uint32_t xcurl_research_gdk_task_queue_terminate(void* queue,std::uint8_t wait,void* context,void* callback,const t6_xcurl::GdkQueryServices* s){
    using Fn=HResult(*)(void*,void*,Bool,void*,void*);
    xcurl_research_gdk_thread_assert_not_time_sensitive(s);
    return invoke<Fn>(s,kThreading,kThreading,22,queue,wait,context,callback);
}
extern "C" void xcurl_research_gdk_thread_assert_not_time_sensitive(const t6_xcurl::GdkQueryServices* s){
    using Fn=void(*)(void*);
    invoke<Fn>(s,kThreading,kThreading,29);
}
