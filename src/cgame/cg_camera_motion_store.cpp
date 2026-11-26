// Store MP camera motion and orientation helpers. Recovered from the x64
// bodies, retaining their float rounding order and packed CRT boundaries.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_camera_motion {
using Byte=unsigned char;
using U32=std::uint32_t;
extern "C" {
extern Byte *Motion_State; // 140E53A20
extern Byte *Motion_EntityBase[]; // 140E4F8E0, client 0
extern const char *Motion_ErrorText; // 1408E2000
__m128 Motion_SinCosPair(float yaw,float pitch); // 140890DA3: sinY,cosY,sinP,cosP
__m128 Motion_SinCosSingle(float roll); // 140890D60: sinR,cosR
float Motion_Cos(float); // 140870BC2
void Motion_MatrixMultiply(const float*,const float*,float*); // 140461740
bool Motion_ErrorCheck(); // 1400A9960
void Motion_Error(int,const char*); // 14039AEB0
void Motion_EntityTween(int,int,U32,int,int,float*,float*); // 1406F3BA0
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 b){float f;std::memcpy(&f,&b,4);return f;}
static U32 Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static float Neg(float f){return F(Bits(f)^0x80000000u);}
// Preserve the destination operand of scalar SSE arithmetic when both
// operands carry NaNs. C++ commutative multiplication may swap operands.
static float MulOrdered(float a,float b){__asm__("mulss %1,%0":"+x"(a):"x"(b));return a;}
static float SubOrdered(float a,float b){__asm__("subss %1,%0":"+x"(a):"x"(b));return a;}
static float Abs(float f){return F(Bits(f)&0x7fffffffu);}
static int Diff(int a,int b){U32 v=U32(a)-U32(b);int i;std::memcpy(&i,&v,4);return i;}
static float Sqrt(float f){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(f)));}
struct Trig{float sy,cy,sp,cp;};
static Trig Pair(float pitch,float yaw){
    alignas(16) float out[4];
    _mm_store_ps(out,Motion_SinCosPair(yaw*F(0x3c8efa35),pitch*F(0x3c8efa35)));
    return {out[0],out[1],out[2],out[3]};
}
// 140464D00..140464E52.
void AnglesToAxis(const float*angles,float*axis){
    const Trig t=Pair(angles[0],angles[1]);
    axis[0]=t.cp*t.cy;axis[1]=t.cp*t.sy;axis[2]=Neg(t.sp);
    alignas(16) float roll[4];
    _mm_store_ps(roll,Motion_SinCosSingle(angles[2]*F(0x3c8efa35)));
    const float sr=roll[0],cr=roll[1];
    const float a=sr*t.sp,b=cr*t.sp;
    axis[3]=a*t.cy-cr*t.sy;axis[4]=a*t.sy+cr*t.cy;axis[5]=sr*t.cp;
    axis[6]=b*t.cy+sr*t.sy;axis[7]=b*t.sy-sr*t.cy;axis[8]=cr*t.cp;
}
// 14016C380..14016C50C. The original uses elapsed*time-factor, not
// displacement divided by elapsed time. The angular field is a dot product.
void UpdateCameraVelocity(Byte*cg){
    float scale=float(Diff(R<int>(cg,0x4808c),R<int>(cg,0x64a04)))*F(0x3d75dbbd);
    if(!(scale>0.0f))scale=0.0f;
    for(unsigned i=0;i<3;++i)W(cg,0x4dbb4+4*i,(R<float>(cg,0x4d924+4*i)-R<float>(cg,0x649dc+4*i))*scale);
    const Trig cur=Pair(R<float>(cg,0x649d0),R<float>(cg,0x649d4));
    const Trig prev=Pair(R<float>(cg,0x649e8),R<float>(cg,0x649ec));
    const float x=prev.cy*prev.cp*(cur.cy*cur.cp);
    const float y=prev.sy*prev.cp*(cur.sy*cur.cp);
    W(cg,0x4dbc0,(x+y)+prev.sp*cur.sp);
}
// 140157BC0..140157D16. Original normalizes the perturbed forward
// direction and the horizontal left vector before constructing the third.
void PerturbCamera(Byte*cg){
    const float y=R<float>(cg,0x6b790),z=R<float>(cg,0x6b794);
    if(!(y<0.0f||y>0.0f) && !(z<0.0f||z>0.0f))return;
    float norm=Sqrt((y*y+1.0f)+z*z);
    if(!(norm>0.0f))norm=1.0f;
    const float x=1.0f/norm,ny=y*x,nz=z*x;
    float horiz=Sqrt(ny*ny+x*x);
    if(!(horiz>0.0f))horiz=1.0f;
    const float inv=1.0f/horiz,lx=Neg(inv*ny),ly=x*inv;
    const float perturb[9]={x,ny,nz,lx,ly,0.0f,Neg(ly*nz),MulOrdered(nz,lx),SubOrdered(MulOrdered(ly,x),MulOrdered(ny,lx))};
    float axis[9];std::memcpy(axis,cg+0x4d934,sizeof(axis));
    Motion_MatrixMultiply(perturb,axis,reinterpret_cast<float*>(cg+0x4d934));
}
static float Pitch90(float a){while(Abs(a)>90.0f)a-=a>0.0f?180.0f:-180.0f;return a;}
// 1400BF940..1400BFCB5. The original yaw loop uses the sign of the
// initial difference for every iteration; preserve this rather than fmod.
void UpdateCameraTween(int client){
    Byte*cg=Motion_State;
    const int now=R<int>(cg,0x4808c);
    float elapsed=float(Diff(now,R<int>(cg,0x64a28)))*F(0x3a83126f);
    if(!(elapsed>=R<float>(cg,0x64a2c))){
        const int entity=R<std::int16_t>(cg,0x4851c);
        if(entity!=0x3ff){
            if(Motion_ErrorCheck())Motion_Error(1,Motion_ErrorText);
            const auto addr=reinterpret_cast<std::uintptr_t>(Motion_EntityBase[client])+std::intptr_t(entity)*0x3d8;
            if(addr){
                const Byte*ent=reinterpret_cast<const Byte*>(addr);
                Motion_EntityTween(0,client,R<U32>(ent,0x1f4),R<int>(cg,0x64a04),R<int>(cg,0x4808c),
                    reinterpret_cast<float*>(cg+0x64a0c),reinterpret_cast<float*>(cg+0x64a18));
            }
        }
        const float pi=F(0x40490fdb);
        elapsed=(elapsed/R<float>(cg,0x64a2c))*pi-pi;
        const float start[3]={R<float>(cg,0x64a0c),R<float>(cg,0x64a10),R<float>(cg,0x64a14)};
        const float old[3]={R<float>(cg,0x64a18),R<float>(cg,0x64a1c),R<float>(cg,0x64a20)};
        const float t=(Motion_Cos(elapsed)+1.0f)*0.5f;
        for(unsigned i=0;i<3;++i)W(cg,0x4d924+4*i,(R<float>(cg,0x4d924+4*i)-start[i])*t+start[i]);
        const float pitch=Pitch90(old[0]);
        const float targetPitch=Pitch90(R<float>(cg,0x649d0));
        const float yaw=R<float>(cg,0x649d4)-old[1];
        float delta=yaw;
        while(Abs(delta)>180.0f)delta-=yaw>0.0f?360.0f:-360.0f;
        const float roll=R<float>(cg,0x649d8)-old[2];
        W(cg,0x649d0,(targetPitch-pitch)*t+pitch);
        W(cg,0x649d8,roll*t+old[2]);
        W(cg,0x649d4,delta*t+old[1]);
    }
    for(unsigned i=0;i<3;++i){
        W(cg,0x649dc+4*i,R<U32>(cg,0x4d924+4*i));
        W(cg,0x649e8+4*i,R<U32>(cg,0x649d0+4*i));
    }
    W(cg,0x64a04,R<int>(cg,0x4808c));
}
}
#endif
