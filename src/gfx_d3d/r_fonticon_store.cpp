// Store MP font icon alias tables and material fallback, 140723450.
#define T6_SOURCE "gfx_d3d/r_fonticon.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_font_icon {
using Byte=unsigned char;using U32=std::uint32_t;
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
extern "C" {
extern int Icon_AliasCount; // 1428386C0
extern const Byte *Icon_Aliases[]; // 1428386A0
extern const char *Icon_DefaultName; // literal at 140943EC0
extern void *Icon_DefaultMaterial; // 1428382C8
int Icon_Lower(int); // IAT 1408A8298
const Byte *Icon_Lookup(int,int); // 140723340
void *Icon_Material(const char*,int,bool,int); // 14071A4E0
}
static int Hash(const char*s){
    if(!s)return 0;
    U32 h=5381;
    for(;*s;++s)h=h*33u+U32(Icon_Lower(static_cast<signed char>(*s)));
    return static_cast<int>(h);
}
static const Byte *FindAlias(int hash){
    for(int i=0;i<Icon_AliasCount;++i){
        const Byte*table=Icon_Aliases[i];int lo=0,hi=R<int>(table,0xc)-1;
        const Byte*entries=R<const Byte*>(table,0x18);
        while(lo<=hi){
            int mid=(lo+hi)/2;const Byte*entry=entries+mid*8;const int v=R<int>(entry,0);
            if(v==hash)return entry;
            if(v<hash)lo=mid+1;else hi=mid-1;
        }
    }
    return nullptr;
}
void GetFontIconEntryInfo(int,const char*name,int height,void**material,float*width,float*heightScale){
    const int hash=Hash(name);
    const Byte*alias=FindAlias(hash);
    const Byte*entry=Icon_Lookup(alias?R<int>(alias,4):Hash(name),height);
    if(entry){
        *material=R<void*>(entry,0x10);*width=R<float>(entry,0x1c);*heightScale=R<float>(entry,0x20);
    }else{
        *material=*Icon_DefaultName?Icon_Material(Icon_DefaultName,0,true,-1):Icon_DefaultMaterial;
        *width=1.0f;*heightScale=1.0f;
    }
}
}
#endif
