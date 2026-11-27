// Store MP zone-name generation and synchronous UI zone load.
// Original addresses 140215300, 140217160, 1403C2D80.
#define T6_SOURCE "database/db_registry.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_font_zones {
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
struct ZoneInfo {const char*name;Q flags;};
static_assert(sizeof(ZoneInfo)==16,"Store zone info");
extern "C"{
extern char* Zone_Names[13];
extern const char* Zone_Bases[13];
extern char* Zone_Suffix;
extern const char* Zone_BaseSuffix;
extern const B* Zone_FontResolution;
extern void* Zone_LoadEvent;
extern void* Zone_WaitHook;
int Zone_Atoi(const char*);
const char* Zone_FormatResolution(const char*,int,const char*);
const char* Zone_FormatPatch(const char*,const char*,const char*,const char*);
const char* Zone_FormatFile(const char*,const char*,const char*);
[[noreturn]] void Zone_StringOverflow(int);
void Zone_Queue(const ZoneInfo*,int,int,int);
void Zone_BeginSync();
void Zone_EndSync();
void Zone_Complete();
unsigned Zone_Wait(void*,unsigned);
void Zone_Pump();
}
static void Copy(char*d,const char*s,int n){
    if(!d)return;
    if(!s){*d=0;return;}
    for(int i=0;i<n;++i){d[i]=s[i];if(!s[i])break;}
    d[n-1]=0;
}
static void Append(char*d,const char*s){
    int len=0;while(d[len])++len;
    if(len>=24)Zone_StringOverflow(0);
    Copy(d+len,s,24-len);
}
static int Resolution(){
    const B*d=Zone_FontResolution;if(!d)return 0;
    int type=R<int>(d,0x18);
    if(U(type-6)<2)return R<int>(d,0x20);
    if(type==1)return d[0x20]!=0;
    if(type==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Zone_Atoi(R<const char*>(d,0x20));
}
// Original destination order and source RVAs are documented alongside the probe.
// Unlike a generic concatenation, each 24-byte slot keeps untouched tail bytes.
void BuildZoneNames(){
    for(unsigned i=0;i<13;++i)Copy(Zone_Names[i],Zone_Bases[i],24);
    Copy(Zone_Suffix,Zone_BaseSuffix,4);
    Append(Zone_Names[0],Zone_Suffix);
    Append(Zone_Names[1],Zone_Suffix);
    Append(Zone_Names[2],Zone_FormatResolution("_%d%s",Resolution(),Zone_Suffix));
    const unsigned order[]={3,4,5,7,6,8,9};
    for(unsigned i:order)Append(Zone_Names[i],Zone_Suffix);
    Append(Zone_Names[11],Zone_FormatPatch("%s%s%s","_tu17",Zone_Suffix,"_147"));
    Append(Zone_Names[12],Zone_FormatFile("%s%s",Zone_Names[11],".ff"));
}
// 1403C2D80. The callback is a function pointer at +0x10, not a COM vtable.
void WaitForZoneLoad(){
    if(Zone_Wait(Zone_LoadEvent,0)==0)return;
    while(Zone_Wait(Zone_LoadEvent,1000)!=0){
        if(Zone_WaitHook)R<void(*)()>(Zone_WaitHook,0x10)();
        Zone_Pump();
    }
}
// 140217160
void LoadUIZones(){
    ZoneInfo zones[]={{Zone_Names[4],2},{Zone_Names[1],8}};
    Zone_Queue(zones,2,0,0);
    Zone_BeginSync();WaitForZoneLoad();Zone_EndSync();Zone_Complete();
}
}
#endif
