#pragma once
#include "gdk_runtime.h"
#ifdef _WIN32
namespace t6_xcurl {
// Returns a process-wide QueryApiImpl adapter using the original Win32 loader,
// SRW lock, dynamic export table and Gaming Services initialization contract.
const GdkQueryServices* win32_gdk_query_services();
}
#endif
