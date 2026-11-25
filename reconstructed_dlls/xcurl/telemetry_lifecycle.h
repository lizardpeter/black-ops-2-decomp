#pragma once
#include "option_dispatch.h"
#include <cstdint>
namespace t6_xcurl {
// Object/list layout recovered from 7A30/7DF4/7C50/8018/7CF8.
// Event-provider registration and payload publication are separate services.
struct TelemetryServices {
    OptionAllocate allocate;
    OptionFree release;
    std::uint32_t(*tick32)(void*);
    void* context;
};
inline constexpr std::size_t telemetry_bytes=0x40;
}
extern "C" {
void* xcurl_research_telemetry_construct(void*,const t6_xcurl::TelemetryServices*);
std::uint32_t xcurl_research_telemetry_create(void**,const t6_xcurl::TelemetryServices*);
void xcurl_research_telemetry_unlink(void*);
void xcurl_research_telemetry_destroy_records(void*,const t6_xcurl::TelemetryServices*);
void xcurl_research_telemetry_destroy_local(void*,const t6_xcurl::TelemetryServices*);
void xcurl_research_telemetry_format_guid(const std::uint8_t*,char16_t*);
}
