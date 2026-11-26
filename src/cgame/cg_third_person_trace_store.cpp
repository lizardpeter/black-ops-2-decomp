// Store MP ThirdPersonViewTrace, 1400C69F0..1400C6D28.
// Complete enclosing body. Collision query is an external engine boundary.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_camera_trace {
using Byte=unsigned char; using U32=std::uint32_t;
struct alignas(16) TraceResult { Byte bytes[64]; };
struct alignas(8) TraceContext { Byte bytes[64]; };
extern "C" {
void Trace_Query(TraceResult*,const float*,const float*,const float*,const float*,int,int,TraceContext*); // 140190530
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 b){float f;std::memcpy(&f,&b,4);return f;}
static U32 Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static float InvSqrt(float x){
    const U32 bits=Bits(x);
    const float seed=F(0x5f3759dfu-((bits>>1)|(bits&0x80000000u)));
    const float half=x*0.5f;
    return (1.5f-(half*seed)*seed)*seed;
}
static int Ignore(const Byte*cg){
    return R<U32>(cg,0x481a4)&0x4000?R<std::int16_t>(cg,0x4851c):R<int>(cg,0x48248);
}
static void Blend(const float*a,const float*b,float fraction,float*out){
    // Calculate all components before publishing, including the common
    // case where output aliases the original start.
    const float x=(b[0]-a[0])*fraction+a[0];
    const float y=(b[1]-a[1])*fraction+a[1];
    const float z=(b[2]-a[2])*fraction+a[2];
    out[0]=x;out[1]=y;out[2]=z;
}
float ThirdPersonViewTrace(Byte*cg,const float*start,const float*end,int mask,float*out,Byte lift,Byte allowSolid){
    const float mins[3]={-6,-6,-6},maxs[3]={6,6,6};
    TraceResult result{};
    TraceContext context{};
    W(&context,0x20,0x3ff);W(&context,0x24,0x3ff);
    int ignore=R<int>(cg,0x481f4);
    if(ignore==0x3ff)ignore=Ignore(cg);
    Trace_Query(&result,start,mins,maxs,end,ignore,mask,&context);
    if(!allowSolid && R<Byte>(&result,0x2b)){
        const float x=end[0]-start[0],y=end[1]-start[1],z=end[2]-start[2];
        const float inv=InvSqrt((x*x+y*y)+z*z);
        Blend(start,end,inv,out);
        return 0.0f;
    }
    float fraction=R<float>(&result,0x10);
    if(!(fraction<1.0f||fraction>1.0f)){
        std::memcpy(out,end,3*sizeof(float));return fraction;
    }
    float point[3];
    if(fraction>0.001f)Blend(start,end,fraction,point);
    else std::memcpy(point,start,sizeof(point));
    if(!lift){std::memcpy(out,point,sizeof(point));return fraction;}
    point[2]+=(1.0f-fraction)*32.0f;
    Trace_Query(&result,start,mins,maxs,point,Ignore(cg),mask,&context);
    fraction=R<float>(&result,0x10);
    if(fraction>0.001f)Blend(start,point,fraction,out);
    else std::memcpy(out,start,3*sizeof(float));
    return fraction;
}
}
#endif
