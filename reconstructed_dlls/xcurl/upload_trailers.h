#pragma once
#include "request_engine.h"
namespace t6_xcurl {
// Native integration of original trailer formatter 0x10420.
// Owns and frees the slist supplied by the application's trailer callback.
std::uint32_t bound_append_upload_trailers(void*,std::uint64_t*,const RequestServices*);
}
