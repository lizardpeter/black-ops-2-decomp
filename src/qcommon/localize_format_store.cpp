// Original Store MP localization formatting (1403E8CC0, 1403E9320).
#define T6_SOURCE "qcommon/strings.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstring>
#include <cstdint>
namespace store_localize{const char*Lookup(const char*);}
namespace store_loc_format{
using B=unsigned char;
template<class T>static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const B*>(p)+off,sizeof(v));return v;}
extern "C" {
extern const B* Loc_Translate,*Loc_Warn;
extern char Loc_ExeBuffer[1024],Loc_Buffers[10][1024];
extern int Loc_BufferIndex;
int Loc_Atoi(const char*);
const char*Loc_Format(const char*,const char*);
void Loc_Error(int,const char*,...);
}
static bool Enabled(const B*d){return d&&(R<int>(d,0x18)==1?d[0x20]!=0:Loc_Atoi(R<const char*>(d,0x20))!=0);}
static int Length(const char*s){int n=0;while(s[n])++n;return n;}
static void Copy(char*d,const char*s,int n){if(d){for(int i=0;i<n;++i){d[i]=s[i];if(!s[i])break;}d[n-1]=0;}}
static void Append(char*d,const char*s){int n=Length(d);if(n>1023)Loc_Error(0,"localization string overflow");Copy(d+n,s,1024-n);}
static void CopyAll(char*d,const char*s){while((*d++=*s++)!=0){}}
static bool Marker(const char*p){return p[0]=='&'&&p[1]=='&'&&p[2]>='0'&&p[2]<='9';}
// 1403E8CC0
const char* TranslateExe(const char*key){
    if(!key[0]||!key[1]||key[0]=='^')return key;
    if(const char*t=store_localize::Lookup(key))return t;
    if(Enabled(Loc_Translate)){
        if(Enabled(Loc_Warn))Loc_Error(7,"Could not translate exe string \"%s\"",key);
        CopyAll(Loc_ExeBuffer,"^1UNLOCALIZED(^7");Append(Loc_ExeBuffer,key);Append(Loc_ExeBuffer,"^1)^7");
    }else Copy(Loc_ExeBuffer,key,1024);
    return Loc_ExeBuffer;
}
// 1403E9320. The original assumes valid sequential substitution markers;
// malformed unmatched markers can read undefined stack contents.
const char* TranslateMessage(const char*input,const char*context,int mode){
    Loc_BufferIndex=(Loc_BufferIndex+1)%10;
    char*out=Loc_Buffers[Loc_BufferIndex];for(int i=0;i<1024;++i)out[i]=0;
    bool localize=true,allowMarkers=true,escaped=false;
    const bool enabled=Enabled(Loc_Translate),warn=Enabled(Loc_Warn);
    int length=0,pending=0,marker=1;
    const char*start=input,*end=input;
    char tail[1032],part[1024];
    while(*start){
        if(*end&&unsigned(B(*end)-20)>=3){++end;continue;}
        if(start<end){
            int n=int(end-start);Copy(part,start,n+1);part[n]=0;
            if(localize){
                const char*t=part;
                if(part[0]&&part[1]&&part[0]!='^'){
                    t=store_localize::Lookup(part);
                    if(!t){
                        const char*fmt="%s";
                        if(Enabled(Loc_Translate)){
                            if(Enabled(Loc_Warn)&&mode!=1)Loc_Error(7,"Could not translate part of %s: \"%s\"",context,part);
                            fmt="^1UNLOCALIZED(^7%s^1)^7";
                        }
                        t=Loc_Format(fmt,part);
                        if(mode==1)return nullptr;
                    }
                }
                if(t)Copy(part,t,1024);else part[0]=0;
                n=Length(part);
            }
            if(n+length>1023&&enabled&&warn&&mode!=1)Loc_Error(1,"%s too long when translated: \"%s\"",context,input);
            for(int i=0;i<n-2;++i)if(Marker(part+i)){if(allowMarkers)++pending;else{part[i]=22;escaped=true;}}
            if(pending<1||length<1)CopyAll(out+length,part);
            else{
                int i=0;
                for(;i<length-2;++i)if(Marker(out+i)&&out[i+2]-'0'==marker){
                    CopyAll(tail,out+i+3);++marker;out[i]=0;break;
                }
                CopyAll(out+i,part);CopyAll(out+i+n,tail);length-=3;--pending;
            }
            length+=n;
        }
        if(*end==20){localize=true;++end;}
        else if(*end==21){localize=false;++end;}
        allowMarkers=*end!=22;if(!allowMarkers)++end;
        start=end;
    }
    if(escaped)for(int i=0;i<length;++i)if(out[i]==22)out[i]='%';
    return out;
}
}
#endif
