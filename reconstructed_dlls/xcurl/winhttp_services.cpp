#include "winhttp_services.h"
#ifdef _WIN32
#include "allocation.h"
namespace t6_xcurl {
namespace {
static_assert(sizeof(wchar_t)==sizeof(char16_t));
LPCWSTR w(const char16_t* p){return reinterpret_cast<LPCWSTR>(p);}
void enter(void* p,void*){::EnterCriticalSection(static_cast<CRITICAL_SECTION*>(p));}
void leave(void* p,void*){::LeaveCriticalSection(static_cast<CRITICAL_SECTION*>(p));}
void* alloc(std::size_t n,std::uint32_t tag,void*){return xcurl_research_heap_allocate(n,tag);}
void release(void* p,std::uint32_t tag,void*){xcurl_research_heap_free(p,tag);}
std::uint32_t error(void*){return ::GetLastError();}
void close(void* h,void*){::WinHttpCloseHandle(h);}
void event(void* h,void*){::SetEvent(h);}
void* create_event(void*){return ::CreateEventW(nullptr,FALSE,FALSE,nullptr);}
void diagnostic(void* e,std::uint32_t site,std::uint32_t status,void* ctx){auto& h=*static_cast<WinHttpHost*>(ctx);if(h.diagnostic)h.diagnostic(e,site,status,h.diagnostic_context);}
void retry(void* e,void* ctx){diagnostic(e,0x21520,0,ctx);}
void* session(const char16_t* agent,std::uint32_t flags,void*){return ::WinHttpOpen(w(agent),4,nullptr,nullptr,flags);}
void* connect(void* h,const char16_t* hostname,std::uint16_t port,void*){return ::WinHttpConnect(h,w(hostname),port,0);}
void* open(void* h,const char16_t* method,const char16_t* path,const char16_t* const* accepts,std::uint32_t flags,void*){
    return ::WinHttpOpenRequest(h,w(method),w(path),nullptr,nullptr,reinterpret_cast<LPCWSTR*>(const_cast<const char16_t**>(accepts)),flags);
}
std::uint32_t option(void* h,std::uint32_t k,std::uint32_t v,void*){return ::WinHttpSetOption(h,k,&v,sizeof(v));}
std::uint32_t timeouts(void* h,std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d,void*){return ::WinHttpSetTimeouts(h,static_cast<int>(a),static_cast<int>(b),static_cast<int>(c),static_cast<int>(d));}
std::uint32_t read(void* h,void* p,std::uint32_t n,void*){return ::WinHttpReadData(h,p,n,nullptr);}
std::uint32_t receive(void* h,void*){return ::WinHttpReceiveResponse(h,nullptr);}
std::uint32_t send(void* h,const void* p,std::uint32_t n,std::uint32_t total,void* context,void*){
    return ::WinHttpSendRequest(h,nullptr,0,const_cast<void*>(p),n,total,reinterpret_cast<DWORD_PTR>(context));
}
std::uint32_t set_context(void* h,void* context,void*){auto v=reinterpret_cast<DWORD_PTR>(context);return ::WinHttpSetOption(h,WINHTTP_OPTION_CONTEXT_VALUE,&v,sizeof(v));}
std::uintptr_t set_callback(void* h,std::uint32_t flags,void* context){return reinterpret_cast<std::uintptr_t>(::WinHttpSetStatusCallback(h,static_cast<WinHttpHost*>(context)->status_callback,flags,0));}
std::uint32_t query_headers(void* h,std::uint32_t level,void* buffer,std::uint32_t* bytes,void*) {
    DWORD n=*bytes;auto ok=::WinHttpQueryHeaders(h,level,WINHTTP_HEADER_NAME_BY_INDEX,buffer,&n,WINHTTP_NO_HEADER_INDEX);*bytes=n;return ok;
}
int convert_utf8(const char16_t* p,int units,char* out,int bytes,void*) {
    return ::WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w(p),units,out,bytes,nullptr,nullptr);
}
std::uint32_t add_headers(void* h,const char16_t* value,std::uint32_t n,std::uint32_t flags,void*) {
    return ::WinHttpAddRequestHeaders(h,w(value),n,flags);
}
std::uint32_t write(void* h,const void* p,std::uint32_t n,void*) {
    return ::WinHttpWriteData(h,p,n,nullptr);
}
std::uint32_t native_addref(void* p){return xcurl_research_session_addref(p);}
std::uint32_t native_release(void* p);
using RefFn=std::uint32_t(*)(void*);
const RefFn vtable[]{native_addref,native_release};
std::uint32_t native_release(void* p){auto services=winhttp_session_services(nullptr);return xcurl_research_session_release(p,&services);}
void release_session(void* p,void*){native_release(p);}
TaskQueueBinding& tasks(void* c){return *static_cast<WinHttpHost*>(c)->tasks;}
std::uint32_t create_queue(void** q,void* c){return tasks(c).create(q);}
std::uint8_t dispatch_queue(void* q,std::uint32_t p,std::uint32_t t,void* c){return tasks(c).dispatch(q,p,t);}
void cancel_task(void* p,void* c){tasks(c).cancel(p);}
std::uint32_t submit_completion(void* q,void* e,void* c){return tasks(c).submit_completion(q,e);}
void adjust_global(std::uint32_t n,void* c){tasks(c).adjust_global(n);}
std::uint32_t wait_one(void* h,std::uint32_t t,void*){return ::WaitForSingleObject(h,t);}
std::uint32_t wait_many(std::uint32_t n,const void* p,std::uint32_t t,void*){return ::WaitForMultipleObjects(n,static_cast<const HANDLE*>(p),FALSE,t);}
void reset_event(void* h,void*){::ResetEvent(h);}
void release_mutex(void* h,void*){::ReleaseMutex(h);}
void delete_cs(void* p,void*){::DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(p));}
void close_handle(void* h,void*){::CloseHandle(h);}
void* create_mutex(void*){return ::CreateMutexW(nullptr,FALSE,nullptr);}
void* create_multi_event(std::uint8_t manual,std::uint8_t initial,void*){return ::CreateEventW(nullptr,manual,initial,nullptr);}
void CALLBACK reconstructed_status(HINTERNET,DWORD_PTR context,DWORD status,LPVOID info,DWORD length){CallbackBinding::status_entry(context,status,info,length);}
}
SessionServices winhttp_session_services(WinHttpHost* host){return {enter,leave,alloc,release,session,option,error,close,diagnostic,vtable,host};}
HttpSetupServices winhttp_setup_services(WinHttpHost* host){return {connect,open,option,timeouts,error,diagnostic,retry,host};}
HeaderQueryServices winhttp_header_query_services(WinHttpHost* host){return {alloc,release,query_headers,error,convert_utf8,host};}
AddHeaderServices winhttp_add_header_services(const RequestServices* s){return {s,add_headers};}
UploadServices winhttp_upload_services(const RequestServices* s,std::uint32_t(*trailers)(void*,std::uint64_t*,void*),void(*diag)(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*)){return {s,write,trailers,diag};}
void bind_winhttp_request_services(RequestServices& s,WinHttpHost* host){
    s.enter=enter;s.leave=leave;s.allocate=alloc;s.release=release;s.set_event=event;s.close_http=close;s.release_connection=release_session;
    s.create_event=create_event;s.last_error=error;s.diagnostic=diagnostic;s.read=read;s.receive=receive;s.send=send;s.set_context=set_context;s.set_callback=set_callback;s.context=host;
    if(host->tasks){s.create_queue=create_queue;s.queue_dispatch=dispatch_queue;s.cancel_task=cancel_task;s.submit_completion=submit_completion;s.adjust_global=adjust_global;}
}
MultiHostServices win32_multi_host_services(void* context){return {wait_one,wait_many,release_mutex,reset_event,context};}
WINHTTP_STATUS_CALLBACK win32_reconstructed_status_callback(){return reconstructed_status;}
EasyLifecycleServices win32_easy_lifecycle_services(const RequestServices* r,const GdkQueryServices* g){return {r,g,delete_cs,close_handle,nullptr};}
MultiLifecycleServices win32_multi_lifecycle_services(const SchedulerServices* s,const RequestServices* r){return {s,r,create_mutex,create_multi_event,close_handle,release_mutex,error,nullptr};}
}
#endif
