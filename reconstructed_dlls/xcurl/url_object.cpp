// Pinned Store XCurl 0x1738C, 0x1C390, 0x1CFD0, 0x1D000, 0x1C610,
// 0x1BCA4 and subsets of 0x1D170/0x1D740. ETW disabled.
// Instruction evidence 45548404885 and 45548553668.
#include "url_object.h"
#include <cstring>
#include <initializer_list>
using namespace t6_xcurl;
namespace {
constexpr int slots[]={-1,0,1,2,3,4,6,7,8,9,5};
constexpr unsigned missing[]={9,10,11,12,13,14,15,9,16,17,9};
bool letter(unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z');}
bool scheme_char(unsigned char c){return letter(c)||(c>='0'&&c<='9')||c=='+'||c=='-'||c=='.';}
}
extern "C" void* xcurl_research_url_create(const UrlObjectServices* s){
    auto* p=s->allocate(sizeof(UrlObject),5,s->context);
    if(p)std::memset(p,0,sizeof(UrlObject));return p;
}
extern "C" void xcurl_research_url_release_fields(void* object,const UrlObjectServices* s){
    auto* u=static_cast<UrlObject*>(object);
    for(auto* field:u->fields)if(field)s->release(field,5,s->context);
}
extern "C" void xcurl_research_url_destroy(void* object,const UrlObjectServices* s){
    if(object){xcurl_research_url_release_fields(object,s);s->release(object,5,s->context);}
}
extern "C" void* xcurl_research_url_duplicate(const void* object,const UrlObjectServices* s){
    if(!object)return nullptr; // invalid original domain, do not dereference null
    auto* u=static_cast<const UrlObject*>(object);
    auto* copy=static_cast<UrlObject*>(xcurl_research_url_create(s));if(!copy)return nullptr;
    // Original duplicate skips the zone and two parser scratch fields.
    for(auto i:{0,1,2,3,4,6,7,8,9})if(u->fields[i]){
        copy->fields[i]=s->duplicate(u->fields[i],s->context);
        if(!copy->fields[i]){xcurl_research_url_destroy(copy,s);return nullptr;}
    }
    copy->port=u->port;return copy;
}
extern "C" void xcurl_research_url_move(void* src,void* dst,const UrlObjectServices* s){
    if(src==dst)return;
    xcurl_research_url_release_fields(dst,s);
    std::memcpy(dst,src,sizeof(UrlObject));s->release(src,5,s->context);
}
extern "C" std::uint8_t xcurl_research_url_has_scheme(const char* str,char* lower){
    if(!str||!*str)return 0;
    // Original deliberately excludes Windows-style single-letter drive paths.
    if(letter(static_cast<unsigned char>(str[0]))&&str[1]==':')return 0;
    for(unsigned i=0;i<41&&str[i];++i){
        auto c=static_cast<unsigned char>(str[i]);
        if(c==':'&&str[i+1]=='/'){if(lower)lower[i]=0;return 1;}
        if(!scheme_char(c))return 0;
        if(lower)lower[i]=static_cast<char>(c>='A'&&c<='Z'?c+32:c);
    }
    return 0;
}
extern "C" std::uint32_t xcurl_research_url_clear_part(void* object,int part,const UrlObjectServices* s){
    if(!object)return 1;if(part==0)return 0; // original null URL setter is a no-op
    if(part<1||part>10)return 9;
    auto* u=static_cast<UrlObject*>(object);auto& f=u->fields[slots[part]];
    if(part==6)u->port=0;
    if(f){s->release(f,5,s->context);f=nullptr;}return 0;
}
extern "C" std::uint32_t xcurl_research_url_get_part(void* object,int part,char** out,const UrlObjectServices* s){
    if(!object)return 1;if(!out)return 2;*out=nullptr;
    if(part<1||part>10)return 9;
    auto* u=static_cast<UrlObject*>(object);auto& f=u->fields[slots[part]];
    if(part==7&&!f){f=s->duplicate("/",s->context);if(!f)return 7;}
    if(!f)return missing[part];
    *out=s->duplicate(f,s->context);return *out?0:7;
}
