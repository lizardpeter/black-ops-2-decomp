// External consumer of the production owner, not a mock GDK integration.
// No network requests are sent. Explicitly balance the original failed-init
// reference before a retry. CI expects a clean failure on hosts without GDK.
#include "win32_runtime.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
bool import_probe_before_install();
namespace {
unsigned errors=0;
void diag(void*,std::uint32_t site,std::uint32_t status,void*){std::printf("runtime diagnostic %x %x\n",site,status);}
void error(std::uint32_t status,std::uint32_t api,void*){++errors;std::printf("API %u status %u\n",api,status);}
bool check(bool ok,const char* label){if(!ok)std::fprintf(stderr,"FAIL: %s\n",label);return ok;}
int run(bool require_live){
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
int main(int argc,char** argv){return run(argc==2&&std::strcmp(argv[1],"--require-live-gdk")==0);}
