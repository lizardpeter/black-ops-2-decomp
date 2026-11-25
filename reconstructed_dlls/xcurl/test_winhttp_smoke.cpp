// Real loopback HTTP smoke for reconstructed setup and transfer leaves. The
// harness drives callbacks; not a test of the still-incomplete production task queue.
#include "winhttp_services.h"
#include "winhttp_pipeline.h"
#include "win32_url_binding.h"
#include "allocation.h"
#include "gdk_runtime_win32.h"
#include "response_headers.h"
#include "transfer_callbacks.h"
#include <algorithm>
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
std::uint32_t crack_for_test(const char16_t* url,void*,std::uint32_t* scheme,void*){
    URL_COMPONENTS parts{};parts.dwStructSize=sizeof(parts);parts.dwSchemeLength=DWORD(-1);
    if(!WinHttpCrackUrl(reinterpret_cast<LPCWSTR>(url),0,0,&parts))return HRESULT_FROM_WIN32(GetLastError());
    *scheme=static_cast<std::uint32_t>(parts.nScheme);return 0;
}
void receive_diag(void*,std::uint32_t site,std::uint64_t a,std::uint32_t b,void*){std::fprintf(stderr,"receive diagnostic %x %llx %x\n",site,static_cast<unsigned long long>(a),b);}
void header_diag(void*,std::uint32_t site,std::uint64_t,std::uint64_t,void*){std::fprintf(stderr,"header diagnostic %x\n",site);}
std::uint64_t header_callback(void* p,std::uint64_t size,std::uint64_t n,void* ctx){
    require(size==1,"header callback size");static_cast<std::string*>(ctx)->append(static_cast<const char*>(p),static_cast<std::size_t>(n));return n;
}
struct ReadCursor { const std::string* body; std::size_t offset=0; };
std::uint64_t read_callback(void* p,std::uint64_t size,std::uint64_t n,void* ctx){
    require(size==1,"upload callback element size");auto& cursor=*static_cast<ReadCursor*>(ctx);auto count=std::min<std::uint64_t>(n,cursor.body->size()-cursor.offset);std::memcpy(p,cursor.body->data()+cursor.offset,static_cast<std::size_t>(count));cursor.offset+=static_cast<std::size_t>(count);return count;
}
std::uint64_t deliver_callback(void* callback,const void* p,std::uint64_t n,void* userdata,void*){
    return xcurl_research_invoke_write(reinterpret_cast<TransferCallback>(callback),p,n,userdata);
}
void upload_diag(void*,std::uint32_t site,std::uint64_t a,std::uint64_t b,void*){std::fprintf(stderr,"upload diagnostic %x %llx %llx\n",site,static_cast<unsigned long long>(a),static_cast<unsigned long long>(b));}
void delivery_diag(void*,std::uint32_t site,std::uint64_t a,std::uint32_t b,void*){upload_diag(nullptr,site,a,b,nullptr);}
void init_cs(P p){require(InitializeCriticalSectionEx(reinterpret_cast<CRITICAL_SECTION*>(p),0,0)!=FALSE,"init critical section");}
void destroy_cs(P p){DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(p));}
std::string request(P easy,void* session,std::uint16_t port,const char16_t* method,const char16_t* path,const std::string& body,WinHttpHost& host,bool streaming=false){
    P r=easy+0x230;std::memset(r,0,0xe0);init_cs(r);put(r,0x30,easy);put(r,0x40,session);put(r,0x28,2U);put(easy,0x80,std::u16string_view(method)==u"POST"?2U:0U);put(easy,0x228,0U);
    alignas(8) std::array<std::uint8_t,0x68> url{};char16_t hostname[]=u"127.0.0.1";
    put(url.data(),0x14,1U);put(url.data(),0x18,hostname+0);put(url.data(),0x20,9U);put(url.data(),0x48,path);
    auto setup=winhttp_setup_services(&host);require(xcurl_research_http_open(r,easy,method,url.data(),nullptr,port,&setup)==0,"native http_open");
    P opt=easy+0x78;put(opt,0x108,1U);put(opt,0x28,10000U);put(opt,0x2c,10000U);
    require(xcurl_research_http_options(r,opt,1,&setup)==0,"native http_options");
    Notices notices;active=&notices;RequestServices rs{};bind_winhttp_request_services(rs,&host);
    require(xcurl_research_http_register(r,&rs)==0,"native http_register");
    struct Node{const char* data;Node* next;};
    Node header{"X-XCurl-Request: original-path",nullptr};
    auto ah=winhttp_add_header_services(&rs);
    require(xcurl_research_add_request_headers(r,&header,&ah)==0,"original request header list");
    require(xcurl_research_http_send(r,streaming||body.empty()?nullptr:body.data(),streaming?0:body.size(),body.size(),&rs)==0,"native http_send");
    notices.wait(WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE);
    if(streaming){
        ReadCursor cursor{&body};put(easy,0x118,&cursor);put(easy,0x120,&read_callback);put(easy,0xb0,4096U);put(easy,0xf0,static_cast<std::uint64_t>(body.size()));put(easy,0xf8,static_cast<std::uint64_t>(body.size()));put(r,0xd0,1U);
        auto upload=winhttp_upload_services(&rs,nullptr,upload_diag);
        unsigned writes=0;
        while(!r[0xd5]){
            xcurl_research_upload_continue(r,&upload);require(get<std::uint32_t>(easy,0x228)==0,"original streamed upload state");
            notices.wait(WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE);require(++writes<=20,"upload bounded progress");
        }
        require(writes>1&&cursor.offset==body.size()&&get<std::uint64_t>(r,0xa0)==body.size(),"original multi-write upload count");
        rs.release(get<void*>(r,0xb0),16,rs.context);put(r,0xb0,static_cast<void*>(nullptr));put(easy,0x120,static_cast<TransferCallback>(nullptr));put(easy,0x118,static_cast<void*>(nullptr));
    }
    require(xcurl_research_http_receive(r,&rs)==0,"native http_receive");notices.wait(WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE);
    auto h=get<HINTERNET>(r,0x50);std::uint32_t code=0;
    auto ascii=std::string("http://127.0.0.1:")+std::to_string(port);
    std::u16string effective(ascii.begin(),ascii.end());effective+=path;put(r,0x38,effective.c_str());
    auto qs=winhttp_header_query_services(&host);ReceiveHeaderServices hs{&qs,&rs,crack_for_test,receive_diag};
    require(xcurl_research_receive_headers(r,easy,&code,&hs)==0&&code==200,"reconstructed headers / HTTP 200");
    require(get<std::uint32_t>(easy,0x1e0)==200&&get<std::uint32_t>(easy,0x1dc)==2,"published response status/version");
    require(std::strcmp(get<const char*>(easy,0x210),"http")==0,"published scheme");
    require(std::strcmp(get<const char*>(easy,0x218),"text/plain")==0,"published content type");
    std::string header_text;put(easy,0x128,&header_text);put(easy,0x130,&header_callback);
    HeaderServices hworker{&rs,nullptr,header_diag};xcurl_research_header_worker(easy,&hworker);
    require(header_text.starts_with("HTTP/1.1 200")&&header_text.find("X-XCurl-Test: native")!=std::string::npos,"header callback bytes");
    require(!get<void*>(easy,0x1d0)&&!get<std::uint64_t>(easy,0x1c8),"header ownership released");
    put(easy,0x130,static_cast<TransferCallback>(nullptr));put(easy,0x128,static_cast<void*>(nullptr));
    std::array<char,8192> buffer{};std::string response;put(easy,0xac,2048U);put(easy,0x108,&response);put(easy,0x110,&header_callback);put(easy,0x33c,0U);
    DeliveryServices ds{&rs,deliver_callback,delivery_diag};
    for(;;){require(xcurl_research_http_read(r,buffer.data(),static_cast<std::uint32_t>(buffer.size()),&rs)==0,"native http_read");auto n=notices.wait(WINHTTP_CALLBACK_STATUS_READ_COMPLETE);if(!n.length)break;require(xcurl_research_deliver_body(easy,buffer.data(),n.length,&ds)==0,"original body delivery");}
    require(get<std::uint32_t>(easy,0x33c)==response.size(),"original body accounting");put(easy,0x110,static_cast<TransferCallback>(nullptr));put(easy,0x108,static_cast<void*>(nullptr));
    require(get<std::uint64_t>(easy,0x1f8)==response.size(),"published content length");
    for(auto off:{0x208U,0x218U}){rs.release(get<void*>(easy,off),8,rs.context);put(easy,off,static_cast<void*>(nullptr));}
    require(WinHttpCloseHandle(h)!=FALSE,"close request");notices.wait(WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING);active=nullptr;
    require(WinHttpCloseHandle(get<HINTERNET>(r,0x48))!=FALSE,"close connection");
    destroy_cs(r);return response;
}
}
int main(int argc,char** argv){
#ifdef _WIN32
    require(win32_gdk_query_services()!=nullptr,"native GDK QueryApiImpl adapter available");
    // Verify the composed native callback/service registration without issuing
    // a real GDK request on a CI machine without Gaming Services.
    {GlobalLifecycle g;RequestServices r{};WinHttpHost h{};UrlServices url{};
     auto* d=win32_gdk_query_services();TaskQueueBinding tasks(g,r,*d);
     Win32UrlBinding urls(h,r,*d);require(urls.activate(),"native URL services installed");
     char16_t* prepared=nullptr;
     require(urls.prepare("https://example.test:443/a?b=c",nullptr,&prepared)==0&&std::u16string(prepared)==u"https://example.test:443/a?b=c","native absolute URL preparation");
     xcurl_research_heap_free(prepared,7);prepared=nullptr;
     require(urls.prepare("example.test",nullptr,&prepared)==0&&std::u16string(prepared)==u"https://example.test","native original HTTPS fallback");
     xcurl_research_heap_free(prepared,7);prepared=nullptr;
     require(urls.prepare("https://host:0",nullptr,&prepared)==0x80072ee5U&&!prepared,"native rejected URL");
     require(urls.prepare_services().start_callback==CallbackBinding::start_token(),"native start callback token");
     urls.deactivate();require(!h.urls,"native URL binding shutdown");
     WinHttpPipeline pipeline(g,h,r,tasks,*d,url,nullptr);
     require(pipeline.activate()&&h.status_callback&&r.create_queue&&r.submit_completion&&r.debug_data,"native composed pipeline activation");
     require(pipeline.start().request==&r&&pipeline.status().request==&r,"native pipeline service ownership");
     pipeline.deactivate();require(!h.pipeline&&!h.status_callback,"native pipeline deactivation");}
#endif
    require(argc==2,"port required");auto port=static_cast<std::uint16_t>(std::atoi(argv[1]));
    alignas(8) std::array<std::uint8_t,0x80> cache{};alignas(8) std::array<std::uint8_t,0x368> easy{};
    init_cs(cache.data());init_cs(easy.data()+0x188);init_cs(easy.data()+0x50);auto* list=cache.data()+0x28;put(list,0,list);put(list,8,list);
    WinHttpHost host{notify,diagnostic,nullptr};auto ss=winhttp_session_services(&host);void* first=nullptr;void* second=nullptr;
    require(xcurl_research_session_acquire(cache.data(),easy.data(),0,1,&first,&ss)==0&&first,"session acquire");
    require(xcurl_research_session_acquire(cache.data(),easy.data(),0,1,&second,&ss)==0&&second==first,"session cache hit");
    require(request(easy.data(),first,port,u"GET",u"/get",{},host)=="xcurl-native-get","GET body");
    std::string body="xcurl-native-post";require(request(easy.data(),second,port,u"POST",u"/echo",body,host)==body,"POST echo");
    std::string streamed;for(unsigned i=0;i<24579;++i)streamed.push_back(static_cast<char>('a'+i%26));
    require(request(easy.data(),second,port,u"POST",u"/echo",streamed,host,true)==streamed,"streamed POST echo");
    require(xcurl_research_session_release(second,&ss)==2,"release second reference");require(xcurl_research_session_release(first,&ss)==1,"release first reference");
    xcurl_research_session_cache_clear(cache.data(),&ss);destroy_cs(easy.data()+0x50);destroy_cs(easy.data()+0x188);destroy_cs(cache.data());
    std::puts("XCurl native WinHTTP loopback PASS: session reuse, HTTP GET, direct POST, streamed multi-write POST, original outgoing headers/body delivery, header metadata/callback, cleanup");
}
