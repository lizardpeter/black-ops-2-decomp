#include "ui/ui_refresh_store.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <sys/syscall.h>
#include <asm/prctl.h>
#include <unistd.h>
#define MS __attribute__((ms_abi))
struct dvar_t;
struct _RTL_CRITICAL_SECTION;
extern "C" {
#define DEF(name) unsigned char store_uiRefresh_##name[8];
STORE_UI_REFRESH_GLOBALS(DEF)
#undef DEF
}
struct Event {uint64_t id,args[14];};
struct Capture {unsigned count,bad;Event events[48];unsigned client[3],deadline,transitionStart,transitionActive,menu[8],luiDirty,threadId,timeInit,timeBase;};
static Capture cap;static StoreUiRefreshGlobals g;static unsigned seed;static unsigned *mainThread,*timeInit,*timeBase;
static unsigned char menu[0x120],focused[0x120],font[64],vars[3][128];static const char text[]="43.1.2";
static unsigned char tlsBlock[0x80],teb[0x80];static void *tlsSlots[1];static unsigned ticks;
template<class T> static T rd(const void *p,int off=0){T v;memcpy(&v,static_cast<const unsigned char *>(p)+off,sizeof(v));return v;}
template<class T> static void wr(void *p,int off,T v){memcpy(static_cast<unsigned char *>(p)+off,&v,sizeof(v));}
static uint64_t ptr(const void *p){return reinterpret_cast<uintptr_t>(p);}
static unsigned bits(float x){return rd<unsigned>(&x);}
static Event &ev(unsigned id){auto &e=cap.events[cap.count++];e.id=id;return e;}
static unsigned MS getThread(){ev(1);return seed&2?7:9;}
static unsigned MS clockNow(){ev(2);return 1000u+ticks++*53u;}
static int MS parse(const char *s){ev(3).args[0]=*s;return atoi(s);}
bool Sys_IsMainThread(){auto id=rd<unsigned>(tlsBlock,0x3c);if(!id){id=getThread();wr(tlsBlock,0x3c,id);}return id==*mainThread;}
int Sys_Milliseconds(){if(!*timeInit){*timeBase=clockNow();*timeInit=1;}return static_cast<int>(clockNow()-*timeBase);}
bool Dvar_GetBool(const dvar_t *d){return d&&(rd<unsigned>(d,0x18)==1?rd<unsigned char>(d,0x20)!=0:parse(rd<const char *>(d,0x20))!=0);}
extern "C" int TryEnterCriticalSection(_RTL_CRITICAL_SECTION *p){ev(4).args[0]=ptr(p);return (seed&4)!=0;}
extern "C" void LeaveCriticalSection(_RTL_CRITICAL_SECTION *p){ev(5).args[0]=ptr(p);}
extern "C" void Store_UI_BeginRefresh(int n){ev(6).args[0]=static_cast<unsigned>(n);}
extern "C" void Store_UI_UpdateController(int n){ev(7).args[0]=static_cast<unsigned>(n);}
extern "C" void Store_UI_LuiInit(bool b){ev(8).args[0]=b;}
extern "C" void Store_UI_LuiRefreshA(int n){ev(9).args[0]=static_cast<unsigned>(n);}
extern "C" void Store_UI_LuiRefreshB(int n){ev(10).args[0]=static_cast<unsigned>(n);}
extern "C" void Store_UI_NotifyFlag(const void *p,unsigned char *b){auto &e=ev(11);e.args[0]=ptr(p);e.args[1]=ptr(b);e.args[2]=*b;}
extern "C" void Store_UI_RefreshPrimary(){ev(12);}
static void context(unsigned id,int n,void *p){auto &e=ev(id);e.args[0]=static_cast<unsigned>(n);e.args[1]=ptr(p);}
extern "C" void Store_UI_UpdateContext(int n,void *p){context(13,n,p);}
extern "C" void Store_UI_MainThreadContext(int n,void *p){context(14,n,p);}
extern "C" void Store_UI_RefreshDelayed(int n,void *p){context(15,n,p);}
extern "C" void Store_UI_PaintMenu(int n,void *p,int a,void *m,int b){auto &e=ev(16);e.args[0]=static_cast<unsigned>(n);e.args[1]=ptr(p);e.args[2]=a;e.args[3]=ptr(m);e.args[4]=static_cast<unsigned>(b);e.args[5]=rd<unsigned>(m,0x5c);if(seed&0x80)wr(m,0x5c,rd<unsigned>(m,0x5c)&~0x42000u);}
extern "C" void *Store_UI_FontForScale(float f){ev(17).args[0]=bits(f);return font;}
extern "C" const char *Store_UI_FormatVersion(const char *fmt,const char *a,const char *b,int major,int minor,int patch){auto &e=ev(18);if(strcmp(fmt,"%s%s%d.%d.%d")||*a||*b)++cap.bad;e.args[0]=major;e.args[1]=minor;e.args[2]=patch;return text;}
extern "C" int Store_UI_TextWidth(int,const char *t,int len,const void *f){auto &e=ev(19);if(t!=text||f!=font)++cap.bad;e.args[0]=len;return static_cast<int>((seed>>8)%2048)-64;}
extern "C" void Store_UI_DrawTextPadding(int lc,const void *placement,const char *t,int max,const void *f,float x,float y,int ha,int va,float scale,const float *white,int style,float pad){auto &e=ev(20);if(t!=text||f!=font)++cap.bad;e.args[0]=lc;e.args[1]=ptr(placement);e.args[2]=max;e.args[3]=bits(x);e.args[4]=bits(y);e.args[5]=ha;e.args[6]=va;e.args[7]=bits(scale);e.args[8]=ptr(white);e.args[9]=style;e.args[10]=bits(pad);}
static int MS tryLock(_RTL_CRITICAL_SECTION *p){return TryEnterCriticalSection(p);}
static void MS leaveLock(_RTL_CRITICAL_SECTION *p){LeaveCriticalSection(p);}
static void MS begin(int n){Store_UI_BeginRefresh(n);}static void MS controller(int n){Store_UI_UpdateController(n);}
static void MS init(bool b){Store_UI_LuiInit(b);}static void MS luiA(int n){Store_UI_LuiRefreshA(n);}static void MS luiB(int n){Store_UI_LuiRefreshB(n);}
static void MS notify(const void *p,unsigned char *b){Store_UI_NotifyFlag(p,b);}static void MS primary(){Store_UI_RefreshPrimary();}
static void MS ctx(int n,void *p){Store_UI_UpdateContext(n,p);}static void MS mainCtx(int n,void *p){Store_UI_MainThreadContext(n,p);}static void MS delayed(int n,void *p){Store_UI_RefreshDelayed(n,p);}
static void MS paint(int n,void *p,int a,void *m,int b){Store_UI_PaintMenu(n,p,a,m,b);}
static void *MS selectFont(float f){return Store_UI_FontForScale(f);}static const char *MS format(const char *f,const char *a,const char *b,int major,int minor,int patch){return Store_UI_FormatVersion(f,a,b,major,minor,patch);}
static int MS width(int lc,const char *t,int len,const void *f){return Store_UI_TextWidth(lc,t,len,f);}
static void MS draw(int lc,const void *p,const char *t,int max,const void *f,float x,float y,int ha,int va,float scale,const float *white,int style,float pad){Store_UI_DrawTextPadding(lc,p,t,max,f,x,y,ha,va,scale,white,style,pad);}
extern "C" void *RefreshMock(unsigned i){void *v[]={(void*)getThread,(void*)clockNow,(void*)parse,(void*)tryLock,(void*)leaveLock,(void*)begin,(void*)controller,(void*)init,(void*)luiA,(void*)luiB,(void*)notify,(void*)primary,(void*)ctx,(void*)mainCtx,(void*)delayed,(void*)paint,(void*)selectFont,(void*)format,(void*)width,(void*)draw};return v[i];}
static const dvar_t *Dvar(unsigned slot,unsigned mode){auto *v=vars[slot];memset(v,0,128);if(!mode)return nullptr;wr(v,0x18,mode<3?1u:8u);if(mode<3)wr<unsigned char>(v,0x20,mode==2);else wr(v,0x20,mode==3?"0":"-1");return reinterpret_cast<const dvar_t *>(v);}
extern "C" unsigned RefreshCaptureSize(){return sizeof(Capture);}
extern "C" void RunRefresh(void *entry,const StoreUiRefreshGlobals *bindings,unsigned *thread,unsigned *ti,unsigned *tb,unsigned s,bool zombies,Capture *out){
    cap={};g=*bindings;seed=s;mainThread=thread;timeInit=ti;timeBase=tb;ticks=0;unsigned x=s;auto next=[&](){x=x*1664525u+1013904223u;return x;};
    int lc=(seed&0x1000)?-1:0;*mainThread=(seed&2)?7:9;if(seed&0x8000)*mainThread=13;*timeInit=seed&0x2000?1:0;*timeBase=800;memset(tlsBlock,0,sizeof(tlsBlock));if(seed&0x4000)wr(tlsBlock,0x3c,(seed&2)?7u:9u);
    tlsSlots[0]=tlsBlock;wr(teb,0x58,tlsSlots+0);memset(g.clients,0,0x458);wr(g.clients,0,static_cast<unsigned>(0x40|(seed&1)));wr(g.clients,4,8u);wr(g.clients,8,static_cast<int>((seed>>1)%7));
    wr(g.primaryClient,0,(seed&0x10)?lc:3);wr(g.info,0,0);wr(g.info,8,400);wr(g.menuCount,0,(seed&8)?1:0);wr(g.focusCount,0,(seed&0x20)?1:0);wr(g.focusStack,0,focused+0);wr(focused,0x5c,(seed&0x40)?6u:2u);
    memset(menu,0,sizeof(menu));wr(g.menu,0,menu+0);wr(g.menuType,0,static_cast<int>((seed>>5)%13));wr(g.transitionDuration,0,(seed&0x200)?500:0);wr(g.transitionStart,0,0);wr<unsigned char>(g.transitionActive,0,(seed&0x100)!=0);wr(menu,0x5c,(seed&0x400)?0x42004u:0u);wr(menu,0xe8,(seed&0x800)?30:0);wr(menu,0xf0,static_cast<unsigned>((seed>>4)%6));wr(menu,0x110,(seed&0x1000)?40:0);
    wr(g.refreshBlocked,0,(seed&0x20)?1:0);wr(g.refreshDeadline,0,(seed&0x40)?300:500);wr(g.refreshIndex,0,static_cast<int>((seed>>2)%6)-1);wr(g.refreshLimit,0,3);
    wr<unsigned char>(g.luiReady,0,(seed&0x40)!=0);wr(g.luiState,0,(seed&0x20)?static_cast<void *>(menu):nullptr);wr<unsigned char>(g.luiDirty,0,0);
    wr(g.buildDvar,0,Dvar(0,(seed>>3)%5));wr(g.friendsDvar,0,Dvar(1,(seed>>5)%5));wr(g.playerDvar,0,Dvar(2,(seed>>7)%5));
    wr(g.hudScale,0,static_cast<float>(int(next()%80)-20)/16.0f);wr(g.placements,0,static_cast<float>(int(next()%40)-5)/8.0f);wr(g.placements,4,static_cast<float>(next()%64)/16.0f);wr(g.hiresActive,0,(seed&0x40)?1:0);wr(g.hiresX,0,1.5f);wr(g.hiresY,0,0.75f);wr(g.versionMinor,0,17);wr(g.versionPatch,0,42);wr(font,8,static_cast<int>(next()%80)-4);
    if(zombies){auto *c=g.controllerTable+lc*24;wr(c,4,(seed&0x10)?lc:3);wr(c,8,2);}
    if(entry){unsigned long old=0;if(syscall(SYS_arch_prctl,ARCH_GET_GS,&old)||syscall(SYS_arch_prctl,ARCH_SET_GS,teb))abort();reinterpret_cast<void(MS *)(int)>(entry)(lc);if(syscall(SYS_arch_prctl,ARCH_SET_GS,old))abort();}else Store_UI_RefreshBound(lc,g,zombies);
    memcpy(cap.client,g.clients,12);cap.deadline=rd<unsigned>(g.refreshDeadline);cap.transitionStart=rd<unsigned>(g.transitionStart);cap.transitionActive=*g.transitionActive;unsigned offsets[]={0x5c,0xe8,0xf0,0x110,0x114,0x118};for(unsigned i=0;i<6;++i)cap.menu[i]=rd<unsigned>(menu,offsets[i]);cap.luiDirty=*g.luiDirty;cap.threadId=rd<unsigned>(tlsBlock,0x3c);cap.timeInit=*timeInit;cap.timeBase=*timeBase;*out=cap;
}
