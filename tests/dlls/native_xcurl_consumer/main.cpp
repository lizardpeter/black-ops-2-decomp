// External consumer of the production owner, not a mock GDK integration.
// Default mode sends no network requests; optional loopback requires genuine GDK. Explicitly balance the original failed-init
// reference before a retry. CI expects a clean failure on hosts without GDK.
#include "win32_runtime.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <charconv>
#include <chrono>
#include <string>
bool import_probe_before_install();
namespace {
unsigned errors=0;
void diag(void*,std::uint32_t site,std::uint32_t status,void*){std::printf("runtime diagnostic %x %x\n",site,status);}
void error(std::uint32_t status,std::uint32_t api,void*){++errors;std::printf("API %u status %u\n",api,status);}
bool check(bool ok,const char* label){if(!ok)std::fprintf(stderr,"FAIL: %s\n",label);return ok;}
// Optional acceptance on a machine with genuine Gaming Services. No mock
// query interface, security metadata, certificate or network callbacks.
bool loopback(void* easy,void* multi,unsigned port){
    std::string url="http://127.0.0.1:"+std::to_string(port)+"/get",body;
    char error_buffer[256]{};
    auto writer=+[](char* data,std::size_t size,std::size_t count,void* ctx)->std::size_t{
        auto& out=*static_cast<std::string*>(ctx);
        if(size&&count>SIZE_MAX/size)return 0;
        auto n=size*count;if(n>65536||out.size()>65536-n)return 0;
        try{out.append(data,n);}catch(...){return 0;}return n;
    };
    if(!check(!curl_easy_setopt(easy,10002,url.c_str())&&!curl_easy_setopt(easy,155,5000L)&&
              !curl_easy_setopt(easy,84,1L)&&!curl_easy_setopt(easy,10010,static_cast<void*>(error_buffer))&&
              !curl_easy_setopt(easy,20011,writer)&&!curl_easy_setopt(easy,10001,static_cast<void*>(&body)),
              "live loopback options"))return false;
    if(!check(!curl_multi_add_handle(multi,easy),"live loopback admission"))return false;
    bool ok=true;int running=1,remaining=0;const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    do{
        if(curl_multi_perform(multi,&running)){ok=false;break;}
        if(!running)break;
        int events=0;if(curl_multi_poll(multi,nullptr,0,100,&events)){ok=false;break;}
        if(std::chrono::steady_clock::now()>=until){ok=false;break;}
    }while(running);
    auto* msg=ok?curl_multi_info_read(multi,&remaining):nullptr;
    std::int32_t http=0,hr=0;curl_easy_getinfo(easy,0x200002,&http);curl_easy_getinfo(easy,0x200019,&hr);
    ok=check(ok&&msg&&msg->type==1&&msg->easy==easy&&!msg->data.result&&!remaining&&http==200&&body=="xcurl-live-gdk-loopback","live GDK loopback transfer");
    if(!ok)std::fprintf(stderr,"live transfer result=%d HRESULT=%08x HTTP=%d body=%zu diagnostic=%s\n",msg?msg->data.result:-1,static_cast<unsigned>(hr),http,body.size(),error_buffer);
    ok&=check(!curl_multi_remove_handle(multi,easy),"live loopback drain and detach");
    ok&=check(!curl_multi_info_read(multi,&remaining)&&!remaining,"live loopback message retired");
    if(ok)std::puts("XCurl live GDK loopback PASS");
    return ok;
}

int run(bool require_live,unsigned port){
    if(!check(import_probe_before_install(),"all sixteen static dllimport slots"))return 1;
    std::puts("XCurl static dllimport ABI PASS: 16/16");
    t6_xcurl::Win32RuntimeConfig c{};c.diagnostic=diag;c.api_error=error;
    t6_xcurl::Win32Runtime runtime(c);
    if(!check(runtime.install()&&!runtime.references()&&!runtime.engine_ready(),"install production owner"))return 1;
    for(unsigned i=0;i<2;++i){
        auto status=curl_global_init(3);
        if(status){
            if(!check(runtime.references()==1&&!runtime.engine_ready()&&!runtime.uninstall(),"failed init retained original reference"))return 1;
            if(!check(curl_global_init(3)!=0&&runtime.references()==2,"repeated failed init cannot advertise usable engine"))return 1;
            if(!check(!curl_easy_init()&&!curl_multi_init(),"failed runtime blocks handles"))return 1;
            curl_global_cleanup();curl_global_cleanup();
            if(!check(!runtime.references()&&!runtime.engine_ready(),"failed initialization cleanup"))return 1;
            auto report=runtime.startup_report();
            if(!check(report.stage==t6_xcurl::BootstrapStage::runtime||report.stage==t6_xcurl::BootstrapStage::engine,"startup failure stage recorded"))return 1;
            std::printf("GDK live initialization unavailable: %d (attempt %u), stage %u, original status 0x%08x\n",status,i+1,static_cast<unsigned>(report.stage),report.status);
            if(require_live)return 2;
            continue;
        }
        if(!check(runtime.engine_ready()&&runtime.references()==1&&runtime.startup_report().stage==t6_xcurl::BootstrapStage::ready,"live GDK engine created"))return 1;
        auto* easy=curl_easy_init();auto* multi=curl_multi_init();
        bool ok=check(easy&&multi,"live handle creation");
        auto* headers=curl_slist_append(nullptr,"X-XCurl-Consumer: native");
        if(easy){
            std::int32_t code=0;
            ok&=check(!curl_easy_setopt(easy,10002,"https://example.invalid/")&&!curl_easy_setopt(easy,10023,headers)&&!curl_easy_getinfo(easy,0x200002,&code),"live option and metadata access");
            if(multi){
                ok&=check(!curl_multi_add_handle(multi,easy)&&!curl_multi_remove_handle(multi,easy),"pending add/remove without networking");
                int n=0;
                ok&=check(!curl_multi_perform(multi,&n)&&!n&&!curl_multi_poll(multi,nullptr,0,0,&n)&&!curl_multi_info_read(multi,&n),"empty live multi");
                if(ok&&port&&i==0)ok=loopback(easy,multi,port);
            }
            curl_easy_cleanup(easy);
        }
        curl_slist_free_all(headers);
        if(multi)ok&=check(!curl_multi_cleanup(multi),"live multi cleanup");
        curl_global_cleanup();
        if(!check(!runtime.references()&&!runtime.engine_ready(),"live GDK teardown")||!ok)return 1;
        std::puts("GDK live engine and handle lifecycle PASS (no requests)");
    }
    if(!check(runtime.uninstall()&&!runtime.installed(),"uninstall production owner"))return 1;
    if(!check(runtime.install()&&runtime.uninstall(),"reinstall production owner"))return 1;
    if(!check(curl_global_init(3)==2&&!curl_easy_init()&&!curl_multi_init(),"uninstalled facade closed"))return 1;
    if(!check(curl_easy_strerror(0)&&std::strlen(curl_easy_strerror(0))>0,"linked error leaf"))return 1;
    std::printf("Independent XCurl consumer PASS; API diagnostic count %u\n",errors);return 0;
}
}
int main(int argc,char** argv){
    if(argc==1)return run(false,0);
    if(argc==2&&!std::strcmp(argv[1],"--require-live-gdk"))return run(true,0);
    if(argc==3&&!std::strcmp(argv[1],"--loopback-port")){
        unsigned port=0;auto end=argv[2]+std::strlen(argv[2]);auto result=std::from_chars(argv[2],end,port);
        if(result.ec==std::errc{}&&result.ptr==end&&port&&port<=65535)return run(true,port);
    }
    std::fprintf(stderr,"Usage: native_xcurl_consumer [--require-live-gdk | --loopback-port PORT]\n");return 64;
}
