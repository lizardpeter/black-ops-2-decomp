#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/database/db_font_zones_store.cpp"
#include <cstdlib>
#include <cstdio>
#define MS __attribute__((ms_abi))
namespace store_font_zones{
extern "C"{
char* Zone_Names[13];
const char* Zone_Bases[13];
char* Zone_Suffix;
const char* Zone_BaseSuffix;
const B* Zone_FontResolution;
void* Zone_LoadEvent,*Zone_WaitHook;
}
static const unsigned names[]={0xf40868,0xe648a8,0xe64408,0xf4b278,0xe64340,0xf7da98,0xf404c8,0xf47c48,0xf47808,0xf40128,0xe64358,0xf417e8,0xf47e28};
static const unsigned bases[]={0x944490,0x9444a0,0x9444a0,0x9444b0,0x9444b4,0x944518,0x944528,0x944538,0x940c1c,0x944544,0x92e420,0x9444f8,0x8e1fd0};
static char storage[13][24],suffix[4],fmt[256];
static B dvar[48],hook[24];
struct Capture{char slots[13][24],suffix[4];unsigned calls[64],count;Q flags[2];char queued[2][24];};
static Capture*cap;static unsigned waitCount,waitIndex;
static void Push(unsigned v){cap->calls[cap->count++]=v;}
extern "C"{
int Zone_Atoi(const char*s){return std::atoi(s);}
const char* Zone_FormatResolution(const char*f,int n,const char*s){std::snprintf(fmt,sizeof fmt,f,n,s);return fmt;}
const char* Zone_FormatPatch(const char*f,const char*a,const char*b,const char*c){std::snprintf(fmt,sizeof fmt,f,a,b,c);return fmt;}
const char* Zone_FormatFile(const char*f,const char*a,const char*b){std::snprintf(fmt,sizeof fmt,f,a,b);return fmt;}
[[noreturn]] void Zone_StringOverflow(int){std::abort();}
void Zone_Queue(const ZoneInfo*z,int n,int a,int b){Push(0x100u+unsigned(n));Push(unsigned(a));Push(unsigned(b));for(int i=0;i<2;++i){cap->flags[i]=z[i].flags;std::memcpy(cap->queued[i],z[i].name,24);}}
void Zone_BeginSync(){Push(0x200);}
void Zone_EndSync(){Push(0x201);}
void Zone_Complete(){Push(0x202);}
unsigned Zone_Wait(void*,unsigned ms){Push(ms);return waitIndex++<waitCount?258:0;}
void Zone_Pump(){Push(0x300);}
}
static void Hook(){Push(0x301);}
static void MS MHook(){Hook();}
static unsigned MS MWait(void*a,unsigned b){return Zone_Wait(a,b);}
static void MS MPump(){Zone_Pump();}
static int MS MAtoi(const char*s){return Zone_Atoi(s);}
static void MS MQueue(const ZoneInfo*z,int n,int a,int b){Zone_Queue(z,n,a,b);}
static void MS MBegin(){Zone_BeginSync();}
static void MS MEnd(){Zone_EndSync();}
static void MS MComplete(){Zone_Complete();}
static const char* MS MFormat(const char*f,Q a,const char*b,const char*c){
    if(f[1]=='%'||f[0]=='_')return Zone_FormatResolution(f,int(a),b);
    if(std::strcmp(f,"%s%s%s")==0)return Zone_FormatPatch(f,reinterpret_cast<const char*>(a),b,c);
    return Zone_FormatFile(f,reinterpret_cast<const char*>(a),b);
}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
void*ZoneMock(unsigned i){void*p[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MFormat),reinterpret_cast<void*>(MWait),reinterpret_cast<void*>(MPump),reinterpret_cast<void*>(MQueue),reinterpret_cast<void*>(MBegin),reinterpret_cast<void*>(MEnd),reinterpret_cast<void*>(MComplete)};return p[i];}
unsigned ZoneSize(){return sizeof(Capture);}
void ZoneBind(void*image){auto*b=static_cast<B*>(image);for(unsigned i=0;i<13;++i){Zone_Names[i]=storage[i];Zone_Bases[i]=reinterpret_cast<const char*>(b+bases[i]);}Zone_Suffix=suffix;Zone_BaseSuffix=reinterpret_cast<const char*>(b+0x93d058);}
void ZoneRun(void*image,unsigned op,unsigned seed,void*out){
    auto*b=static_cast<B*>(image);Capture c{};cap=&c;waitIndex=0;waitCount=(seed>>4)%8;
    std::memset(dvar,0,sizeof dvar);std::memset(storage,0xa5,sizeof storage);std::memset(suffix,0xa5,sizeof suffix);
    int n=int(seed%4000)-1000;
    Zone_FontResolution=dvar;
    switch(seed%6){case 0:Zone_FontResolution=nullptr;break;case 1:W(dvar,0x18,1);W(dvar,0x20,B(seed&8));break;case 2:W(dvar,0x18,2);W(dvar,0x20,float(n)+0.5f);break;case 3:W(dvar,0x18,6);W(dvar,0x20,n);break;case 4:W(dvar,0x18,7);W(dvar,0x20,n);break;case 5:W(dvar,0x18,8);W(dvar,0x20,seed&8?"720":"1080");break;}
    Zone_LoadEvent=reinterpret_cast<void*>(0x1234);
    Zone_WaitHook=seed&1?hook:nullptr;
    W(hook,0x10,b?reinterpret_cast<void*>(MHook):reinterpret_cast<void*>(Hook));
    if(b){
        for(unsigned i=0;i<13;++i)std::memcpy(b+names[i],storage[i],24);
        std::memcpy(b+0xf7dcb8,suffix,4);W(b,0x27e71d0,Zone_FontResolution);W(b,0x406fc60,Zone_LoadEvent);W(b,0x283cb70,Zone_WaitHook);
        if(op==0)reinterpret_cast<void(MS*)()>(b+0x215300)();
        if(op==1)reinterpret_cast<void(MS*)()>(b+0x3c2d80)();
        if(op==2){std::memcpy(b+names[4],"ui_zone",8);std::memcpy(b+names[1],"common",7);reinterpret_cast<void(MS*)()>(b+0x217160)();}
        for(unsigned i=0;i<13;++i)std::memcpy(c.slots[i],b+names[i],24);
        std::memcpy(c.suffix,b+0xf7dcb8,4);
    }else{
        if(op==0)BuildZoneNames();
        if(op==1)WaitForZoneLoad();
        if(op==2){std::memcpy(storage[4],"ui_zone",8);std::memcpy(storage[1],"common",7);LoadUIZones();}
        std::memcpy(c.slots,storage,sizeof storage);std::memcpy(c.suffix,suffix,4);
    }
    std::memcpy(out,&c,sizeof c);
}
}
}
