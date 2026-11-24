// Original XCurl Store x64 SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Public 0xE460 and inner 0xCBF8 transcribed from the pinned instructions.
// Disabled ETW provider path only. Explicit callbacks for the unported
// 0xEAF8 option dispatcher and 0xA224 active-request diagnostic.
#include "easy_setopt.h"
#include <cstring>
using namespace t6_xcurl;
extern "C" std::uint32_t xcurl_research_setopt_locked(void* easy,int option,const std::uint64_t* args,
    Lock enter,Lock leave,OptionDispatch dispatch,ActiveRequestLog active_log,void* ctx) {
    auto* h=static_cast<std::uint8_t*>(easy);
    enter(h+0x10,ctx);
    enter(h+0x50,ctx);
    std::uint64_t request=0;
    std::memcpy(&request,h+0x310,sizeof(request));
    std::uint32_t status;
    if(!request)status=dispatch(h+0x78,option,args,ctx);
    else {
        active_log(easy,ctx);
        status=2;
    }
    leave(h+0x50,ctx);
    leave(h+0x10,ctx);
    return status;
}
extern "C" std::uint32_t xcurl_research_easy_setopt(void* easy,int option,std::uint64_t argument,
    ApiProfile* profile,void* profile_lock,Lock enter,Lock leave,Tick tick,
    OptionDispatch dispatch,ActiveRequestLog active_log,ErrorLog error,void* ctx) {
    constexpr unsigned api_id=7;
    xcurl_research_record_api(profile,api_id,profile_lock,enter,leave,tick,ctx);
    const auto status=easy?xcurl_research_setopt_locked(easy,option,&argument,enter,leave,dispatch,active_log,ctx):0x2bU;
    // Original suppresses general API diagnostics for success, unsupported
    // build feature (4), and unknown option (0x30), but not null handles.
    if(status!=0&&status!=4&&status!=0x30&&error)error(status,api_id,ctx);
    return status;
}
