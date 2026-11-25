#include "winhttp_services.h"
#ifdef _WIN32
#include "allocation.h"
#include "debug_data.h"
#include "diagnostic_text.h"
#include <cstdio>
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
void diagnostic(void* e,std::uint32_t site,std::uint32_t status,void* ctx){winhttp_report_diagnostic(e,site,status,0,0,status,static_cast<WinHttpHost*>(ctx));}
void debug_output(const char* text,void*){::OutputDebugStringA(text);}
void debug_write(const void* p,std::uint64_t n,std::uint64_t k,void* stream,void*){std::fwrite(p,static_cast<std::size_t>(n),static_cast<std::size_t>(k),static_cast<std::FILE*>(stream));}
void debug_data(void* e,std::uint32_t type,const void* p,std::uint64_t n,void* c){
    const DebugServices services{debug_output,debug_write,c};
    (void)xcurl_research_debug_data(e,static_cast<int>(type),p,n,&services);
}
void retry(void* e,void* ctx){diagnostic(e,0x21520,0,ctx);}
void* session(const char16_t* agent,std::uint32_t flags,void*){// Original checks last-error after WinHttpOpen even on success.
    // Clear stale error 87 before the call to prevent an unnecessary second session.
    ::SetLastError(ERROR_SUCCESS);return ::WinHttpOpen(w(agent),4,nullptr,nullptr,flags);}
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
// Original diagnostic strings recovered from XCurl .rdata; 0xA224/0xA32C.
// Numeric observer remains separate from game-visible text publication.
void winhttp_report_diagnostic(void* e,std::uint32_t site,std::uint64_t a,std::uint64_t b,std::uint32_t status,std::uint32_t numeric,WinHttpHost* host){
    if(e){
        char buf[4096]{};const char* format=nullptr;bool info=false;
        auto x=static_cast<unsigned>(a);auto u=static_cast<unsigned long long>(a);auto v=static_cast<unsigned long long>(b);
        switch(site){
        case 0x20710:format="Failed to create XTaskQueue! (hr=0x%08x)";break;
        case 0x20740:format="Failed to create event! (hr=0x%08x)";break;
        case 0x20810:format="Failed get URL (result=0x%08x)!";break;
        case 0x20848:format="Failed to get port from URL (result=0x%08x)!";break;
        case 0x20920:format="Failed to queue completion callback! (hr=0x%08x)";break;
        case 0x21060:format="XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize failed! (result=0x%08x)";break;
        case 0x21168:format="LoadClientCert failed! (hr=0x%08x)";break;
        case 0x21200:format="Failed to prevent Https to Http redirects! (result=0x%08x)";break;
        case 0x21240:format="Failed to set automatic redirects! (result=0x%08x)";break;
        case 0x21278:format="Failed to set accept encoding! (result=0x%08x)";break;
        case 0x21308:format="Failed to disable automatic cookie support. (result=0x%08x)";break;
        case 0x21348:format="Failed to set http version! (result=0x%08x)";break;
        case 0x21418:format="Curl_easy::SendRequest failed to send request! (result=0x%08x)";break;
        case 0x21460:format="Failed to write header to buffer (hr=%08X)";break;
        case 0x21610:format="Failed to create HTTP connection! (result=0x%08x)";break;
        case 0x21648:format="Failed to set WinHttp Callback! (hr=0x%08x)";break;
        case 0x21678:format="Failed to set request context! (hr=0x%08x)";break;
        case 0x21728:format="WinHttpQueryHeaders failed: (hr=0x%08x)";break;
        case 0x21750:format="Failed to read status code: (hr=0x%08x)";break;
        case 0x21810:format="Failed to queue the next chunk send (hr=0x%08x)";break;
        case 0x21868:format="WinHttpQueryDataAvailable failed: (hr=0x%08x)";break;
        case 0x21898:format="Failed to queue headers callback (hr=0x%08x)";break;
        case 0x21940:format="Failed to queue read complete (hr=0x%08x)";break;
        case 0x207b8:format="Encountered error updating URL (hr=0x%08x)!";break;
        case 0x208e8:format="WinHttpReadData failed to queue data read. (hr=0x%08x)";break;
        case 0x20e10:format="WinHttpCrackUrl failed on Curl validated URL (hr = 0x%08x)!";break;
        case 0x20e78:format="Failed to build URL with scheme prefix (result = 0x%08x)!";break;
        case 0x20eb8:format="Failed to set URL with https scheme (result = 0x%08x)!";break;
        case 0x20ef0:format="Failed to set https scheme (result = 0x%08x)!";break;
        case 0x20f20:format="URL is invalid (result = 0x%08x)!";break;
        case 0x20f80:format="Failed to call XNetworkingQuerySecurityInformationForUrlUtf16Async! (result=0x%08x)";break;
        case 0x20fe0:format="XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult failed! (result=0x%08x)";break;
        case 0x210b8:format="Failed to retrieve XBL security information! (result=0x%08x)";break;
        case 0x211c8:format="Failed to disable automatic redirects! (result=0x%08x)";break;
        case 0x212a8:format="Failed to set automatic timeouts! (result=0x%08x)";break;
        case 0x212e0:format="Failed to exempt CA. (result=0x%08x)";break;
        case 0x213b0:format="WinHttpWriteData failed to send data. (hr=0x%08x)";break;
        case 0x213e8:format="Failed to allocate send buffer. (hr=0x%08x)";break;
        case 0x215e0:format="Failed to create request! (result=0x%08x)";break;
        case 0x216f8:format="Failed to retrieve redirect URL (result=0x%08x)";break;
        case 0x217a8:format="Failed to validate security information. (hr=0x%08x)";break;
        case 0x217e0:format="Failed to start receiving response (hr=0x%08x)";break;
        case 0x218c8:format="More data sent than expected. (hr=0x%08x)";break;
        case 0x219d8:format="The request has timed out! (hr=0x%08x)";break;
        case 0x21a00:format="Too many redirects! (hr=0x%08x)";break;
        case 0x21a20:format="Server reset the connection without sending any data! (hr=0x%08x)";break;
        case 0x21a68:format="Failed to read data! (hr=0x%08x)";break;
        case 0x21a90:format="Failed to write data! (hr=0x%08x)";break;
        case 0x21ab8:format="Failed to send request! (hr=0x%08x)";break;
        case 0x21ae0:format="Unknown winhttp failure! (hr=0x%08x)";break;
        case 0x21b90:format="Failed to set security options! (hr=0x%08x)";break;
        case 0x21bf0:format="Failed to create http session! (hr=0x%08x)";break;
        case 0x20878:std::snprintf(buf,sizeof(buf),"%s","Request paused/aborted from header callback");break;
        case 0x20958:std::snprintf(buf,sizeof(buf),"%s","Request paused/aborted in write callback");break;
        case 0x21038:std::snprintf(buf,sizeof(buf),"%s","Failed to allocate XCurlSecurityBuffer!");break;
        case 0x20a08:std::snprintf(buf,sizeof(buf),"%s","Aborted by Read Callback.");break;
        case 0x20d38:std::snprintf(buf,sizeof(buf),"%s","Could not allocate buffer for XErrorReport message.");break;
        case 0x20d70:std::snprintf(buf,sizeof(buf),"%s","HTTP and HTTPS are the only schemes supported by XCurl.");break;
        case 0x20de8:std::snprintf(buf,sizeof(buf),"%s","Could not allocate buffer for URL");break;
        case 0x20f48:std::snprintf(buf,sizeof(buf),"%s","Failed to create CURLU!");break;
        case 0x214c0:std::snprintf(buf,sizeof(buf),"%s","Could not allocate buffer for trailing headers");break;
        case 0x214f0:std::snprintf(buf,sizeof(buf),"%s","Request aborted by trailer callback");break;
        case 0x208a8:std::snprintf(buf,sizeof(buf),"Failed writing body from header callback (%llu != %llu)",u,v);break;
        case 0x20988:std::snprintf(buf,sizeof(buf),"Failed writing body in write callback (%llu != %llu)",u,v);break;
        case 0x209c0:std::snprintf(buf,sizeof(buf),"Failed to resize send buffer previous size: %llu, new size: %llu",u,v);break;
        case 0x21378:std::snprintf(buf,sizeof(buf),"Caller overflowed send buffer (%llu > %llu)!",u,v);break;
        case 0x21778:std::snprintf(buf,sizeof(buf),"Failed to allocate send buffer size: %llu",u);break;
        case 0x21840:std::snprintf(buf,sizeof(buf),"The requested URL returned error: %d",static_cast<int>(x));break;
        case 0x21900:std::snprintf(buf,sizeof(buf),"Less data sent than expected. %llu vs %llu (Win32 error=0x%08x)",u,v,status);break;
        case 0x21970:std::snprintf(buf,sizeof(buf),"Less data sent than expected. %llu vs %llu (hr=0x%08x)",u,v,status);break;
        case 0x207e8:std::snprintf(buf,sizeof(buf),"Failed to validate URL %s (hr=0x%08x)!",a?reinterpret_cast<const char*>(a):"",status);break;
        case 0x20e50:std::snprintf(buf,sizeof(buf),"Failed to widen URL %s (result=0x%08x)!",a?reinterpret_cast<const char*>(a):"",status);break;
        case 0x20da8:std::snprintf(buf,sizeof(buf),"Failed to parse URL, likely malformed! %ls (result=0x%08x)",a?reinterpret_cast<const wchar_t*>(a):L"",status);break;
        case 0x216d0:std::snprintf(buf,sizeof(buf),"Invalid effective URL (%ls, hr=0x%08x)",a?reinterpret_cast<const wchar_t*>(a):L"",status);break;
        case 0x219a8:std::snprintf(buf,sizeof(buf),"Failed to allocate redirect URL (url=%ls)",a?reinterpret_cast<const wchar_t*>(a):L"");break;
        case 0x20830:info=true;format="Setting port to %u";break;
        case 0x20d10:info=true;std::snprintf(buf,sizeof(buf),"WARNING: Insecure HTTP request \"%ls\"",a?reinterpret_cast<const wchar_t*>(a):L"");break;
        case 0x20f60:info=true;std::snprintf(buf,sizeof(buf),"Final request URL in XCurl: %ls",a?reinterpret_cast<const wchar_t*>(a):L"");break;
        case 0x21520:info=true;std::snprintf(buf,sizeof(buf),"%s","WARNING: HTTP/2 streaming requests are not available on Windows 10. The whole request body will be sent at once with a Content-Length header. Set CURLOPT_INFILESIZE to prevent this warning.");break;
        case 0x21490:info=true;std::snprintf(buf,sizeof(buf),"%s","Sending terminating chunk with trailing headers");break;
        default:break;
        }
        if(format)std::snprintf(buf,sizeof(buf),format,x);
        if(buf[0]){if(info)publish_info_text(e,buf,false,debug_data,host);else publish_error_text(e,buf,debug_data,host);}
    }
    if(host&&host->diagnostic)host->diagnostic(e,site,numeric,host->diagnostic_context);
}
SessionServices winhttp_session_services(WinHttpHost* host){return {enter,leave,alloc,release,session,option,error,close,diagnostic,vtable,host};}
HttpSetupServices winhttp_setup_services(WinHttpHost* host){return {connect,open,option,timeouts,error,diagnostic,retry,host};}
HeaderQueryServices winhttp_header_query_services(WinHttpHost* host){return {alloc,release,query_headers,error,convert_utf8,host};}
AddHeaderServices winhttp_add_header_services(const RequestServices* s){return {s,add_headers};}
UploadServices winhttp_upload_services(const RequestServices* s,std::uint32_t(*trailers)(void*,std::uint64_t*,void*),void(*diag)(void*,std::uint32_t,std::uint64_t,std::uint64_t,void*)){return {s,write,trailers,diag};}
void bind_winhttp_request_services(RequestServices& s,WinHttpHost* host){
    s.enter=enter;s.leave=leave;s.allocate=alloc;s.release=release;s.set_event=event;s.close_http=close;s.release_connection=release_session;
    s.create_event=create_event;s.last_error=error;s.diagnostic=diagnostic;s.read=read;s.receive=receive;s.send=send;s.set_context=set_context;s.set_callback=set_callback;s.debug_data=debug_data;s.context=host;
    if(host->tasks){s.create_queue=create_queue;s.queue_dispatch=dispatch_queue;s.cancel_task=cancel_task;s.submit_completion=submit_completion;s.adjust_global=adjust_global;}
}
MultiHostServices win32_multi_host_services(void* context){return {wait_one,wait_many,release_mutex,reset_event,context};}
WINHTTP_STATUS_CALLBACK win32_reconstructed_status_callback(){return reconstructed_status;}
EasyLifecycleServices win32_easy_lifecycle_services(const RequestServices* r,const GdkQueryServices* g){return {r,g,delete_cs,close_handle,nullptr};}
MultiLifecycleServices win32_multi_lifecycle_services(const SchedulerServices* s,const RequestServices* r){return {s,r,create_mutex,create_multi_event,close_handle,release_mutex,error,nullptr};}
}
#endif
