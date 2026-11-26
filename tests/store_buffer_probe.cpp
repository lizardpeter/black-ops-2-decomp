#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_buffer_store.h"
#include "universal/dvar.h"
#include <cstring>
#include <cstdlib>
#include <csetjmp>
extern "C" {GfxVertexBufferState store_skinnedVertexPool[2];}
int store_142155DFC;
static void *ds[4],*rs[3];static void **dev=ds,**res=rs;
ID3D11Device *device=reinterpret_cast<ID3D11Device *>(&dev);
static dvar_t var;const dvar_t *store_imageCreateDvar;
struct Event {unsigned id,words[9];};
struct Capture {unsigned n,bad,fatal;Event events[24];GfxVertexBufferState pool[2],state;ID3D11Buffer *out;};
static Capture cap;static int *retry;static unsigned repeat,left,failAt,allocs;static unsigned char data[16];static GfxVertexBufferState *pool;static jmp_buf escape;
static Event &ev(unsigned id){auto &e=cap.events[cap.n++];e.id=id;return e;}
static int WINAPI parse(const char *s){ev(1);return atoi(s);}
bool Dvar_GetBool(const dvar_t *d){return d&&(d->type==1?d->current.enabled:parse(d->current.string)!=0);}
static int WINAPI create(void *self,const unsigned *desc,const void *init,void **out){auto &e=ev(2);memcpy(e.words,desc,24);e.words[6]=init!=nullptr;if(init){void *ptr;memcpy(&ptr,init,8);e.words[7]=ptr==data?1:ptr?2:0;unsigned tail[2];memcpy(tail,static_cast<const unsigned char *>(init)+8,8);if(tail[0]||tail[1])++cap.bad;}if(self!=&dev)++cap.bad;*out=&res;*retry=--left?1:0;if(!left)left=repeat;return *retry?-1:0;}
static unsigned WINAPI release(void *self){ev(3);if(self!=&res)++cap.bad;return 0;}
extern "C" void *Store_SkinVirtualAlloc(int bytes){ev(4).words[0]=bytes;if(++allocs==failAt)return nullptr;return data;}
extern "C" [[noreturn]] void Store_SkinAllocFatal(){ev(5);cap.fatal=1;longjmp(escape,1);}
int VirtualFree(void *p,__SIZE_TYPE__ bytes,unsigned flags){auto &e=ev(6);e.words[0]=p==data?1:p?2:0;e.words[1]=bytes;e.words[2]=flags;return 1;}
static void *WINAPI alloc(int b){return Store_SkinVirtualAlloc(b);}
static void WINAPI fatal(){Store_SkinAllocFatal();}
static int WINAPI freeMemory(void *p,__SIZE_TYPE__ b,unsigned f){return VirtualFree(p,b,f);}
extern "C" void *MockBuffer(unsigned i){ds[3]=(void*)create;rs[2]=(void*)release;switch(i){case 0:return &dev;case 1:return (void*)parse;case 2:return (void*)alloc;case 3:return (void*)fatal;default:return (void*)freeMemory;}}
extern "C" void RunBuffer(void *entry,void *globals,unsigned kind,unsigned value,unsigned config,unsigned initial,unsigned repeats,unsigned fail,unsigned pattern,Capture *out){
    cap={};auto *g=static_cast<unsigned char *>(globals);retry=entry?reinterpret_cast<int *>(g+8):&store_142155DFC;*retry=0;repeat=left=repeats;allocs=0;failAt=fail;pool=entry?reinterpret_cast<GfxVertexBufferState *>(g+0x100):store_skinnedVertexPool;
    var={};var.type=config<3?1:8;if(config<3)var.current.enabled=config==2;else var.current.string=config==3?"0":"-3";store_imageCreateDvar=config?&var:nullptr;if(entry)memcpy(g+16,&store_imageCreateDvar,8);
    for(unsigned i=0;i<2;++i)pool[i]={i+17,i+37,(pattern&(1u<<i))?reinterpret_cast<ID3D11Buffer *>(&res):nullptr,(pattern&(4u<<i))?data:nullptr};
    cap.state=pool[0];cap.out=reinterpret_cast<ID3D11Buffer *>(&res);if(initial)cap.out=nullptr;
    unsigned def[3]={0,0,value};
    if(!setjmp(escape)) {
        if(entry){if(kind<2)reinterpret_cast<void(WINAPI *)(ID3D11Buffer **,void *,int)>(entry)(&cap.out,initial?data:nullptr,value);else if(kind==2)reinterpret_cast<void(WINAPI *)(GfxVertexBufferState *,int)>(entry)(&cap.state,value);else if(kind==3)reinterpret_cast<void(WINAPI *)(void *)>(entry)(def);else reinterpret_cast<void(WINAPI *)()>(entry)();}
        else {if(kind==0)Load_IndexBuffer(&cap.out,initial?data:nullptr,value);else if(kind==1)Load_VertexBuffer(&cap.out,initial?data:nullptr,value);else if(kind==2)R_InitDynamicVertexBufferState(&cap.state,value);else if(kind==3)R_InitSkinnedVertexBuffers(reinterpret_cast<SkinnedVertsDef *>(def));else R_ShutdownSkinnedVertexBuffers();}
    }
    memcpy(cap.pool,pool,sizeof(cap.pool));*out=cap;
}
extern "C" unsigned BufferCaptureSize(){return sizeof(Capture);}
