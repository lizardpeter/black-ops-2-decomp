#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_release.h"
#include "gfx_d3d/r_image_prog.h"
#include "gfx_d3d/r_image_builtin.h"
#include "gfx_d3d/r_render_target_create.h"
#include <cstring>
extern "C" {StoreRenderTarget store_renderTargets[72];}
int store_142155DFC;
static void *ds[11],*vs[8],*rs[3];static void **dev=ds,**view=vs,**res=rs;
ID3D11Device *device=reinterpret_cast<ID3D11Device *>(&dev);unsigned multiSampleType;
static unsigned char image[104];static const char *name;static int *retry;static unsigned left;
static StoreRenderTarget *active;
struct Capture {unsigned n,bad,events[16],alloc[3],setup[6],desc[6];unsigned char target[32],img[104];};
static Capture cap;
static void event(unsigned e){if(cap.n<16)cap.events[cap.n++]=e;else ++cap.bad;}
GfxImage *Image_AllocProg(int index,unsigned char category,unsigned char semantic){event(1);cap.alloc[0]=index;cap.alloc[1]=category;cap.alloc[2]=semantic;memcpy(image+0x58,&name,8);return reinterpret_cast<GfxImage *>(image);}
extern "C" void Store_ImageSetup(GfxImage *p,int w,int h,int d,unsigned flags,unsigned format,void *data){event(2);if(p!=reinterpret_cast<GfxImage *>(image)||data||active->image!=p)++cap.bad;cap.setup[0]=w;cap.setup[1]=h;cap.setup[2]=d;cap.setup[3]=flags;cap.setup[4]=format;void *v=&view;memcpy(image,&v,8);}
static GfxImage *WINAPI alloc(int i,unsigned char c,unsigned char s){return Image_AllocProg(i,c,s);}
static void WINAPI setup(GfxImage *p,int w,int h,int d,unsigned flags,unsigned fmt,void *data){Store_ImageSetup(p,w,h,d,flags,fmt,data);}
static int WINAPI compare(const char *a,const char *b){return strcmp(a,b);}
static void WINAPI resource(void *self,void **out){event(3);if(self!=&view)++cap.bad;*out=&res;}
static unsigned WINAPI release(void *self){event(6);if(self!=&res)++cap.bad;return 0;}
static int WINAPI color(void *self,void *r,const unsigned *desc,void **out){event(4);if(self!=&dev||r!=&res||out!=&active->colorView)++cap.bad;memcpy(cap.desc,desc,20);*out=&view;*retry=--left?1:0;return left?-1:0;}
static int WINAPI depth(void *self,void *r,const unsigned *desc,void **out){event(5);if(self!=&dev||r!=&res||out!=&active->depthView)++cap.bad;memcpy(cap.desc,desc,24);*out=&view;*retry=1;return -1;}
extern "C" void *MockTarget(unsigned i){ds[9]=(void*)color;ds[10]=(void*)depth;vs[7]=(void*)resource;rs[2]=(void*)release;switch(i){case 0:return &dev;case 1:return (void*)alloc;case 2:return (void*)setup;default:return (void*)compare;}}
extern "C" void CaptureTarget(void *entry,void *globals,unsigned n,unsigned w,unsigned h,unsigned fmt,unsigned colorTarget,unsigned special,unsigned samples,unsigned repeat,Capture *out){
    static const char *names[]={"$scene","$ssao_csz","$dof_scene_fp16","$scene_extra","$scene_depth","","$SCENE","$ssao_csz_0"};name=names[n];cap={};memset(image,0xa5,sizeof(image));multiSampleType=samples;left=repeat;auto *g=static_cast<unsigned char *>(globals);retry=entry?reinterpret_cast<int *>(g+16):&store_142155DFC;*retry=0;
    StoreRenderTarget local;active=special?(entry?reinterpret_cast<StoreRenderTarget *>(g+0x2080):&store_renderTargets[4]):&local;memset(active,0x5a,sizeof(*active));
    if(entry){memcpy(g+8,&samples,4);reinterpret_cast<void(WINAPI *)(int,unsigned,unsigned,unsigned,int,StoreRenderTarget *)>(entry)(n,w,h,fmt,colorTarget,active);}else Store_ImageSetupRenderTarget(n,w,h,fmt,colorTarget,active);
    memcpy(cap.target,active,32);memcpy(cap.img,image,104);*out=cap;
}
extern "C" unsigned TargetCaptureSize(){return sizeof(Capture);}
