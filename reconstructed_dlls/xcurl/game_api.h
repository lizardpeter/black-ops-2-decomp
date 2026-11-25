#pragma once
#include "handle_binding.h"
#include "transfer_binding.h"
#include "xcurl_leaf_api.h"
#include <set>
namespace t6_xcurl {
class TaskQueueBinding;
// Source-link facade for the 16 imports used by the three Store clients.
// Not a drop-in retail DLL. The application installs a fully composed owner
// before invoking any stateful entrypoint. Calls and owner lifetime must be
// serialized by the application; callbacks may query a live easy on the same
// thread but must not destroy handles or uninstall the owner.
class GameApi {
public:
    GameApi(ApiBinding&,HandleBinding&,TransferBinding&,TaskQueueBinding* reservations=nullptr);
    GameApi(const GameApi&)=delete;
    GameApi& operator=(const GameApi&)=delete;
    bool install();
    bool uninstall();
    std::uint32_t global_init();
    void global_cleanup();
    void* easy_init();
    void easy_cleanup(void*);
    std::uint32_t setopt(void*,int,std::uint64_t);
    std::uint32_t getinfo(void*,int,void*);
    void* multi_init();
    std::uint32_t multi_cleanup(void*);
    std::uint32_t multi_add(void*,void*);
    std::uint32_t multi_remove(void*,void*);
    std::uint32_t multi_perform(void*,int*);
    std::uint32_t multi_poll(void*,int,int*);
    void* multi_info(void*,int*);
private:
    ApiBinding& api_;
    HandleBinding& handles_;
    TransferBinding& transfers_;
    TaskQueueBinding* reservations_;
    std::set<void*> easies_,multis_;
    std::uint32_t record(std::uint32_t,std::uint32_t);
    void profile(std::uint32_t);
};
struct GameMessage {
    std::int32_t type;
    void* easy;
    union {void* whatever;std::int32_t result;} data;
};
static_assert(sizeof(GameMessage)==24);
}
extern "C" {
int curl_global_init(long);
void curl_global_cleanup();
void* curl_easy_init();
void curl_easy_cleanup(void*);
int curl_easy_setopt(void*,int,...);
int curl_easy_getinfo(void*,int,...);
void* curl_multi_init();
int curl_multi_cleanup(void*);
int curl_multi_add_handle(void*,void*);
int curl_multi_remove_handle(void*,void*);
int curl_multi_perform(void*,int*);
int curl_multi_poll(void*,void*,unsigned,int,int*);
t6_xcurl::GameMessage* curl_multi_info_read(void*,int*);
}
