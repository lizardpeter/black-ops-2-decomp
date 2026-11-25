// End-to-end Store C ABI, real WinHTTP loopback transport. GDK is modeled
// only in this fixture; no security override is installed in production.
#include "game_api.h"
#include "win32_url_binding.h"
#include "win32_engine_binding.h"
#include "winhttp_pipeline.h"
#include <array>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
using namespace t6_xcurl;
namespace {
void require(bool ok,const char* label){if(!ok){std::fprintf(stderr,"FAIL %s (Win32 %lu)\n",label,GetLastError());std::exit(1);}}
template<class T>T get(const void* p,unsigned off){T v;std::memcpy(&v,static_cast<const char*>(p)+off,sizeof(v));return v;}
template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<char*>(p)+off,&v,sizeof(v));}
struct Task {void* context;void(*fn)(void*,bool);};
struct Queue {std::mutex mutex;std::deque<Task> tasks;};
struct Mock {
    struct Interface{void** vt;Mock* owner;};
    std::array<void*,30> thread{},network{},errors{};
    Interface t{thread.data(),this},n{network.data(),this},e{errors.data(),this};
    Queue* engine=nullptr;
    std::atomic<unsigned> queue_count{0},queries{0},releases{0},starts{0},reports{0};
    Mock();
};
Mock& self(void* p){return *static_cast<Mock::Interface*>(p)->owner;}
std::uint32_t release(void* p){++self(p).releases;return 1;}
void assert_thread(void*){}
std::uint32_t create_queue(void* p,std::uint32_t w,std::uint32_t c,void** out){
    auto& m=self(p);require(w==c&&(w==0||w==1),"GDK queue mode");auto* q=new Queue;
    ++m.queue_count;*out=q;if(w==1)m.engine=q;return 0;
}
void close_queue(void* p,void* q){
    auto& m=self(p);auto* queue=static_cast<Queue*>(q);
    require(queue->tasks.empty(),"queue drained");if(m.engine==queue)m.engine=nullptr;
    --m.queue_count;delete queue;
}
std::uint32_t submit(void*,void* q,std::uint32_t port,void* context,void* fn){
    require(port==1&&q&&fn,"submit queue");auto* queue=static_cast<Queue*>(q);std::lock_guard lock(queue->mutex);
    queue->tasks.push_back({context,reinterpret_cast<void(*)(void*,bool)>(fn)});return 0;
}
std::uint8_t dispatch(void*,void* q,std::uint32_t port,std::uint32_t){
    if(port!=1)return 0;auto* queue=static_cast<Queue*>(q);Task task{};
    {std::lock_guard lock(queue->mutex);if(queue->tasks.empty())return 0;task=queue->tasks.front();queue->tasks.pop_front();}
    task.fn(task.context,false);return 1;
}
std::uint32_t terminate(void*,void*,std::uint8_t,void*,void*){return 0;}
std::uint32_t status(void*,void*,std::uint8_t){return 0;}
void cancel(void*,void*){}
std::uint32_t connect_hint(void*,void* out){std::memset(out,0,16);static_cast<char*>(out)[12]=1;return 0;}
std::uint32_t register_connect(void*,void*,void* engine,void* callback,void* token){
    put(token,0,std::uint64_t(1));reinterpret_cast<void(*)(void*,const void*)>(callback)(engine,nullptr);return 0;
}
std::uint8_t unregister_connect(void*,std::uint64_t,std::uint8_t){return 1;}
std::uint32_t security_async(void* p,const char16_t* text,void* block){
    require(std::u16string_view(text).starts_with(u"http://127.0.0.1:"),"loopback only security fixture");
    auto& m=self(p);++m.starts;
    auto* queue=get<Queue*>(block,0);require(queue==m.engine,"engine queue");
    auto worker=+[](void* b,bool){get<void(*)(void*)>(b,16)(b);};
    return submit(p,queue,1,block,reinterpret_cast<void*>(worker));
}
std::uint32_t security_size(void*,void*,std::uint64_t* n){*n=24;return 0;}
std::uint32_t security_result(void*,void*,std::uint64_t n,std::uint64_t*,void* data,void** info){
    require(n==24,"security metadata size");std::memset(data,0,24);*info=data;return 0;
}
std::uint32_t verify(void*,void*,const void*){return 0;} // fixture only: cleartext localhost
void report(void* p,std::uint32_t,const char* msg){require(std::strstr(msg,"127.0.0.1"),"loopback warning");++self(p).reports;}
template<class F>void slot(std::array<void*,30>& a,unsigned n,F fn){a[n]=reinterpret_cast<void*>(fn);}
Mock::Mock(){
    slot(thread,2,release);slot(thread,3,status);slot(thread,5,cancel);slot(thread,12,create_queue);slot(thread,16,dispatch);slot(thread,17,close_queue);slot(thread,18,submit);slot(thread,22,terminate);slot(thread,29,assert_thread);
    slot(network,2,release);slot(network,11,security_async);slot(network,12,security_size);slot(network,13,security_result);slot(network,14,verify);slot(network,15,connect_hint);slot(network,16,register_connect);slot(network,17,unregister_connect);
    slot(errors,2,release);slot(errors,3,report);
}
std::uint32_t query(const void* cls,const void*,void** out,void* c){
    auto& m=*static_cast<Mock*>(c);auto id=get<std::uint32_t>(cls,0);
    *out=id==0x073b7dcbU?&m.t:id==0x37e56907U?&m.n:id==0x8ca467f7U?&m.e:nullptr;
    if(!*out)return 0x80004002U;++m.queries;return 0;
}
void diag(void*,std::uint32_t site,std::uint32_t code,void*){std::fprintf(stderr,"XCurl site %x code %x\n",site,code);}
std::size_t writer(char* p,std::size_t n,std::size_t k,void* c){static_cast<std::string*>(c)->append(p,n*k);return k*n;}
struct Cursor {std::string text;std::size_t at=0;};
std::size_t reader(char* p,std::size_t n,std::size_t k,void* c){auto& cur=*static_cast<Cursor*>(c);auto count=std::min(n*k,cur.text.size()-cur.at);std::memcpy(p,cur.text.data()+cur.at,count);cur.at+=count;return count;}
void transfer(void* m,Mock& mock,unsigned port,int kind){
    auto* e=curl_easy_init();require(e,"easy init");auto url=std::string("http://127.0.0.1:")+std::to_string(port)+(kind?"/echo":"/get");
    std::string body,headers;Cursor upload;for(unsigned i=0;i<24579;++i)upload.text.push_back(static_cast<char>('a'+i%26));
    auto* list=curl_slist_append(nullptr,"X-XCurl-Request: original-path");require(list,"header list");
    require(!curl_easy_setopt(e,10002,url.c_str())&&!curl_easy_setopt(e,10023,list),"url/header options");
    require(!curl_easy_setopt(e,84,1L)&&!curl_easy_setopt(e,155,10000L),"HTTP version and timeout");
    require(!curl_easy_setopt(e,20011,&writer)&&!curl_easy_setopt(e,10001,static_cast<void*>(&body)),"body callback");
    require(!curl_easy_setopt(e,20079,&writer)&&!curl_easy_setopt(e,10029,static_cast<void*>(&headers)),"header callback");
    if(kind==1){require(!curl_easy_setopt(e,10015,upload.text.c_str())&&!curl_easy_setopt(e,60,static_cast<long>(upload.text.size())),"direct POST");}
    if(kind==2){require(!curl_easy_setopt(e,47,1L)&&!curl_easy_setopt(e,20012,&reader)&&!curl_easy_setopt(e,10009,static_cast<void*>(&upload))&&!curl_easy_setopt(e,60,static_cast<long>(upload.text.size()))&&!curl_easy_setopt(e,280,4096L),"streamed POST");}
    require(!curl_multi_add_handle(m,e),"multi add");
    auto until=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    int running=1;while(running){
        require(!curl_multi_perform(m,&running),"multi perform");
        while(dispatch(&mock.t,mock.engine,1,0)){}
        if(!running)break;
        int count=0;require(!curl_multi_poll(m,nullptr,0,50,&count),"multi poll");
        require(std::chrono::steady_clock::now()<until,"request timeout");
    }
    int n=99;auto* message=curl_multi_info_read(m,&n);
    require(message&&message->type==1&&message->easy==e&&!message->data.result&&!n,"completed message");
    std::int32_t response=0;require(!curl_easy_getinfo(e,0x200002,&response)&&response==200,"HTTP response 200");
    require(body==(kind?upload.text:"xcurl-native-get"),"exact response body");
    require(headers.find("X-XCurl-Test: native")!=std::string::npos,"header callback");
    if(kind==2)require(upload.at==upload.text.size(),"entire streamed upload");
    require(!curl_multi_info_read(m,&n)&&!n,"message consumed");
    require(!curl_multi_remove_handle(m,e),"multi remove");curl_easy_cleanup(e);curl_slist_free_all(list);
}
}
int main(int argc,char** argv){
    require(argc==2,"port argument");auto port=static_cast<unsigned>(std::atoi(argv[1]));
    Mock mock;GdkQueryServices gdk{query,&mock};ApiBinding api{};WinHttpHost host{nullptr,diag,nullptr};RequestServices request{};
    TaskQueueBinding tasks(api.global,request,gdk);Win32UrlBinding urls(host,request,gdk);
    WinHttpPipeline pipe(api.global,host,request,tasks,gdk,urls.crack_services(),nullptr);
    auto os=win32_multi_host_services(nullptr);TransferBinding transfers(api.global,urls.prepare_services(),os);
    EnginePlatformHost telemetry{[](void** p,void*)->std::uint32_t{*p=nullptr;return 0;},[](void*,void*){},nullptr};
    Win32EngineBinding engine(gdk,pipe.sessions(),transfers.scheduler(),telemetry);
    require(urls.activate()&&pipe.activate()&&engine.activate(),"activate native adapters");
    engine.bind_api(api,8);auto services=engine.services();
    // No actual Gaming Services or app registration in CI. All remaining OS
    // calls and XCurl functions are the same ones used by the native adapters.
    services.initialize_runtime=[](void*)->std::uint32_t{return 0;};
    services.register_app_state=[](void*,void*,void** p,void*)->std::uint32_t{*p=nullptr;return 0;};
    api.engine=&services;api.shutdown_runtime=nullptr;
    auto easy=win32_easy_lifecycle_services(&request,&gdk);
    auto multi=win32_multi_lifecycle_services(&transfers.scheduler(),&request);
    HandleBinding handles(api,easy,multi);GameApi facade(api,handles,transfers);
    require(facade.install()&&!curl_global_init(3),"global init");
    auto* m=curl_multi_init();require(m,"multi init");
    for(int kind=0;kind<3;++kind)transfer(m,mock,port,kind);
    require(!curl_multi_cleanup(m),"multi cleanup");curl_global_cleanup();
    require(!api.global.engine&&!api.global.references&&!mock.queue_count&&mock.queries==mock.releases&&mock.starts==3&&mock.reports>=3,"balanced runtime lifetime");
    require(facade.uninstall()&&engine.deactivate(),"deactivate engine");pipe.deactivate();urls.deactivate();
    std::puts("XCurl full C ABI loopback PASS: actual WinHTTP callbacks, GET/direct POST/streamed POST, modeled GDK only");
}
