// C-callable test/integration entry point for the source-only coordinator.
// NOT the vendor BinkDoFrame export or original BINK handle ABI.
#include "video_registered.h"

extern "C" int bink_research_decode_registered_packet(
    const std::uint8_t* packet,std::uint32_t bytes,std::uint32_t flags,
    const t6_bink::RegisteredFrameBuffers* info,
    t6_bink::RegisteredDecodeReport* report) {
    if(!report)return -1;
    if(!packet || !info) {
        *report=t6_bink::RegisteredDecodeReport{};
        report->status=1;
        return 1;
    }
    return t6_bink::decode_registered_packet({packet,bytes},flags,*info,*report);
}
