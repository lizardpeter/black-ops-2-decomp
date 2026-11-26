#include "gfx_d3d/r_image_release.h"
#include "universal/dvar.h"
#include <cstring>
#include <cstdint>
#include <cstdlib>
extern "C" {
unsigned store_imageTotalMemory[2], store_imageLoadedBytes;
StoreRenderTarget store_renderTargets[72];
unsigned char store_extraImageA[0x68], store_extraImageB[0x68], store_extraImageC[0x68];
unsigned long long store_renderTargetShutdownState;
}
static dvar_t var; const dvar_t *store_imageCreateDvar;
struct Object {void **vtable;unsigned id;};
struct Capture {unsigned count,bad,events[300],total[2],loaded;unsigned long long shutdown;unsigned char image[75][104], targets[72][32];};
static Capture cap; static void *slots[3]; static Object objects[216+3];
static unsigned char images[72][104];
static void ev(unsigned n){if(cap.count<300)cap.events[cap.count++]=n;else ++cap.bad;}
static unsigned __attribute__((ms_abi)) release(void *obj){ev(static_cast<Object *>(obj)->id);return 0;}
static int __attribute__((ms_abi)) parse(const char *s){ev(999);return atoi(s);}
bool Dvar_GetBool(const dvar_t *d){return d&&(d->type==1?d->current.enabled:parse(d->current.string)!=0);}
extern "C" void *ParseMock(){return reinterpret_cast<void *>(parse);}
static unsigned next(unsigned &s){s=s*1664525u+1013904223u;return s;}
static void init(unsigned char *p,unsigned &s,Object *v,unsigned track,unsigned delay){for(unsigned i=0;i<104;++i)p[i]=next(s)>>24;void *vp=v;memcpy(p,&vp,8);p[15]=track;p[11]=delay;}
extern "C" void Run(void *entry,void *globals,unsigned seed,unsigned full,unsigned track,unsigned hasView,unsigned delay,unsigned mode,Capture *out){
    cap={};slots[2]=reinterpret_cast<void *>(release);for(unsigned i=0;i<219;++i)objects[i]={slots,i+1};
    var={};var.type=mode>=3?8:1;if(mode>=3)var.current.string=mode==3?"0":"-5";else var.current.enabled=mode==2;store_imageCreateDvar=mode?&var:nullptr;
    store_imageTotalMemory[0]=next(seed);store_imageTotalMemory[1]=next(seed);store_imageLoadedBytes=next(seed);store_renderTargetShutdownState=0xdeadbeef12345678ull;
    auto *g=static_cast<unsigned char *>(globals);auto *targets=entry?reinterpret_cast<StoreRenderTarget *>(g+0x100):store_renderTargets;
    unsigned char *extra[]={entry?g+0xb00:store_extraImageA,entry?g+0xc00:store_extraImageB,entry?g+0xd00:store_extraImageC};
    for(unsigned i=0;i<72;++i){init(images[i],seed,(next(seed)>>24)&1?&objects[i*3]:nullptr,next(seed)>>24,next(seed)>>31);targets[i]={((next(seed)>>24)&1)?reinterpret_cast<GfxImage *>(images[i]):nullptr,((next(seed)>>24)&1)?&objects[i*3+1]:nullptr,((next(seed)>>24)&1)?&objects[i*3+2]:nullptr,static_cast<unsigned short>(next(seed)),static_cast<unsigned short>(next(seed)),next(seed)};}
    for(unsigned i=0;i<3;++i)init(extra[i],seed,&objects[216+i],next(seed)>>24,next(seed)>>31);
    if(!full)init(images[0],seed,hasView?&objects[0]:nullptr,track,delay);
    if(entry){memcpy(g,store_imageTotalMemory,8);memcpy(g+8,&store_imageLoadedBytes,4);memcpy(g+16,&store_imageCreateDvar,8);memcpy(g+32,&store_renderTargetShutdownState,8);
       if(full)reinterpret_cast<void (__attribute__((ms_abi)) *)()>(entry)();else reinterpret_cast<void (__attribute__((ms_abi)) *)(void *)>(entry)(images[0]);
       memcpy(cap.total,g,8);memcpy(&cap.loaded,g+8,4);memcpy(&cap.shutdown,g+32,8);
    }else{if(full)sub_140753660();else Image_Release(reinterpret_cast<GfxImage *>(images[0]));memcpy(cap.total,store_imageTotalMemory,8);cap.loaded=store_imageLoadedBytes;cap.shutdown=store_renderTargetShutdownState;}
    memcpy(cap.image,images,sizeof(images));for(unsigned i=0;i<3;++i)memcpy(cap.image[72+i],extra[i],104);
    // Pointer fields vary by test mapping; compare exact records after shutdown only.
    if(full)memcpy(cap.targets,targets,sizeof(cap.targets));
    *out=cap;
}
extern "C" unsigned CaptureSize(){return sizeof(Capture);}
