// Real loopback HTTP smoke for reconstructed setup and transfer leaves. The
// harness drives callbacks; not a test of the still-incomplete production task queue.
#include "winhttp_services.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>void put(P p,unsigned o,T v){std::memcpy(p+o,&v,sizeof(v));}
template<class T>T get(P p,unsigned o){T v;std::memcpy(&v,p+o,sizeof(v));return v;}
void require(bool ok,const char* label){if(!ok){std::fprintf(stderr,"FAIL: %s (last_error=%lu)\n",label,GetLastError());std::exit(1);}}
struct Notice {DWORD status;DWORD length;DWORD error;};
struct Notices {
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<Notice> queue;
    void record(Notice n){std::lock_guard l(mutex);queue.push_back(n);cv.notify_all();}
    Notice wait(DWORD wanted){
        std::unique_lock l(mutex);auto until=std::chrono::steady_clock::now()+std::chrono::seconds(20);
        for(;;){
            if(!cv.wait_until(l,until,[&]{return !queue.empty();})){require(false,"callback timeout");}
            auto n=queue.front();queue.pop_front();
            if(n.status==WINHTTP_CALLBACK_STATUS_REQUEST_ERROR){std::fprintf(stderr,"WinHTTP callback error: %lu\n",n.error);require(false,"callback request error");}
            if(n.status==wanted)return n;
        }
    }
};
// One active request at a time in this smoke harness; lifetime is synchronized
// by HANDLE_CLOSING before the next one is started.
std::atomic<Notices*> active{nullptr};
void CALLBACK notify(HINTERNET,DWORD_PTR,DWORD status,LPVOID info,DWORD length){
    auto* n=active.load();
    if(!n)return;
    DWORD error=status==WINHTTP_CALLBACK_STATUS_REQUEST_ERROR?static_cast<WINHTTP_ASYNC_RESULT*>(info)->dwError:0;
    n->record({status,length,error});
}
void diagnostic(void*,std::uint32_t site,std::uint32_t status,void*){std::fprintf(stderr,"XCurl site %x status %x\n",site,status);}
void init_cs(P p){require(InitializeCriticalSectionEx(reinterpret_cast<CRITICAL_SECTION*>(p),0,0)!=FALSE,"init critical section");}
void destroy_cs(P p){DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(p));}
std::string request(P easy,void* session,std::uint16_t port,const char16_t* method,const char16_t* path,const std::string& body,WinHttpHost& host){
    P r=easy+0x230;std::memset(r,0,0xe0);put(r,0x30,easy);put(r,0x40,session);
    alignas(8) std::array<std::uint8_t,0x68> url{};char16_t hostname[]=u"127.0.0.1";
    put(url.data(),0x14,1U);put(url.data(),0x18,hostname+0);put(url.data(),0x20,9U);put(url.data(),0x48,path);
    auto setup=winhttp_setup_services(&host);require(xcurl_research_http_open(r,easy,method,url.data(),nullptr,port,&setup)==0,"native http_open");
    P opt=easy+0x78;put(opt,0x108,1U);put(opt,0x28,10000U);put(opt,0x2c,10000U);
    require(xcurl_research_http_options(r,opt,1,&setup)==0,"native http_options");
    Notices notices;active=&notices;RequestServices rs{};bind_winhttp_request_services(rs,&host);
    require(xcurl_research_http_register(r,&rs)==0,"native http_register");
    require(xcurl_research_http_send(r,body.empty()?nullptr:body.data(),body.size(),body.size(),&rs)==0,"native http_send");
    notices.wait(WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE);
    require(xcurl_research_http_receive(r,&rs)==0,"native http_receive");notices.wait(WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE);
    auto h=get<HINTERNET>(r,0x50);DWORD code=0,len=sizeof(code);
    require(WinHttpQueryHeaders(h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&code,&len,nullptr)!=FALSE&&code==200,"HTTP 200");
    std::array<char,8192> buffer{};std::string response;
    for(;;){require(xcurl_research_http_read(r,buffer.data(),static_cast<std::uint32_t>(buffer.size()),&rs)==0,"native http_read");auto n=notices.wait(WINHTTP_CALLBACK_STATUS_READ_COMPLETE);if(!n.length)break;response.append(buffer.data(),n.length);}
    require(WinHttpCloseHandle(h)!=FALSE,"close request");notices.wait(WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING);active=nullptr;
    require(WinHttpCloseHandle(get<HINTERNET>(r,0x48))!=FALSE,"close connection");
    return response;
}
}
int main(int argc,char** argv){
    require(argc==2,"port required");auto port=static_cast<std::uint16_t>(std::atoi(argv[1]));
    alignas(8) std::array<std::uint8_t,0x80> cache{};alignas(8) std::array<std::uint8_t,0x368> easy{};
    init_cs(cache.data());init_cs(easy.data()+0x188);auto* list=cache.data()+0x28;put(list,0,list);put(list,8,list);
    WinHttpHost host{notify,diagnostic,nullptr};auto ss=winhttp_session_services(&host);void* first=nullptr;void* second=nullptr;
    require(xcurl_research_session_acquire(cache.data(),easy.data(),0,1,&first,&ss)==0&&first,"session acquire");
    require(xcurl_research_session_acquire(cache.data(),easy.data(),0,1,&second,&ss)==0&&second==first,"session cache hit");
    require(request(easy.data(),first,port,u"GET",u"/get",{},host)=="xcurl-native-get","GET body");
    std::string body="xcurl-native-post";require(request(easy.data(),second,port,u"POST",u"/echo",body,host)==body,"POST echo");
    require(xcurl_research_session_release(second,&ss)==2,"release second reference");require(xcurl_research_session_release(first,&ss)==1,"release first reference");
    xcurl_research_session_cache_clear(cache.data(),&ss);destroy_cs(easy.data()+0x188);destroy_cs(cache.data());
    std::puts("XCurl native WinHTTP loopback PASS: session reuse, HTTP GET and POST, async send/receive/read, cleanup");
}
