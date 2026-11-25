#pragma once
#include "request_engine.h"
namespace t6_xcurl {
// Recovered A224/A32C publication semantics for a preformatted message.
void publish_error_text(void* easy,const char* text,DebugData debug,void* context);
void publish_info_text(void* easy,const char* text,bool format_ends_newline,DebugData debug,void* context);
}
