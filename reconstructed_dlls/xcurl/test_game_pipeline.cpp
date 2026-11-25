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
#include <vector>
#include <wincrypt.h>
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
    bool tls=false;
    unsigned bad_metadata=0;
    std::atomic<unsigned> verifications{0},accepted{0};
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
    require(std::u16string_view(text).starts_with(self(p).tls?u"https://127.0.0.1:":u"http://127.0.0.1:"),"loopback only security fixture");
    auto& m=self(p);++m.starts;
    auto* queue=get<Queue*>(block,0);require(queue==m.engine,"engine queue");
    auto worker=+[](void* b,bool){get<void(*)(void*)>(b,16)(b);};
    return submit(p,queue,1,block,reinterpret_cast<void*>(worker));
}
std::uint32_t security_size(void* p,void*,std::uint64_t* n){*n=self(p).bad_metadata==1?8:24;return 0;}
std::uint32_t security_result(void* p,void*,std::uint64_t n,std::uint64_t*,void* data,void** info){
    require(n==24,"security metadata size");std::memset(data,0,24);if(self(p).tls)put(data,0,std::uint32_t(0x800));*info=data;
    switch(self(p).bad_metadata){case 2:*info=nullptr;break;case 3:*info=static_cast<char*>(data)+8;break;case 4:put(data,8,UINT64_MAX);break;case 5:put(data,8,std::uint64_t(1));put(data,16,static_cast<char*>(data)+16);break;}
    return 0;
}
std::uint32_t verify(void* p,void* request,const void*){
    auto& m=self(p);if(!m.tls)return 0; // cleartext loopback only
    ++m.verifications;
    // Test substitute for the GDK certificate service uses Windows trust
    // chain and SSL hostname policy, not a blanket acceptance callback.
    PCCERT_CONTEXT cert=nullptr;DWORD size=sizeof(cert);
    if(!WinHttpQueryOption(request,WINHTTP_OPTION_SERVER_CERT_CONTEXT,&cert,&size)||!cert)return 0x80072f89U;
    CERT_CHAIN_PARA params{};params.cbSize=sizeof(params);
    LPSTR eku=const_cast<LPSTR>(szOID_PKIX_KP_SERVER_AUTH);
    params.RequestedUsage.dwType=USAGE_MATCH_TYPE_AND;
    params.RequestedUsage.Usage.cUsageIdentifier=1;params.RequestedUsage.Usage.rgpszUsageIdentifier=&eku;
    PCCERT_CHAIN_CONTEXT chain=nullptr;
    bool ok=CertGetCertificateChain(nullptr,cert,nullptr,cert->hCertStore,&params,0,nullptr,&chain)!=FALSE;
    if(ok){
        SSL_EXTRA_CERT_CHAIN_POLICY_PARA ssl{};ssl.cbSize=sizeof(ssl);ssl.dwAuthType=AUTHTYPE_SERVER;
        ssl.pwszServerName=const_cast<wchar_t*>(L"127.0.0.1");
        CERT_CHAIN_POLICY_PARA policy{};policy.cbSize=sizeof(policy);policy.pvExtraPolicyPara=&ssl;
        CERT_CHAIN_POLICY_STATUS result{};result.cbSize=sizeof(result);
        ok=CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,chain,&policy,&result)&&!result.dwError;
    }
    if(chain)CertFreeCertificateChain(chain);CertFreeCertificateContext(cert);
    if(ok)++m.accepted;
    return ok?0U:0x80072f89U;
}
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
// Exercise scheduler saturation, easy-handle reuse, failed preparation, HTTP
// response error and active cancellation using the actual public ABI.
void pump(void* m,Mock& mock){
    auto until=std::chrono::steady_clock::now()+std::chrono::seconds(20);
    int running=1;while(running){
        require(!curl_multi_perform(m,&running),"batch multi perform");
        require(get<std::uint32_t>(m,0x3c)<=8,"global admission limit");
        while(dispatch(&mock.t,mock.engine,1,0)){}
        if(!running)break;
        int n=0;require(!curl_multi_poll(m,nullptr,0,50,&n),"batch multi poll");
        require(std::chrono::steady_clock::now()<until,"batch timeout");
    }
}
void configure(void* e,const std::string& url,curl_slist* list,std::string* output){
    require(!curl_easy_setopt(e,10002,url.c_str())&&!curl_easy_setopt(e,10023,list),"batch URL/header");
    require(!curl_easy_setopt(e,84,1L)&&!curl_easy_setopt(e,155,10000L),"batch timeout");
    require(!curl_easy_setopt(e,20011,&writer)&&!curl_easy_setopt(e,10001,static_cast<void*>(output)),"batch body");
}
void batch(void* m,Mock& mock,unsigned port){
    constexpr unsigned count=13;
    std::array<void*,count> handles{};std::array<std::string,count> bodies{};std::array<bool,count> seen{};
    auto* list=curl_slist_append(nullptr,"X-XCurl-Request: original-path");
    const auto base=std::string("http://127.0.0.1:")+std::to_string(port);
    for(unsigned i=0;i<count;++i){
        handles[i]=curl_easy_init();require(handles[i],"batch easy");
        configure(handles[i],base+"/get",list,&bodies[i]);require(!curl_multi_add_handle(m,handles[i]),"batch add");
    }
    require(curl_multi_add_handle(m,handles[0])==7,"duplicate easy add rejected");
    int running=0;require(!curl_multi_perform(m,&running)&&running==count&&get<std::uint32_t>(m,0x3c)==8,"eight admitted five pending");
    pump(m,mock);
    int n=0;unsigned completed=0;
    while(auto* msg=curl_multi_info_read(m,&n)){
        auto it=std::find(handles.begin(),handles.end(),msg->easy);require(it!=handles.end()&&msg->type==1&&!msg->data.result,"batch successful result");
        auto i=static_cast<unsigned>(it-handles.begin());require(!seen[i]&&bodies[i]=="xcurl-native-get","batch response matched");
        seen[i]=true;++completed;require(n==count-completed,"batch exact message count");
        require(!curl_multi_remove_handle(m,handles[i]),"batch remove");
    }
    require(completed==count&&!n,"batch completed");
    // Reuse one of the just-completed easy handles: state and metadata must reset.
    auto* e=handles[0];bodies[0].clear();
    configure(e,base+"/get",list,&bodies[0]);require(!curl_multi_add_handle(m,e),"reuse add");
    pump(m,mock);auto* msg=curl_multi_info_read(m,&n);
    require(msg&&msg->easy==e&&!msg->data.result&&!n&&bodies[0]=="xcurl-native-get","reuse completion");
    require(!curl_multi_remove_handle(m,e),"reuse remove");
    require(!curl_multi_info_read(m,&n),"reuse message retired");
    // Invalid URL never reaches the modeled security service.
    auto starts=mock.starts.load();bodies[0].clear();
    configure(e,"https://host:0",list,&bodies[0]);require(!curl_multi_add_handle(m,e),"invalid add");
    pump(m,mock);msg=curl_multi_info_read(m,&n);
    std::int32_t error=0;require(msg&&msg->easy==e&&msg->data.result==3&&!n&&mock.starts==starts,"invalid URL error completion");
    require(!curl_easy_getinfo(e,0x200019,&error)&&static_cast<std::uint32_t>(error)==0x80072ee5U,"original HRESULT getinfo");
    require(curl_easy_strerror(msg->data.result)&&*curl_easy_strerror(msg->data.result),"original error string symbol linked");
    require(!curl_multi_remove_handle(m,e),"invalid remove");require(!curl_multi_info_read(m,&n),"invalid message retired");
    require(!curl_easy_setopt(e,10002,static_cast<const char*>(nullptr))&&!curl_multi_add_handle(m,e),"missing URL add");
    pump(m,mock);msg=curl_multi_info_read(m,&n);
    require(msg&&msg->data.result==3&&!n&&mock.starts==starts,"missing URL rejected before security");
    require(!curl_multi_remove_handle(m,e)&&!curl_multi_info_read(m,&n),"missing URL retired");
    for(unsigned mode=1;mode<=5;++mode){
        mock.bad_metadata=mode;configure(e,base+"/get",list,&bodies[0]);
        require(!curl_multi_add_handle(m,e),"malformed security result add");pump(m,mock);msg=curl_multi_info_read(m,&n);
        require(msg&&msg->easy==e&&msg->data.result==58&&!n&&bodies[0].empty(),"malformed security result rejected");
        require(!curl_easy_getinfo(e,0x200019,&error)&&static_cast<std::uint32_t>(error)==0x8007000dU,"malformed result HRESULT");
        require(!curl_multi_remove_handle(m,e)&&!curl_multi_info_read(m,&n),"malformed result retired");
    }
    mock.bad_metadata=0;
    // The actual HTTP status notification publishes metadata before fail-on-error.
    configure(e,base+"/missing",list,&bodies[0]);require(!curl_easy_setopt(e,45,1L)&&!curl_multi_add_handle(m,e),"HTTP error add");
    pump(m,mock);msg=curl_multi_info_read(m,&n);
    std::int32_t http=0;require(msg&&msg->easy==e&&msg->data.result==22&&!n,"HTTP fail-on-error result");
    require(!curl_easy_getinfo(e,0x200002,&http)&&http==404,"HTTP error status getinfo");
    require(!curl_multi_remove_handle(m,e),"HTTP error remove");require(!curl_multi_info_read(m,&n),"HTTP error message retired");
    // Start a genuine slow WinHTTP request, cancel after the security worker
    // has opened it, and require completion/drain before freeing the easy.
    configure(e,base+"/slow",list,&bodies[0]);require(!curl_easy_setopt(e,45,0L)&&!curl_multi_add_handle(m,e),"slow add");
    require(!curl_multi_perform(m,&running)&&running==1,"slow admission");
    while(dispatch(&mock.t,mock.engine,1,0)){}
    require(get<std::uint32_t>(e,0x258)==2,"active WinHTTP handle before cancellation");
    require(!curl_multi_remove_handle(m,e)&&get<std::uint32_t>(e,0x258)==0,"active cancel drained");
    require(!curl_multi_info_read(m,&n)&&!n,"canceled detached easy does not publish completion");
    require(!curl_easy_getinfo(e,0x200019,&error)&&static_cast<std::uint32_t>(error)==0x80004004U,"canceled HRESULT");
    for(auto* handle:handles)curl_easy_cleanup(handle);
    curl_slist_free_all(list);
    std::puts("XCurl batch PASS: admission limit, 13 parallel requests, reuse, invalid URL, HTTP error, active cancel");
}
// Exercise the original clients' two branch-dependent method selectors,
// custom method, verbose callback, header-in-body fallback and input length.
struct DebugTrace {
    std::mutex mutex;
    std::string headers,received,sent;
};
int debug_callback(void*,int type,char* p,std::size_t n,void* c){
    auto& d=*static_cast<DebugTrace*>(c);std::lock_guard lock(d.mutex);
    if(type==1)d.headers.append(p,n);
    if(type==3)d.received.append(p,n);
    if(type==4)d.sent.append(p,n);
    return 0;
}
void method_transfers(void* m,Mock& mock,unsigned port){
    auto base=std::string("http://127.0.0.1:")+std::to_string(port);
    auto* list=curl_slist_append(nullptr,"X-XCurl-Request: original-path");
    auto* e=curl_easy_init();require(e&&list,"method fixture init");
    std::string body;DebugTrace trace;Cursor upload;
    for(unsigned i=0;i<24579;++i)upload.text.push_back(static_cast<char>('a'+i%26));
    char error_buffer[256]{};
    configure(e,base+"/get",list,&body);
    require(!curl_easy_setopt(e,10010,static_cast<void*>(error_buffer)),"game error-buffer option");
    require(!curl_easy_setopt(e,41,1L)&&!curl_easy_setopt(e,42,1L),"verbose and include headers");
    require(!curl_easy_setopt(e,20094,&debug_callback)&&!curl_easy_setopt(e,10095,static_cast<void*>(&trace)),"debug function and userdata");
    auto run=[&]{
        require(!curl_multi_add_handle(m,e),"method add");pump(m,mock);
        int n=0;auto* msg=curl_multi_info_read(m,&n);
        std::int32_t status=0;
        require(msg&&msg->easy==e&&!msg->data.result&&!n,"method completion");
        require(!curl_easy_getinfo(e,0x200002,&status)&&status==200,"method HTTP status");
        require(!curl_multi_remove_handle(m,e)&&!curl_multi_info_read(m,&n),"method remove");
    };
    require(!curl_easy_setopt(e,44,1L),"HEAD selection");run();
    require(body.starts_with("HTTP/1.1 200")&&body.find("X-XCurl-Test: native")!=std::string::npos&&body.find("xcurl-native-get")==std::string::npos,"HEAD header-only body callback");
    require(trace.headers==body&&trace.received.empty()&&trace.sent.empty(),"HEAD debug callback");
    body.clear();trace.headers.clear();
    require(!curl_easy_setopt(e,42,0L),"disable header fallback");
    require(!curl_easy_setopt(e,46,1L)&&!curl_easy_setopt(e,14,static_cast<long>(upload.text.size())),"PUT method and input size");
    require(!curl_easy_setopt(e,20012,&reader)&&!curl_easy_setopt(e,10009,static_cast<void*>(&upload))&&!curl_easy_setopt(e,280,8192L),"PUT read callback");
    configure(e,base+"/put",list,&body);run();
    require(body==upload.text&&upload.at==upload.text.size(),"PUT exact body and read count");
    require(trace.received==body&&trace.sent==upload.text&&trace.headers.find("X-XCurl-Test: native")!=std::string::npos,"PUT exact debug bytes");
    body.clear();upload.at=0;trace.headers.clear();trace.received.clear();trace.sent.clear();
    configure(e,base+"/patch",list,&body);
    require(!curl_easy_setopt(e,10036,"PATCH"),"custom method option");run();
    require(body==upload.text&&upload.at==upload.text.size()&&trace.sent==upload.text&&trace.received==body,"custom PATCH exact upload and response");
    body.clear();trace.received.clear();trace.sent.clear();trace.headers.clear();
    configure(e,base+"/get",list,&body);
    require(!curl_easy_setopt(e,10036,static_cast<const char*>(nullptr))&&!curl_easy_setopt(e,80,1L),"explicit GET after custom method");run();
    require(body=="xcurl-native-get"&&trace.received==body&&trace.sent.empty(),"explicit GET after upload");
    curl_easy_cleanup(e);curl_slist_free_all(list);
    std::puts("XCurl Store methods PASS: HEAD, PUT, custom PATCH, debug data, header fallback");
}

