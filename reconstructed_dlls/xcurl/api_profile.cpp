// Original XCurl Store x64 SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Original public curl_easy_getinfo 0xDFC0, API accounting 0x88A0, inner
// field dispatch 0xAD58. Disabled ETW provider path. Locks, clock and
// formatted error reporting are explicit boundaries, not curl wrappers.
#include "api_profile.h"
#include <cstddef>
using namespace t6_xcurl;
extern "C" void xcurl_research_record_api(ApiProfile* state,std::uint32_t id,
    void* profile_lock,Lock enter,Lock leave,Tick tick,void* ctx) {
    enter(profile_lock,ctx);
    const auto now=tick(ctx);
    if(!state->first_tick)state->first_tick=now;
    ++state->counts[id];
    leave(profile_lock,ctx);
}
extern "C" std::uint32_t xcurl_research_easy_getinfo(const void* easy,int option,void* out,
    ApiProfile* state,void* profile_lock,Lock enter,Lock leave,Tick tick,
    ErrorLog log,void* ctx) {
    constexpr unsigned api_id=10;
    xcurl_research_record_api(state,api_id,profile_lock,enter,leave,tick,ctx);
    std::uint32_t result=0x30;
    if(easy) {
        // Original lock at easy+0x188 spans the option switch and output write.
        auto* lock=const_cast<std::uint8_t*>(static_cast<const std::uint8_t*>(easy))+0x188;
        enter(lock,ctx);
        result=xcurl_easy_getinfo_unlocked(easy,option,&out);
        leave(lock,ctx);
    }
    if(result&&log)log(result,api_id,ctx);
    return result;
}
