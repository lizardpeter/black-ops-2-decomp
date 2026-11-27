// Store MP: binding key scan and bounded display-name extraction.
#define T6_SOURCE "client/cl_keys.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_client_text{
using B=unsigned char;using U=std::uint32_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
extern "C"{
extern const B* TextBindings_ClientMode; // 141EF91F4, 24-byte stride
extern const int* TextBindings_KeyTable; // 140E5D36C
extern const B* TextBindings_ClientState;
extern int TextBindings_ClientCount;
const char* TextBindings_Lookup(int,int,int); // 1401A5060
const char* TextBindings_Resolve(const void*,const void*,U); // 1403A5E70
void TextBindings_Sanitize(char*); // 140471E50
}
static bool ControllerKey(U key){return B(key-1)<6||B(key-14)<12||B(key-28)<4;}
// 1401A51F0: preserve the original input-device filtering and two-result limit.
int FindBindingKeys(int client,int binding,int*out,int context,int filter){
    if(!binding)return 0;
    out[0]=out[1]=-1;
    int count=0;
    for(U key=0;key<256;++key){
        bool selected=ControllerKey(key);
        if(filter==1&&!selected)continue;
        if(selected!=(R<int>(TextBindings_ClientMode+static_cast<long long>(client)*24,0)==0))continue;
        if(TextBindings_KeyTable[static_cast<long long>(client)*0x44a+key*4+context]==binding){
            out[count++]=int(key);if(count==2)return count;
        }
    }
    return count;
}
// 1401BF5A0
void CopyBindingText(int client,int key,char*out){
    const char*s=TextBindings_Lookup(client,key,1);
    if(!out)return;
    if(!s){*out=0;return;}
    for(int i=0;i<128;++i){out[i]=s[i];if(!s[i])break;}
    out[127]=0;
}
static bool FindDisplay(int client,int id,char*out,int limit,U flags){
    *out=0;
    const B*state=client<TextBindings_ClientCount?TextBindings_ClientState:nullptr;
    if(!R<int>(state,0x68)||R<int>(state,0x2a94)<=0)return false;
    for(int i=0;i<R<int>(state,0x2a94);++i){
        int index=int(U(R<int>(state,0x2aa4))+U(i))%R<int>(state,0x28);
        const B*rec=R<const B*>(state,0x20)+static_cast<long long>(index)*0x120;
        if(R<int>(rec,0)!=id)continue;
        const char*s=TextBindings_Resolve(rec+0x44,rec+0xb0,flags);
        if(!s)*out=0;
        else{
            for(int i=0;i<limit;++i){out[i]=s[i];if(!s[i])break;}
            out[static_cast<long long>(limit)-1]=0;
        }
        TextBindings_Sanitize(out);return true;
    }
    return false;
}
// 1401BF600 and 1401BF710 differ only in the original resolver flags.
bool FindDisplayText(int client,int id,char*out,int limit,B flag){return FindDisplay(client,id,out,limit,U(flag)*2u|1u);}
bool FindDisplayTextAlternate(int client,int id,char*out,int limit){return FindDisplay(client,id,out,limit,2);}
}
#endif
