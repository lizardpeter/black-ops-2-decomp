// Original Store XCurl 0x1C5D4, 0x1AE40, 0x1BFB4, 0x1C678,
// 0x1BD5C (user/password subset), 0x1C744/0x1C780 HTTP parser,
// and 0x1D740 initial/full URL setter. Disabled provider, flag zero.
// Evidence runs 45550472340, 45550531798 and 45550637567.
#include "url_parse.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
using namespace t6_xcurl;
namespace {
bool ipchar(unsigned char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')||c==':'||c=='.';}
char* copy_n(const char* p,std::size_t n,const UrlObjectServices* s){
    auto* dst=static_cast<char*>(s->allocate(n+1,5,s->context));
    if(dst){std::memcpy(dst,p,n);dst[n]=0;}return dst;
}
void release(char*& p,const UrlObjectServices* s){if(p){s->release(p,5,s->context);p=nullptr;}}
std::uint32_t userinfo(UrlObject* u,char*& host,const UrlObjectServices* s){
    auto* at=std::strchr(host,'@');if(!at)return 0;
    auto* colon=static_cast<char*>(std::memchr(host,':',at-host));auto* end=colon?colon:at;
    char* user=nullptr;char* pass=nullptr;
    if(end!=host){user=copy_n(host,end-host,s);if(!user)return 3;}
    if(colon&&colon+1!=at){pass=copy_n(colon+1,at-colon-1,s);if(!pass){release(user,s);return 3;}}
    host=at+1;u->fields[1]=user;u->fields[2]=pass;return 0;
}
}
extern "C" std::uint32_t xcurl_research_url_control_check(const char* p){
    if(!p)return 0;for(;*p;++p){auto c=static_cast<unsigned char>(*p);if(c<32||c==127)return 3;}return 0;
}
extern "C" char* xcurl_research_url_normalize_path(const char* source,const UrlObjectServices* s){
    auto n=std::strlen(source);auto* output=static_cast<char*>(s->allocate(n+1,5,s->context));if(!output)return nullptr;
    *output=0;auto* input=s->duplicate(source,s->context);if(!input){s->release(output,5,s->context);return nullptr;}
    if(!*input){s->release(output,5,s->context);return input;}
    auto* query=std::strchr(input,'?');if(query)*query=0;
    auto* r=input;auto* w=output;
    auto pop=[&]{while(w>output){--w;if(*w=='/')break;}};
    while(*r){
        if(!std::strncmp(r,"./",2)){r+=2;continue;}
        if(!std::strncmp(r,"../",3)){r+=3;continue;}
        if(!std::strncmp(r,"/./",3)){r+=2;continue;}
        if(!std::strcmp(r,"/.")){++r;*r='/';continue;}
        if(!std::strncmp(r,"/../",4)){r+=3;pop();*w=0;continue;}
        if(!std::strcmp(r,"/..")){r+=2;*r='/';pop();*w=0;continue;}
        if(!std::strcmp(r,".")||!std::strcmp(r,"..")){*r=0;*output=0;break;}
        do{*w++=*r++;}while(*r&&*r!='/');*w=0;
    }
    if(query){const auto* tail=source+(query-input);std::memcpy(w,tail,std::strlen(tail)+1);}
    s->release(input,5,s->context);return output;
}
extern "C" std::uint32_t xcurl_research_url_parse_port(void* object,char* host,const UrlObjectServices* s){
    auto* u=static_cast<UrlObject*>(object);char* colon=nullptr;
    if(*host=='['){
        auto* p=host+1;unsigned n=0;while(*p&&ipchar(*p)&&n<45){++p;++n;}
        if(n&&*p==']')++p;
        else if(n&&*p=='%'){auto* bracket=std::strchr(p+1,']');if(!bracket||bracket==p+1)return 3;p=bracket+1;}
        else if(n)return 3;
        else p=nullptr;
        if(p){if(*p&&*p!=':')return 3;if(*p)colon=p;}
        else colon=std::strchr(host,':');
    }else colon=std::strchr(host,':');
    if(!colon)return 0;
    if(!colon[1]){*colon=0;return 0;}
    if(colon[1]<'0'||colon[1]>'9')return 4;
    char* end=nullptr;errno=0;const long port=std::strtol(colon+1,&end,10);
    if(errno==ERANGE||port<1||port>65535||*end)return 4;
    *colon=0;u->port=static_cast<std::uint32_t>(port);char buf[7];std::snprintf(buf,sizeof(buf),"%u",u->port);
    u->fields[6]=s->duplicate(buf,s->context);return u->fields[6]?0:7;
}
extern "C" std::uint32_t xcurl_research_url_parse_http(const char* text,void* object,const UrlObjectServices* s){
    auto* u=static_cast<UrlObject*>(object);
    auto parse=[&]()->std::uint32_t{
        if(!text)return 3;const auto n=std::strlen(text);if(n>8000000)return 3;
        auto* scratch=static_cast<char*>(s->allocate(n*2+2,5,s->context));u->fields[10]=scratch;if(!scratch)return 7;
        auto* authority=scratch+n+1;*authority=0;*scratch=0;
        char scheme[42]{};
        if(!xcurl_research_url_has_scheme(text,scheme))return 3;
        // The original contains a separate file URL parser before its
        // HTTP/HTTPS scheme table. Not a supported XCurl transport.
        if(!url_scheme_port(scheme))return 5;
        const char* p=text+std::strlen(scheme)+1;
        unsigned slashes=0;while(*p=='/'&&slashes<4){++p;++slashes;}if(slashes<1||slashes>3)return 3;
        auto host_n=std::strcspn(p,"/?#");if(!host_n)return 3;
        std::memcpy(authority,p,host_n);authority[host_n]=0;p+=host_n;
        std::strcpy(scratch,p);
        u->fields[0]=s->duplicate(scheme,s->context);if(!u->fields[0])return 7;
        if(xcurl_research_url_control_check(scratch))return 3;
        if(auto* fragment=std::strchr(scratch,'#')){*fragment++=0;if(*fragment){u->fields[9]=s->duplicate(fragment,s->context);if(!u->fields[9])return 7;}}
        if(auto* query=std::strchr(scratch,'?')){*query++=0;u->fields[8]=s->duplicate(query,s->context);if(!u->fields[8])return 7;}
        if(*scratch){
            auto* path=xcurl_research_url_normalize_path(scratch,s);if(!path)return 7;
            if(!std::strcmp(path,scratch)){s->release(path,5,s->context);path=s->duplicate(scratch,s->context);if(!path)return 7;}
            u->fields[7]=path;
        }
        if(xcurl_research_url_control_check(authority))return 3;
        auto* host=authority;auto status=userinfo(u,host,s);if(status)return status;
        status=xcurl_research_url_parse_port(u,host,s);if(status)return status;
        status=xcurl_research_url_validate_host(u,host,s);if(status)return status;
        u->fields[4]=s->duplicate(host,s->context);if(!u->fields[4])return 7;
        release(u->fields[10],s);release(u->fields[11],s);return 0;
    };
    auto result=parse();
    if(result){xcurl_research_url_release_fields(u,s);std::memset(u,0,sizeof(UrlObject));}
    return result;
}
extern "C" std::uint32_t xcurl_research_url_set_http(void* object,int part,const char* text,const UrlObjectServices* s){
    if(!object)return 1;if(part||!text)return xcurl_research_url_set_part(object,part,text,s);
    // The original's non-absolute URL path performs relative resolution
    // against the existing object. XCurl's 10EA4 creates a fresh object;
    // preserve its missing-scheme result without inventing relative behavior.
    if(!xcurl_research_url_has_scheme(text,nullptr))return 3;
    auto* parsed=xcurl_research_url_create(s);if(!parsed)return 7;
    auto status=xcurl_research_url_parse_http(text,parsed,s);
    if(status)xcurl_research_url_destroy(parsed,s);else xcurl_research_url_move(parsed,object,s);
    return status;
}
