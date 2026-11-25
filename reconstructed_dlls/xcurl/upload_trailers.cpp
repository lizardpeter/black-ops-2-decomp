// Original 0x10420: callback, list length sum, tagged request buffer,
// "0\\r\\n" + each trailer + CRLF + final CRLF, then free the list.
// Evidence run 45545280844 / 45556303149. Native guards prevent integer
// overflow and stale-buffer use; the original formatting path did not.
#include "upload_trailers.h"
#include "xcurl_leaf_api.h"
#include <cstring>
#include <memory>
namespace t6_xcurl {
namespace {
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const char*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<char*>(p)+o,&v,sizeof(v));}
using Callback=int(*)(curl_slist**,void*);
}
extern "C" std::uint32_t bound_append_upload_trailers(void* req,std::uint64_t* out,const RequestServices* s){
    auto* easy=get<void*>(req,0x30);auto c=s->context;
    auto cb=get<Callback>(easy,0x140);
    *out=0;
    auto fail=[&](std::uint32_t hr,unsigned code,unsigned site){s->diagnostic(easy,site,hr,c);xcurl_research_easy_cancel(easy,hr,code,s);return hr;};
    if(!cb)return fail(0x80070057U,43,0x214f0);
    curl_slist* list=nullptr;
    int code=cb(&list,get<void*>(easy,0x138));
    std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)> owned(list,curl_slist_free_all);
    if(code)return fail(0x80004004U,42,0x214f0);
    if(!list)return 0; // original no-list branch leaves length at zero
    std::size_t bytes=5;
    for(auto* n=list;n;n=n->next){
        if(!n->data)return fail(0x80070057U,43,0x21460);
        auto len=std::strlen(n->data);
        if(bytes>UINT32_MAX-2||len>UINT32_MAX-bytes-2)return fail(0x8007000eU,27,0x214c0);
        bytes+=len+2;
    }
    auto* buffer=get<char*>(req,0xb0);
    if(!buffer||get<std::uint64_t>(req,0xa8)<bytes){
        if(buffer)s->release(buffer,16,c);
        buffer=static_cast<char*>(s->allocate(bytes,16,c));
        put(req,0xb0,buffer);put(req,0xa8,static_cast<std::uint64_t>(bytes));
        if(!buffer)return fail(0x8007000eU,27,0x214c0);
    }
    auto* dest=buffer;std::memcpy(dest,"0\r\n",3);dest+=3;
    for(auto* n=list;n;n=n->next){auto len=std::strlen(n->data);std::memcpy(dest,n->data,len);dest+=len;std::memcpy(dest,"\r\n",2);dest+=2;}
    std::memcpy(dest,"\r\n",2);*out=bytes;return 0;
}
}