// Additional real HTTP response and error paths. Each case must publish
// one completion, balance the scheduler and permit immediate easy reuse.
void response_edges(void* m,Mock& mock,unsigned port){
    auto base=std::string("http://127.0.0.1:")+std::to_string(port);
    auto* list=curl_slist_append(nullptr,"X-XCurl-Request: original-path");
    auto* e=curl_easy_init();require(e&&list,"response edge init");
    std::string body;
    char error_buffer[256]{};
    require(!curl_easy_setopt(e,10010,error_buffer),"error buffer option");
    auto run=[&](unsigned expected){
        require(!curl_multi_add_handle(m,e),"response edge add");pump(m,mock);
        int n=0;auto* msg=curl_multi_info_read(m,&n);
        if(!msg||msg->data.result!=static_cast<int>(expected)){std::int32_t hr=0;curl_easy_getinfo(e,0x200019,&hr);std::fprintf(stderr,"response edge expected %u got %d HRESULT %08x\n",expected,msg?msg->data.result:-1,static_cast<unsigned>(hr));}
        require(msg&&msg->easy==e&&msg->data.result==static_cast<int>(expected)&&!n,"response edge result");
        require(!curl_multi_remove_handle(m,e)&&!curl_multi_info_read(m,&n),"response edge retire");
        require(!get<std::uint32_t>(m,0x3c),"response edge scheduler drained");
    };
    configure(e,base+"/chunked",list,&body);run(0);
    std::string expected;for(unsigned i=0;i<32783;++i)expected.push_back(static_cast<char>('a'+i%26));
    require(body==expected&&!error_buffer[0],"chunked response exact bytes and no error text");
    body.clear();configure(e,base+"/redirect",list,&body);
    require(!curl_easy_setopt(e,52,1L),"follow redirect option");run(0);
    char* effective=nullptr;
    auto info_status=curl_easy_getinfo(e,0x100001,&effective);
    if(info_status||!effective||std::string(effective)!=base+"/get")std::fprintf(stderr,"redirect effective URL: %s (getinfo=%d)\n",effective?effective:"<null>",info_status);
    require(body=="xcurl-native-get"&&!info_status&&effective&&std::string(effective)==base+"/get","redirect effective URL");
    body.clear();configure(e,base+"/get",list,&body);
    auto short_write=+[](char*,std::size_t n,std::size_t k,void*)->std::size_t{return n*k-1;};
    require(!curl_easy_setopt(e,20011,short_write),"short body callback");run(23);
    std::int32_t hr=0;require(!curl_easy_getinfo(e,0x200019,&hr)&&static_cast<std::uint32_t>(hr)==0x800700eaU,"short write HRESULT");
    require(std::strstr(error_buffer,"Failed writing body in write callback")!=nullptr,"short write original error text");
    auto abort_write=+[](char*,std::size_t,std::size_t,void*)->std::size_t{return 0x10000001ULL;};
    configure(e,base+"/get",list,&body);require(!curl_easy_setopt(e,20079,abort_write),"abort header callback");run(23);
    require(!curl_easy_getinfo(e,0x200019,&hr)&&static_cast<std::uint32_t>(hr)==0x800704c7U,"header abort HRESULT");
    require(!std::strcmp(error_buffer,"Request paused/aborted from header callback"),"header abort original error text");
    require(!curl_easy_setopt(e,20079,static_cast<decltype(abort_write)>(nullptr)),"remove abort callback");
    configure(e,base+"/timeout",list,&body);require(!curl_easy_setopt(e,155,1000L),"short receive timeout");run(28);
    require(!curl_easy_getinfo(e,0x200019,&hr)&&static_cast<std::uint32_t>(hr)==0x80072ee2U,"timeout HRESULT");
    require(!std::strcmp(error_buffer,"The request has timed out! (hr=0x80072ee2)"),"timeout original error text");
    // No giant allocation or network upload: a DWORD overflow must fail
    // before WinHttpSendRequest can consume the deliberately tiny payload.
    configure(e,base+"/echo",list,&body);
    require(!curl_easy_setopt(e,10015,"x")&&!curl_easy_setopt(e,30120,std::int64_t(0x100000000ULL)),"oversize upload option");
    run(43);require(!curl_easy_getinfo(e,0x200019,&hr)&&static_cast<std::uint32_t>(hr)==0x80070057U,"oversize upload HRESULT");
    curl_easy_cleanup(e);curl_slist_free_all(list);
    std::puts("XCurl response edges PASS: chunked body, redirect, write abort, header abort, timeout, DWORD overflow");
}

