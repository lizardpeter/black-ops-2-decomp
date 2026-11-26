#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_release.h"
#include "gfx_d3d/r_render_target_create.h"
#include "gfx_d3d/r_extra_cam_targets.h"
#include <cstring>
extern "C" {StoreRenderTarget store_renderTargets[72]; StoreExtraCamSettings store_extraCamSettings; unsigned store_depthTextureFormat;}
int store_142155DFC;
unsigned multiSampleType;
static void *ds[11],*rs[3]; static void **dev=ds,**res=rs;
ID3D11Device *device=reinterpret_cast<ID3D11Device *>(&dev);
struct Event {unsigned id,args[12];};
struct Capture {unsigned n,bad;Event events[100];unsigned char state[24], targets[72*32];};
static Capture cap;static StoreRenderTarget *targets,*active; static int *retry;static unsigned cr,vr,cl,vl;
static Event &event(unsigned id){auto &e=cap.events[cap.n++];e.id=id;return e;}
extern "C" void Store_ImageSetupRenderTarget(int index,unsigned w,unsigned h,unsigned fmt,int color,StoreRenderTarget *t){auto &e=event(1);e.args[0]=index;e.args[1]=w&65535;e.args[2]=h&65535;e.args[3]=fmt;e.args[4]=color;e.args[5]=t-targets;active=t; t->width=w;t->height=h;cl=cr;vl=vr;}
static void WINAPI setup(int i,unsigned w,unsigned h,unsigned f,int c,StoreRenderTarget *t){Store_ImageSetupRenderTarget(i,w,h,f,c,t);}
static int WINAPI create(void *self,const unsigned *d,const void *data,void **out){auto &e=event(2);memcpy(e.args,d,44);if(self!=&dev||data)++cap.bad;*out=&res;*retry=--cl?1:0;return cl?-1:0;}
static int WINAPI depth(void *self,void *r,const unsigned *d,void **out){auto &e=event(3);memcpy(e.args,d,24);if(self!=&dev||r!=&res||out!=&active->depthView)++cap.bad;*out=&res;*retry=--vl?1:0;return vl?-1:0;}
static unsigned WINAPI release(void *self){event(4).args[0]=self==&res?0:1;return 0;}
void Image_Release(GfxImage *im){event(5).args[0]=static_cast<unsigned>(reinterpret_cast<__UINTPTR_TYPE__>(im));}
static void WINAPI releaseImage(GfxImage *im){Image_Release(im);}
extern "C" void *MockExtra(unsigned i){ds[5]=(void*)create;ds[10]=(void*)depth;rs[2]=(void*)release;switch(i){case 0:return &dev;case 1:return (void*)setup;default:return (void*)releaseImage;}}
extern "C" void RunExtra(void *entry,void *globals,unsigned mode,unsigned seed,unsigned w,unsigned h,unsigned mw,unsigned mh,unsigned f,unsigned repeats,Capture *out){
    cap={}; auto *g=static_cast<unsigned char *>(globals);targets=entry?reinterpret_cast<StoreRenderTarget *>(g+0x100):store_renderTargets;auto *state=entry?reinterpret_cast<StoreExtraCamSettings *>(g+0x20):&store_extraCamSettings;
    retry=entry?reinterpret_cast<int *>(g+8):&store_142155DFC;*retry=0;store_depthTextureFormat=f;if(entry)memcpy(g+12,&f,4);cr=1+repeats%3;vr=1+(repeats/3)%3;
    memset(state,0xa5,24);memset(targets,0,sizeof(store_renderTargets));
    if(mode==2){for(unsigned i=0;i<72;++i){seed=seed*1664525u+1013904223u;targets[i].image=(seed&1)?reinterpret_cast<GfxImage *>(static_cast<__UINTPTR_TYPE__>(i+1)):nullptr;targets[i].colorView=(seed&2)?&res:nullptr;targets[i].depthView=(seed&4)?&res:nullptr;targets[i].width=seed;targets[i].height=seed>>16;targets[i].tail=seed;}}
    if(entry){if(mode==0)reinterpret_cast<void(WINAPI *)(int,int,int,unsigned,unsigned,unsigned,StoreRenderTarget *)>(entry)(29,w,h,0,f,0,&targets[19]);else if(mode==1)reinterpret_cast<void(WINAPI *)(unsigned,unsigned,unsigned,unsigned,int)>(entry)(w,h,mw,mh,seed);else reinterpret_cast<void(WINAPI *)()>(entry)();}
    else {if(mode==0)Store_InitSizedTargetWithDepth(29,w,h,0,f,0,&targets[19]);else if(mode==1)R_InitExtraCamRenderTargets(w,h,mw,mh,seed);else R_ShutdownExtraCamRenderTargets();}
    memcpy(cap.state,state,24);memcpy(cap.targets,targets,sizeof(cap.targets));*out=cap;
}
extern "C" unsigned ExtraCaptureSize(){return sizeof(Capture);}
