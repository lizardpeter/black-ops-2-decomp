#include "gfx_d3d/r_state_store.h"
#include "gfx_d3d/r_image_release.h"
#include <cstring>
#include <cstdint>
extern "C" {StoreRenderTarget store_renderTargets[72];unsigned store_shaderImageSlotCount;}
#define MS __attribute__((ms_abi))
struct Event {unsigned id, data[8];};struct Capture {unsigned n,bad;Event events[32];unsigned char source[0x1b70],state[0x5420];};
static Capture cap;static void *slots[54],**dev=slots;static unsigned char input[0x150];static float color[4];static StoreRenderTarget *table;
static Event &ev(unsigned n,void *self){if(self!=&dev)++cap.bad;auto &e=cap.events[cap.n++];e.id=n;return e;}
static void MS unbind(void *self,unsigned i,unsigned count,void *const *p){auto &e=ev(8,self);e.data[0]=i;e.data[1]=count;if(*p)++cap.bad;}
static void MS bind(void *self,unsigned n,void *const *p,void *d){auto &e=ev(33,self);e.data[0]=n;e.data[1]=reinterpret_cast<uintptr_t>(*p);e.data[2]=reinterpret_cast<uintptr_t>(d);}
static void MS viewport(void *self,unsigned n,const float *v){auto &e=ev(44,self);e.data[0]=n;memcpy(e.data+1,v,24);}
static void MS clearColor(void *self,void *v,const float *c){auto &e=ev(50,self);e.data[0]=reinterpret_cast<uintptr_t>(v);memcpy(e.data+1,c,16);}
static void MS clearDepth(void *self,void *v,unsigned flags,float depth,unsigned char stencil){auto &e=ev(53,self);e.data[0]=reinterpret_cast<uintptr_t>(v);e.data[1]=flags;memcpy(e.data+2,&depth,4);e.data[3]=stencil;}
template<class T>static void put(void *p,unsigned off,T v){memcpy(static_cast<unsigned char *>(p)+off,&v,sizeof(v));}
extern "C" void RunState(void *entry,void *globals,unsigned kind,unsigned seed,unsigned mode,unsigned mask,unsigned pattern,unsigned count,Capture *out){
 cap={};slots[8]=(void*)unbind;slots[33]=(void*)bind;slots[44]=(void*)viewport;slots[50]=(void*)clearColor;slots[53]=(void*)clearDepth;
 auto *g=static_cast<unsigned char *>(globals);table=entry?reinterpret_cast<StoreRenderTarget *>(g+0x100):store_renderTargets;store_shaderImageSlotCount=count;if(entry)memcpy(g,&count,4);
 auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
 for(auto &b:cap.source)b=next()>>24;
 for(auto &b:cap.state)b=next()>>24;
 for(auto &b:input)b=next()>>24;
 for(unsigned i=0;i<72;++i)table[i]={reinterpret_cast<GfxImage *>(uintptr_t(1+i%5)),reinterpret_cast<void *>(uintptr_t((i%4)*16)),reinterpret_cast<void *>(uintptr_t((i%3)*32)),static_cast<unsigned short>(next()),static_cast<unsigned short>(next()),next()};
 unsigned char old=pattern%12,id=mode%12;put(cap.state,0x5404,old);put(cap.state,0xd0,&dev);for(unsigned i=0;i<16;++i)put(cap.state,0x50+8*i,reinterpret_cast<GfxImage *>(uintptr_t(next()%6)));
 if(pattern&16)table[id].image=nullptr;
 if(pattern&32)table[id].colorView=table[old].colorView;
 if(pattern&64)table[id].depthView=table[old].depthView;
 for(unsigned i=0;i<3;++i){put(cap.source,0x1ac0+i*4,static_cast<float>(int(next()%1000)-500)/8);put(input,0x100+i*4,static_cast<float>(int(next()%1000)-500)/8);}
 unsigned flags[]={0,0x80000000,0x3f800000,0xbf800000,0x7fc00000};put(input,0x10c,flags[pattern%5]);put(cap.source,0x1ae0,input+0);put(cap.source,0x1b10,mode%3);
 for(auto &v:color)v=static_cast<float>(next()%1024)/1024;
 float depth=static_cast<float>(next()%1024)/1024;auto *s=reinterpret_cast<GfxCmdBufSourceState *>(cap.source);auto *st=reinterpret_cast<GfxCmdBufState *>(cap.state);StoreCmdBufContext ctx{s,st};
 if(entry){if(kind<2)reinterpret_cast<void(MS *)(GfxCmdBufSourceState *)>(entry)(s);else if(kind==2)reinterpret_cast<void(MS *)(const StoreCmdBufContext *,unsigned char)>(entry)(&ctx,id);else reinterpret_cast<void(MS *)(GfxCmdBufState *,unsigned char,const float *,float,unsigned char)>(entry)(st,mask,color,depth,pattern);}
 else{if(kind==0)Store_ResetWorldMatrix(s);else if(kind==1)Store_Set3DView(s);else if(kind==2)R_SetRenderTarget(&ctx,id);else Store_ClearScreen(st,mask,color,depth,pattern);}
 *out=cap;
}
extern "C" unsigned StateCaptureSize(){return sizeof(Capture);}
