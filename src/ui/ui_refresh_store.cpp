// Store MP 1403FD090 / ZM 1403FDA60. Complete refresh, including the inlined build overlay.
#define T6_SOURCE "ui/ui_main.cpp"
#ifdef T6_STORE
#include "ui/ui_refresh_store.h"
#include <string.h>
struct dvar_t;
struct _RTL_CRITICAL_SECTION;
#ifdef STORE_UI_REFRESH_REPLAY
extern "C" int TryEnterCriticalSection(_RTL_CRITICAL_SECTION *);
extern "C" void LeaveCriticalSection(_RTL_CRITICAL_SECTION *);
#else
#include <windows.h>
#endif
bool Sys_IsMainThread();
int Sys_Milliseconds();
bool Dvar_GetBool(const dvar_t *);
extern "C" {
void Store_UI_BeginRefresh(int);
void Store_UI_UpdateController(int);
void Store_UI_LuiInit(bool);
void Store_UI_LuiRefreshA(int);
void Store_UI_LuiRefreshB(int);
void Store_UI_NotifyFlag(const void *, unsigned char *);
void Store_UI_RefreshPrimary();
void Store_UI_UpdateContext(int, void *);
void Store_UI_MainThreadContext(int, void *);
void Store_UI_RefreshDelayed(int, void *);
void Store_UI_PaintMenu(int, void *, int, void *, int);
void *Store_UI_FontForScale(float);
const char *Store_UI_FormatVersion(const char *, const char *, const char *, int, int, int);
int Store_UI_TextWidth(int, const char *, int, const void *);
void Store_UI_DrawTextPadding(int, const void *, const char *, int, const void *, float, float, int, int, float, const float *, int, float);
#define DECLARE(name) extern unsigned char store_uiRefresh_##name[];
STORE_UI_REFRESH_GLOBALS(DECLARE)
#undef DECLARE
#ifdef T6_STORE_ZM
extern unsigned char store_uiRefresh_controllerTable[];
#endif
}
namespace {
template<class T> T Read(const void *p, int off=0) {T v;memcpy(&v,static_cast<const unsigned char *>(p)+off,sizeof(v));return v;}
template<class T> void Write(void *p,int off,T v){memcpy(static_cast<unsigned char *>(p)+off,&v,sizeof(v));}
int Add(int a,int b){return static_cast<int>(static_cast<unsigned>(a)+static_cast<unsigned>(b));}
int Sub(int a,int b){return static_cast<int>(static_cast<unsigned>(a)-static_cast<unsigned>(b));}
// Store loop has exactly one 0x458-byte local-client slot.
bool Connected(const StoreUiRefreshGlobals &g){return (Read<unsigned char>(g.clients)&1)&&Read<int>(g.clients,8)>=4;}
bool Dvar(const void *slot){return Dvar_GetBool(Read<const dvar_t *>(slot));}
int FloorToInt(float x){if(!(x>=-2147483648.0f&&x<2147483648.0f))return static_cast<int>(0x80000000u);int i=static_cast<int>(x);if(i==static_cast<int>(0x80000000u)||static_cast<float>(i)==x)return i;return i-(x<0.0f);}
void DrawBuildNumber(const StoreUiRefreshGlobals &g,int index){
    if(!Dvar(g.buildDvar)||Connected(g)||Read<int>(g.clients,8)!=0)return;
    const auto *placement=g.placements+static_cast<__PTRDIFF_TYPE__>(index)*0x7c;
    const float hud=Read<float>(g.hudScale);
    const float fontScale=((Read<int>(g.hiresActive)?Read<float>(g.hiresY):1.0f)*((hud*0.2f)*hud))*Read<float>(placement,4);
    const auto *font=Store_UI_FontForScale(fontScale);
    const char *text=Store_UI_FormatVersion("%s%s%d.%d.%d","","",43,Read<int>(g.versionMinor),Read<int>(g.versionPatch));
    const float norm=9.6f/static_cast<float>(Read<int>(font,8));
    // TextWidth's first register argument is dead in the two Store bodies.
    const int width=FloorToInt(static_cast<float>(Store_UI_TextWidth(0,text,64,font))*norm+0.5f);
    const float xScale=((Read<int>(g.hiresActive)?Read<float>(g.hiresX):1.0f)*Read<float>(g.hudScale))*Read<float>(placement);
    if(xScale>0.0f)Store_UI_DrawTextPadding(0,placement,text,64,font,-40.0f-static_cast<float>(width),20.0f,3,1,0.2f,reinterpret_cast<const float *>(g.white),0,0.0f);
}
}
void Store_UI_RefreshBound(int localClient,const StoreUiRefreshGlobals &g,bool zombies){
    Store_UI_BeginRefresh(localClient);
    if(!Connected(g)){
        if(zombies){const auto *entry=g.controllerTable+static_cast<__PTRDIFF_TYPE__>(localClient)*24;Store_UI_UpdateController(Read<int>(entry,4)==localClient?Read<int>(entry,8):-1);}
        bool main=Sys_IsMainThread();auto *lock=reinterpret_cast<_RTL_CRITICAL_SECTION *>(g.luiLock);
        if(main||TryEnterCriticalSection(lock)){
            if(main&&!Read<unsigned char>(g.luiReady)&&Read<void *>(g.luiState))Store_UI_LuiInit(true);
            Store_UI_LuiRefreshA(localClient);Store_UI_LuiRefreshB(localClient);
            Store_UI_NotifyFlag(g.luiEvent,g.luiDirty);Write<unsigned char>(g.luiDirty,0,1);
            if(!main)LeaveCriticalSection(lock);
        }
    }
    if(localClient==Read<int>(g.primaryClient))Store_UI_RefreshPrimary();
    const int index=Read<int>(g.info);
    if(Read<int>(g.menuCount)>0){
        Store_UI_UpdateContext(localClient,g.info);
        if(Sys_IsMainThread()){
            Store_UI_MainThreadContext(localClient,g.info);
            const int deadline=Read<int>(g.refreshDeadline);
            if(!Read<int>(g.refreshBlocked)&&deadline&&deadline<=Read<int>(g.info,8)){
                Store_UI_RefreshDelayed(localClient,g.info);
                const int i=Read<int>(g.refreshIndex),limit=Read<int>(g.refreshLimit);
                if(i>=0&&i<=limit&&limit)Write(g.refreshDeadline,0,Add(Read<int>(g.info,8),500));
            }
        }
        if(Read<unsigned char>(g.transitionActive)){
            auto *m=Read<unsigned char *>(g.menu);const int off=0x5c+index*4;
            const int elapsed=Sub(Sys_Milliseconds(),Read<int>(g.transitionStart));const unsigned flags=Read<unsigned>(m,off);
            if(elapsed<Read<int>(g.transitionDuration)){
                Write(m,off,flags|4u);Store_UI_PaintMenu(localClient,g.info,0,m,-1);
            }else if(flags&0x42000u){
                Store_UI_PaintMenu(localClient,g.info,0,m,-1);
                const unsigned nowFlags=Read<unsigned>(m,off);
                if(!(nowFlags&0x42000u)){Write(m,off,nowFlags&~6u);Write(g.transitionStart,0,-1);Write<unsigned char>(g.transitionActive,0,0);}
            }else{
                Store_UI_PaintMenu(localClient,g.info,0,m,-1);
                if(Read<int>(m,0xe8)>0&&Read<unsigned>(m,0xf0)<=3){
                    Write(m,off,Read<unsigned>(m,off)|0x40000u);Write(m,0x118,Sys_Milliseconds());
                    if(Read<int>(m,0x110)>0){Write(m,off,Read<unsigned>(m,off)|0x2000u);int a=Read<int>(m,0xe8),b=Read<int>(m,0x110),v=a>b?a:b;Write(m,0xe8,v);Write(m,0x110,v);Write(m,0x114,Sys_Milliseconds());}
                }else if(Read<int>(m,0x110)>0){Write(m,off,Read<unsigned>(m,off)|0x2000u);Write(m,0x114,Sys_Milliseconds());}
            }
        }
        DrawBuildNumber(g,index);
    }
    if(Connected(g)){
        bool focused=false;const int currentIndex=Read<int>(g.info);
        for(int i=Read<int>(g.focusCount)-1;i>=0;--i){auto *m=Read<unsigned char *>(g.focusStack,i*16);if((Read<unsigned>(m,0x5c+currentIndex*4)&6u)==6u){focused=m!=nullptr;break;}}
        if(!focused&&localClient!=-1&&(Read<unsigned>(g.clients,4)&8u)&&static_cast<unsigned>(Read<int>(g.menuType))-8u>2u){Write(g.clients,4,Read<unsigned>(g.clients,4)&~8u);const int off=localClient*0x458;Write(g.clients,off,Read<unsigned>(g.clients,off)&~0x40u);}
    }
    if(!Dvar(g.friendsDvar)){
        const auto *dvar=Read<const dvar_t *>(g.playerDvar);
        if(dvar&&Read<unsigned>(dvar,0x18)!=1)(void)Dvar_GetBool(dvar);
    }
}
void UI_Refresh(int localClient){
    // Filled field by field: a constant initializer would become a table of exe addresses in .rdata, which the
    // x64 hybrid link can't relocate (tools/build_recon.py exe_relative_asm).
    StoreUiRefreshGlobals g;
#define BIND(name) g.name=store_uiRefresh_##name;
    STORE_UI_REFRESH_GLOBALS(BIND)
#ifdef T6_STORE_ZM
    g.controllerTable=store_uiRefresh_controllerTable;
#else
    g.controllerTable=nullptr;
#endif
#undef BIND
#ifdef T6_STORE_ZM
    Store_UI_RefreshBound(localClient,g,true);
#else
    Store_UI_RefreshBound(localClient,g,false);
#endif
}
#endif
