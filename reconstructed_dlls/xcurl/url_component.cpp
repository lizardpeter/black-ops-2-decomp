// Original Store XCurl 0x1C46C, scheme table 0x1F1C0,
// 0x1D740 flag-zero individual setter, 0x1D170 formatter/getter.
// Instruction evidence 45548553668, 45550472340, 45550531798.
// URL encode/decode flags and full-URL parsing are separate boundaries.
#include "url_component.h"
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <string>
#include <new>
#include <cctype>
using namespace t6_xcurl;
namespace {
constexpr int slots[]={-1,0,1,2,3,4,6,7,8,9,5};
bool equal(const char* a,const char* b){
    if(!a)return false;
    for(;;++a,++b){unsigned char x=*a,y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;if(!x)return true;}
}
bool hex(unsigned char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');}
bool ipchar(unsigned char c){return hex(c)||c==':'||c=='.';}
char lower(char c){return c>='A'&&c<='F'?c+32:c;}
const char* str(const char* s){return s?s:"";}
}
std::uint32_t t6_xcurl::url_scheme_port(const char* s){
    if(equal(s,"https"))return 443;if(equal(s,"http"))return 80;return 0;
}
extern "C" std::uint32_t xcurl_research_url_validate_host(void* object,char* host,const UrlObjectServices* s){
    auto* u=static_cast<UrlObject*>(object);auto len=std::strlen(host);
    if(*host!='[')return !len?14:std::strchr(host,' ')?3:0;
    if(len<5||host[len-1]!=']')return 3;
    auto* p=host+1;while(ipchar(*p))++p;
    if(*p==']'&&p==host+len-1)return p==host+1?14:0;
    if(*p!='%')return 3;
    auto* zone=p+1;
    if(zone[0]=='2'&&zone[1]=='5'&&zone[2]&&zone[2]!=']')zone+=2;
    auto n=std::strcspn(zone,"]");if(!n||n>15||zone[n]!=']')return 3;
    char value[16];std::memcpy(value,zone,n);value[n]=0;
    // Original stores the zone directly; caller owns clearing prior zone.
    u->fields[5]=s->duplicate(value,s->context);if(!u->fields[5])return 7;
    *p=']';p[1]=0;return host[1]==']'?14:0;
}
extern "C" std::uint32_t xcurl_research_url_set_part(void* object,int part,const char* text,const UrlObjectServices* s){
    if(!object)return 1;if(!text)return xcurl_research_url_clear_part(object,part,s);
    if(part<1||part>10)return 9;
    auto* u=static_cast<UrlObject*>(object);auto n=std::strlen(text);std::uint32_t port=0;
    if(part==1){if(n>40)return 3;if(!url_scheme_port(text))return 5;}
    if(part==6){
        char* end=nullptr;errno=0;long v=std::strtol(text,&end,10);
        if(errno==ERANGE||v<1||v>65535)return 4;if(*end)return 3;port=static_cast<std::uint32_t>(v);
    }
    if(part==5&&u->fields[5]){s->release(u->fields[5],5,s->context);u->fields[5]=nullptr;}
    if(n>8000000)return 3;
    auto* copy=s->duplicate(text,s->context);if(!copy)return 7;
    for(auto* p=copy;*p;++p)if(*p=='%'&&p[1]&&p[2]&&hex(p[1])&&hex(p[2])){
        p[1]=lower(p[1]);p[2]=lower(p[2]);p+=2;
    }
    if(part==5&&xcurl_research_url_validate_host(u,copy,s)){s->release(copy,5,s->context);return 3;}
    auto& field=u->fields[slots[part]];
    if(field)s->release(field,5,s->context);field=copy;
    if(port)u->port=port;return 0;
}
extern "C" std::uint32_t xcurl_research_url_format(void* object,char** out,unsigned flags,const UrlObjectServices* s){
    if(!object)return 1;if(!out)return 2;*out=nullptr;
    if(flags&~7U)return 9;
    const auto* u=static_cast<const UrlObject*>(object);const auto& f=u->fields;
    const char* scheme=f[0];char portbuf[16]{};
    try {
        std::string value;
        if(equal(scheme,"file")){
            value="file://";value+=str(f[7]);if(f[9]){value+='#';value+=f[9];}
        }else{
            if(!f[4])return 14;
            if(!scheme){if(!(flags&4))return 10;scheme="https";}
            const auto default_port=url_scheme_port(scheme);const char* port=f[6];
            if(!port&&(flags&1)&&default_port){std::snprintf(portbuf,sizeof(portbuf),"%u",default_port);port=portbuf;}
            else if(port&&(flags&2)&&default_port&&u->port==default_port)port=nullptr;
            value=scheme;value+="://";
            // HTTP/HTTPS descriptors have bit 0x400 clear. The original
            // omits options for these schemes, retaining user/password.
            const char* options=default_port?nullptr:f[3];
            if(f[1]||f[2]||options){
                value+=str(f[1]);if(f[2]){value+=':';value+=f[2];}
                if(options){value+=';';value+=options;}value+='@';
            }
            if(f[4][0]=='['&&f[5]){
                auto n=std::strlen(f[4]);value.append(f[4],n?n-1:0);value+="%25";value+=f[5];value+=']';
            }else value+=f[4];
            if(port){value+=':';value+=port;}
            if(!f[7]||f[7][0]!='/')value+='/';value+=str(f[7]);
            if(f[8]&&*f[8]){value+='?';value+=f[8];}
            if(f[9]){value+='#';value+=f[9];}
        }
        *out=s->duplicate(value.c_str(),s->context);return *out?0:7;
    }catch(const std::bad_alloc&){return 7;}
}
extern "C" std::uint32_t xcurl_research_url_get_flags(void* object,int part,char** out,unsigned flags,const UrlObjectServices* s){
    if(!object)return 1;if(!out)return 2;*out=nullptr;
    if(!part)return xcurl_research_url_format(object,out,flags,s);
    if(flags&~7U)return 9;
    if(part!=6)return xcurl_research_url_get_part(object,part,out,s);
    auto* u=static_cast<UrlObject*>(object);auto port=url_scheme_port(u->fields[0]);
    if(!u->fields[6]&&port&&(flags&1)){char buf[16];std::snprintf(buf,sizeof(buf),"%u",port);*out=s->duplicate(buf,s->context);return *out?0:7;}
    if(u->fields[6]&&port&&u->port==port&&(flags&2))return 15;
    return xcurl_research_url_get_part(object,part,out,s);
}
