// Separate translation unit intentionally compiled with dllimport declarations.
// The __imp_ slots must resolve inside the static archive, not a vendor DLL.
#include <cstdint>
#include <cstring>
struct curl_slist {char* data;curl_slist* next;};
struct GameMessage;
extern "C" {
__declspec(dllimport) int curl_global_init(long);
__declspec(dllimport) void curl_global_cleanup();
__declspec(dllimport) void* curl_easy_init();
__declspec(dllimport) void curl_easy_cleanup(void*);
__declspec(dllimport) int curl_easy_setopt(void*,int,...);
__declspec(dllimport) int curl_easy_getinfo(void*,int,...);
__declspec(dllimport) const char* curl_easy_strerror(int);
__declspec(dllimport) curl_slist* curl_slist_append(curl_slist*,const char*);
__declspec(dllimport) void curl_slist_free_all(curl_slist*);
__declspec(dllimport) void* curl_multi_init();
__declspec(dllimport) int curl_multi_cleanup(void*);
__declspec(dllimport) int curl_multi_add_handle(void*,void*);
__declspec(dllimport) int curl_multi_remove_handle(void*,void*);
__declspec(dllimport) int curl_multi_perform(void*,int*);
__declspec(dllimport) int curl_multi_poll(void*,void*,unsigned,int,int*);
__declspec(dllimport) GameMessage* curl_multi_info_read(void*,int*);
}
bool import_probe_before_install(){
    // Run all sixteen import-shaped calls with no active runtime. This is a
    // link and closed-owner check, not a mock of live GDK.
    int n=0;
    bool ok=curl_global_init(3)==2;
    curl_global_cleanup();
    ok&=curl_easy_init()==nullptr;
    curl_easy_cleanup(nullptr);
    ok&=curl_easy_setopt(nullptr,10002,"https://example.invalid/")==2;
    std::int32_t code=0;
    ok&=curl_easy_getinfo(nullptr,0x200002,&code)==2;
    ok&=std::strcmp(curl_easy_strerror(0),"No error")==0;
    auto* list=curl_slist_append(nullptr,"X-Static-Link: yes");
    ok&=list&&std::strcmp(list->data,"X-Static-Link: yes")==0;
    curl_slist_free_all(list);
    ok&=curl_multi_init()==nullptr;
    ok&=curl_multi_cleanup(nullptr)==1;
    ok&=curl_multi_add_handle(nullptr,nullptr)==1;
    ok&=curl_multi_remove_handle(nullptr,nullptr)==1;
    ok&=curl_multi_perform(nullptr,&n)==1;
    ok&=curl_multi_poll(nullptr,nullptr,0,0,&n)==1;
    ok&=curl_multi_info_read(nullptr,&n)==nullptr;
    return ok;
}
