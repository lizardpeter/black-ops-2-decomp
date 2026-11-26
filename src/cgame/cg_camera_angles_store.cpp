// Store MP axis-to-Euler conversion and linked view roll transformation.
// Complete bodies at 140462D70..140462EBD and 140621C10..140621E33.
#define T6_SOURCE "universal/com_math.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_camera_angles {
using Byte=unsigned char;using U32=std::uint32_t;
struct Dvar;
extern "C" {
void Angle_VectorToAngles(const float*,float*); // 140461020
__m128 Angle_SinCos(float,float); // 140890DA3
float Angle_Atan2(float,float); // 140870BBC
void Angle_ToAxis(const float*,float*); // 140464D00
void Angle_FromAxis(const float*,float*); // 140462D70, linked transform boundary
extern const Dvar *Angle_RollLimit; // 14265CF30
double Angle_Atof(const char*); // IAT 1408A7DD8
}
static float F(U32 n){float f;std::memcpy(&f,&n,4);return f;}
static float Neg(float f){U32 n;std::memcpy(&n,&f,4);return F(n^0x80000000u);}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
static float Float(const Dvar*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(t==2)return R<float>(d,0x20);
    if(t==6)return float(R<int>(d,0x20));
    return float(Angle_Atof(R<const char*>(d,0x20)));
}
void AxisToAngles(const float*a,float*out){
    Angle_VectorToAngles(a,out);
    alignas(16) float t[4];
    _mm_store_ps(t,Angle_SinCos(out[1]*F(0xbc8efa35),out[0]*F(0xbc8efa35)));
    const float x=a[3],y=a[4],z=a[5];
    const float v=t[1]*x-t[0]*y;
    const float u=t[1]*y+t[0]*x;
    const float h=z*t[2]+v*t[3],r=z*t[3]-v*t[2];
    float roll;
    if(!(u<0||u>0)&&!(h<0||h>0))roll=r>0?90.0f:-90.0f;
    else {
        const float radius=_mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(h*h+u*u)));
        const float angle=Angle_Atan2(r,radius)*F(0xc2652ee0);
        if(u>=0)roll=Neg(angle);
        else roll=angle>=0?angle-180.0f:angle+180.0f;
    }
    out[2]=roll;
}
void TransformLinkedRoll(const float*linked,float*view){
    float a[9],b[9],c[9],angles[3];
    Angle_ToAxis(linked,a);Angle_ToAxis(view,b);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
        const float y=b[3*i+1]*a[3*j+1];
        const float x=b[3*i]*a[3*j];
        const float z=b[3*i+2]*a[3*j+2];
        c[3*i+j]=(y+x)+z;
    }
    Angle_FromAxis(c,angles);
    const float limit=Float(Angle_RollLimit);
    view[2]=Neg(angles[2]);
    if(view[0]<-90.0f && limit>85.0f)view[2]-=180.0f;
}
}
#endif
