#include "ui/ui_register_dvars.h"
#include <cstring>
#include <cstdint>
#define MS __attribute__((ms_abi))
union alignas(16) StoreUiDvarValue { unsigned words[4]; const char *string; };
struct alignas(16) StoreUiDvarDomain { unsigned words[4]; };
using Value=StoreUiDvarValue;using Domain=StoreUiDvarDomain;
extern "C" {
#define DEF(name) const dvar_t *store_uiDvar_##name;
STORE_UI_DVAR_GLOBALS(DEF)
#undef DEF
}
struct Event {unsigned kind,type,flags,words[6],token;char name[64],str[32];};
struct Capture {unsigned n,bad;Event events[160];};
static Capture cap;static unsigned seed,ix;static constexpr unsigned TOKEN=0x12340000;
static Event &emit(unsigned kind,const char *name){auto &e=cap.events[cap.n++];e.kind=kind;if(name)std::strncpy(e.name,name,sizeof(e.name)-1);return e;}
static const dvar_t *token(){return reinterpret_cast<const dvar_t *>(uintptr_t(TOKEN+16*++ix));}
static unsigned val(const dvar_t *p){return static_cast<unsigned>(reinterpret_cast<uintptr_t>(p));}
extern "C" const dvar_t *Store_UI_FindDvar(const char *name){auto &e=emit(1,name);unsigned h=seed;for(auto p=name;*p;++p)h=h*33u+static_cast<unsigned char>(*p);bool exists=seed==1||(seed>1&&(h%3==0));e.token=exists?TOKEN+16*(ix+1):0;return reinterpret_cast<const dvar_t *>(uintptr_t(e.token));}
static const dvar_t *reg(unsigned kind,const char *name,unsigned type,unsigned flags,const Value *value,const Domain *domain,const char *desc){auto &e=emit(kind,name);e.type=type;e.flags=flags;const auto *r=token();e.token=val(r);if(desc&&*desc)++cap.bad;if(type==8)std::strncpy(e.str,value->string,sizeof(e.str)-1);else e.words[0]=type==1?value->words[0]&255u:value->words[0];if(type==6){e.words[1]=domain->words[0];e.words[2]=domain->words[1];}return r;}
extern "C" const dvar_t *Store_UI_RegisterNew(const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *desc){return reg(2,n,t,f,v,d,desc);}
extern "C" void Store_UI_Reregister(const dvar_t *p,const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *desc){if(reg(3,n,t,f,v,d,desc)!=p)++cap.bad;}
const dvar_t *_Dvar_RegisterFloat(const char *n,float a,float b,float c,unsigned f,const char *desc){auto &e=emit(4,n);e.flags=f;float v[]={a,b,c};memcpy(e.words,v,12);if(*desc)++cap.bad;auto r=token();e.token=val(r);return r;}
extern "C" const dvar_t *Store_UI_RegisterString(const char *n,const char *s,unsigned f){auto &e=emit(5,n);e.flags=f;std::strncpy(e.str,s,sizeof(e.str)-1);auto r=token();e.token=val(r);return r;}
extern "C" const dvar_t *Store_UI_RegisterColor(const char *n,float r,float g,float b,float a,unsigned f){auto &e=emit(6,n);e.flags=f;float v[]={r,g,b,a};memcpy(e.words,v,16);auto p=token();e.token=val(p);return p;}
const dvar_t *_Dvar_RegisterBool(const char *n,bool b,unsigned f,const char *desc){auto &e=emit(7,n);e.flags=f;e.words[0]=b;if(*desc)++cap.bad;auto p=token();e.token=val(p);return p;}
extern "C" void Store_UI_RegisterRelatedDvars(){emit(8,nullptr);}
extern "C" void Store_UI_RegisterTailDvars(){emit(9,nullptr);}
static const dvar_t *MS lookup(const char *n){return Store_UI_FindDvar(n);}
static const dvar_t *MS fresh(const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *s){return Store_UI_RegisterNew(n,t,f,v,d,s);}
static void MS again(const dvar_t *p,const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *s){Store_UI_Reregister(p,n,t,f,v,d,s);}
static const dvar_t *MS flt(const char *n,float a,float b,float c,unsigned f,const char *s){return _Dvar_RegisterFloat(n,a,b,c,f,s);}
static const dvar_t *MS str(const char *n,const char *s,unsigned f){return Store_UI_RegisterString(n,s,f);}
static const dvar_t *MS color(const char *n,float r,float g,float b,float a,unsigned f){return Store_UI_RegisterColor(n,r,g,b,a,f);}
static const dvar_t *MS boolean(const char *n,bool b,unsigned f,const char *s){return _Dvar_RegisterBool(n,b,f,s);}
static void MS other(){Store_UI_RegisterRelatedDvars();}
static void MS tail(){Store_UI_RegisterTailDvars();}
extern "C" void *UiRegistrationMock(unsigned i){void *v[]={(void*)lookup,(void*)fresh,(void*)again,(void*)flt,(void*)str,(void*)color,(void*)boolean,(void*)other,(void*)tail};return v[i];}
extern "C" unsigned UiRegistrationCaptureSize(){return sizeof(Capture);}
extern "C" void UiRegistrationRun(void *entry,unsigned s,bool draw,Capture *out){cap={};seed=s;ix=0;
#define CLEAR(name) store_uiDvar_##name=nullptr;
STORE_UI_DVAR_GLOBALS(CLEAR)
#undef CLEAR
if(entry)reinterpret_cast<void(MS*)()>(entry)();else if(draw)UI_RegisterDvars();else UI_RegisterDvarsWithBuildNumberDefault(false);*out=cap;}
extern "C" unsigned UiRegistrationGlobals(unsigned *out){unsigned i=0;
#define GET(name) out[i++]=val(store_uiDvar_##name);
STORE_UI_DVAR_GLOBALS(GET)
#undef GET
return i;}