// Each TLS server has a distinct certificate and the mock never accepts a
// remote endpoint. Schannel also retains its ordinary WinHTTP verification.
void tls_transfer(void* m,Mock& mock,unsigned port,bool valid,const char* selector=nullptr){
    auto* e=curl_easy_init();require(e,"TLS easy init");
    std::string body;auto* list=curl_slist_append(nullptr,"X-XCurl-Request: original-path");
    auto url=std::string("https://127.0.0.1:")+std::to_string(port)+"/get";
    configure(e,url,list,&body);
    if(selector)require(!curl_easy_setopt(e,10025,selector),"client certificate selector option");
    auto before=mock.accepted.load();
    require(!curl_multi_add_handle(m,e),"TLS add");pump(m,mock);
    int n=0;auto* msg=curl_multi_info_read(m,&n);
    require(msg&&msg->easy==e&&!n,"TLS completion");
    std::int32_t http=0,hr=0;
    require(!curl_easy_getinfo(e,0x200002,&http)&&!curl_easy_getinfo(e,0x200019,&hr),"TLS metadata");
    if(valid){
        require(!msg->data.result&&http==200&&body=="xcurl-native-get","trusted TLS response");
        require(mock.accepted>before,"Windows certificate chain and hostname accepted");
    }else{
        require(msg->data.result&&hr&&body.empty()&&mock.accepted==before,"untrusted TLS rejected");
    }
    require(!curl_multi_remove_handle(m,e),"TLS remove");require(!curl_multi_info_read(m,&n),"TLS message retired");
    curl_easy_cleanup(e);curl_slist_free_all(list);
    std::printf("XCurl TLS %s PASS: result %s, native Schannel, GDK fixture chain policy\n",selector?(valid?"mutual TLS":"missing client certificate"):(valid?"trusted":"untrusted"),valid?"success":"rejected");
}
}
int main(int argc,char** argv){
    require(argc==2||argc==5,"HTTP port or trusted, untrusted, mTLS ports and selector");auto port=static_cast<unsigned>(std::atoi(argv[1]));
    Mock mock;mock.tls=argc==5;GdkQueryServices gdk{query,&mock};ApiBinding api{};WinHttpHost host{nullptr,diag,nullptr};RequestServices request{};
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
    HandleBinding handles(api,easy,multi);GameApi facade(api,handles,transfers,&tasks);
    require(facade.install()&&!curl_global_init(3),"global init");
    auto* m=curl_multi_init();require(m,"multi init");
    if(mock.tls){tls_transfer(m,mock,port,true);tls_transfer(m,mock,static_cast<unsigned>(std::atoi(argv[2])),false);
        const auto mutual=static_cast<unsigned>(std::atoi(argv[3]));
        tls_transfer(m,mock,mutual,false,"CurrentUser\\MY\\0000000000000000000000000000000000000000");
        tls_transfer(m,mock,mutual,true,argv[4]);}
    else{for(int kind=0;kind<3;++kind)transfer(m,mock,port,kind);batch(m,mock,port);method_transfers(m,mock,port);response_edges(m,mock,port);}
    require(!curl_multi_cleanup(m),"multi cleanup");curl_global_cleanup();
    require(!api.global.engine&&!api.global.references&&!mock.queue_count&&mock.queries==mock.releases&&(mock.tls?(mock.starts==4&&!mock.reports&&mock.verifications>0):(mock.starts==34&&mock.reports>=34)),"balanced runtime lifetime");
    require(tasks.reservations_empty()&&facade.uninstall()&&engine.deactivate(),"deactivate engine");pipe.deactivate();urls.deactivate();
    std::puts("XCurl full C ABI loopback PASS: actual WinHTTP callbacks, GET/direct POST/streamed POST, modeled GDK only");
}
