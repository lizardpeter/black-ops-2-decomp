#include "gfx_d3d/r_dx.h"
#include "universal/win_common.h"
#include <cstring>
#include <cstdint>
struct GfxImage;
extern "C" void Store_ImageUploadData(GfxImage *,unsigned,int,int,const unsigned char *);
struct Capture {unsigned count, bad, events[64], sub[16], row, depth, mapped; unsigned char dst[16][2048];};
static Capture cap;
static void *vs[8],*rs[11],*cs[49];
static void **view=vs,**resource=rs,**ctx=cs;
ID3D11DeviceContext *context=reinterpret_cast<ID3D11DeviceContext *>(&ctx);
static unsigned char source[65536];
static unsigned pitch,levels;
static void ev(unsigned n){if(cap.count<64)cap.events[cap.count++]=n;else ++cap.bad;}
void Sys_EnterCriticalSection(CriticalSection s){if(s!=CRITSECT_DXCONTEXT)++cap.bad;ev(3);}
void Sys_LeaveCriticalSection(CriticalSection s){if(s!=CRITSECT_DXCONTEXT)++cap.bad;ev(5);}
static void WINAPI enter(void*){Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);}
static void WINAPI leave(void*){Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);}
static void WINAPI get(void *s,void **o){ev(1);if(s!=&view)++cap.bad;*o=&resource;}
static void WINAPI desc(void *s,unsigned *o){ev(2);if(s!=&resource)++cap.bad;memset(o,0,44);o[2]=levels;}
static unsigned WINAPI release(void *s){ev(6);if(s!=&resource)++cap.bad;return 1;}
static int WINAPI map(void *s,void *r,unsigned sub,unsigned type,unsigned flags,D3D11_MAPPED_SUBRESOURCE *o){ev(7);if(s!=&ctx||r!=&resource||type!=4||flags||cap.mapped>=16)++cap.bad;unsigned i=cap.mapped++;cap.sub[i]=sub;*o={cap.dst[i],pitch,2048};return 0;}
static void WINAPI unmap(void *s,void *r,unsigned sub){ev(8);if(s!=&ctx||r!=&resource||sub!=cap.sub[cap.mapped-1])++cap.bad;}
static void WINAPI update(void *s,void *r,unsigned sub,const void *box,const void *data,unsigned row,unsigned depth){ev(4);if(s!=&ctx||r!=&resource||box||data!=source)++cap.bad;cap.sub[0]=sub;cap.row=row;cap.depth=depth;}
static void *WINAPI copy(void *d,const void *s,__SIZE_TYPE__ n){return memcpy(d,s,n);}
extern "C" void *UploadMock(unsigned i){vs[7]=(void*)get;rs[10]=(void*)desc;rs[2]=(void*)release;cs[14]=(void*)map;cs[15]=(void*)unmap;cs[48]=(void*)update;switch(i){case 0:return &ctx;case 1:return (void*)enter;case 2:return (void*)leave;default:return (void*)copy;}}
extern "C" void CaptureFull(void *entry,unsigned type,unsigned w,unsigned h,unsigned d,unsigned fmt,unsigned face,unsigned mip,unsigned n,unsigned p,Capture *out){
    cap={};memset(cap.dst,0xa5,sizeof(cap.dst));for(unsigned i=0;i<sizeof(source);++i)source[i]=static_cast<unsigned char>(i*17+i/251);pitch=p;levels=n;
    alignas(8) unsigned char img[64]={};void *v=&view;memcpy(img,&v,8);img[8]=type;unsigned short dims[]={static_cast<unsigned short>(w),static_cast<unsigned short>(h),static_cast<unsigned short>(d)};memcpy(img+24,dims,6);img[30]=n;
    auto *im=reinterpret_cast<GfxImage *>(img);
    if(entry)reinterpret_cast<void (WINAPI *)(GfxImage *,unsigned,int,int,const unsigned char *)>(entry)(im,fmt,face,mip,source);else Store_ImageUploadData(im,fmt,face,mip,source);
    *out=cap;
}
extern "C" unsigned CaptureSize(){return sizeof(Capture);}
