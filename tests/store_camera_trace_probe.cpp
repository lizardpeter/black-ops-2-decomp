#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_third_person_trace_store.cpp"
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_trace {
alignas(16) static Byte state[0x48600];
static unsigned key;static U32 fractions[2];
struct Event {U ignore,mask,start[3],end[3],context[2];};
struct Trace {U count;Event events[2];U fraction,output[3];};static Trace trace;
extern "C" {
void Trace_Query(TraceResult*r,const float*a,const float*mins,const float*maxs,const float*b,int ignore,int mask,TraceContext*c){
    assert(trace.count<2);
    auto &e=trace.events[trace.count];e.ignore=U32(ignore);e.mask=U32(mask);
    for(unsigned i=0;i<3;++i){assert(mins[i]==-6&&maxs[i]==6);e.start[i]=Bits(a[i]);e.end[i]=Bits(b[i]);}
    e.context[0]=R<U32>(c,0x20);e.context[1]=R<U32>(c,0x24);
    W(r,0x10,fractions[trace.count]);W(r,0x2b,Byte((key>>4)&1));
    // Verify that the same context survives a possible second trace.
    W(c,0x20,0x1234);W(c,0x24,0x5678);
    ++trace.count;
}
}
static void MS MQuery(TraceResult*r,const float*a,const float*mi,const float*ma,const float*b,int i,int m,TraceContext*c){Trace_Query(r,a,mi,ma,b,i,m,c);}
extern "C" {
void*TraceMock(){return reinterpret_cast<void*>(MQuery);}
unsigned TraceSize(){return sizeof(Trace);}
void TraceRun(void*image,U32 seed,U32 f1,U32 f2,void*out){
    key=seed;fractions[0]=f1;fractions[1]=f2;trace={};
    W(state,0x481f4,(key&1)?17:0x3ff);W(state,0x481a4,U32((key&2)?0x4000:0));
    W(state,0x4851c,std::int16_t(23));W(state,0x48248,31);
    float a[]={10,20,30},b[]={float(int((key>>8)&15)-8)*10,float(int((key>>12)&15)-8)*10,50};
    if(key&32)std::memcpy(b,a,sizeof(b));
    float dest[3]={-1,-2,-3};
    float *d=key&64?a:dest;
    const Byte lift=(key>>2)&1,allow=(key>>3)&1;
    const float f=image?reinterpret_cast<float(MS*)(Byte*,const float*,const float*,int,float*,Byte,Byte)>(static_cast<Byte*>(image)+0xc69f0)(state,a,b,0x810811,d,lift,allow):
        ThirdPersonViewTrace(state,a,b,0x810811,d,lift,allow);
    trace.fraction=Bits(f);for(unsigned i=0;i<3;++i)trace.output[i]=Bits(d[i]);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
