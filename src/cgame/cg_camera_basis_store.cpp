// Store MP camera/math helper bodies. Store offsets, constants and
// arithmetic order are retained. The cubemap face order is corroborated
// by the symbol-bearing reference and the Store original replay.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_camera_basis {
using Byte=unsigned char;using U32=std::uint32_t;
struct Dvar;
extern "C" {
float Basis_Atan2(float,float); // 140870BBC
float Basis_Sin(float); // 140870BC8
extern const Dvar *Basis_RemoteAmplitudeA,*Basis_RemoteRateA,*Basis_RemoteLimitsA;
extern const Dvar *Basis_RemoteAmplitudeB,*Basis_RemoteRateB,*Basis_RemoteLimitsB;
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 v){float f;std::memcpy(&f,&v,4);return f;}
static int I(U32 v){int i;std::memcpy(&i,&v,4);return i;}
static float Sqrt(float f){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(f)));}
static float Max(float a,float b){return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a),_mm_set_ss(b)));}
static float Min(float a,float b){return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a),_mm_set_ss(b)));}
static bool Equal(float a,float b){return !(a<b||a>b);}
// 140461020..14046111F. In particular, the vertical-only branch
// uses 90 degrees for nonpositive Z and 270 for positive Z.
void VectorToAngles(const float*v,float*out){
    const float y=v[1],x=v[0];float yaw,pitch;
    if(Equal(y,0.0f)&&Equal(x,0.0f)){
        yaw=0;pitch=-0.0f<v[2]?270.0f:90.0f;
    }else{
        yaw=Basis_Atan2(y,x)*F(0x42652ee0);
        yaw+=yaw>=0.0f?0.0f:360.0f;
        pitch=Basis_Atan2(v[2],Sqrt(x*x+y*y))*F(0xc2652ee0);
        pitch+=pitch>=0.0f?0.0f:360.0f;
    }
    out[1]=yaw;out[0]=pitch;out[2]=0.0f;
}
// 1400DE970..1400DEAA2. DObj tag record contains quaternion, origin,
// and a precomputed scale at byte 0x1c; do NOT normalize the quaternion.
void TagAxis(const Byte*tag,float*out){
    const float x=R<float>(tag,0),y=R<float>(tag,4),z=R<float>(tag,8),w=R<float>(tag,12),s=R<float>(tag,0x1c);
    const float sx=x*s,sy=y*s,sz=z*s;
    const float xx=sx*x,yy=sy*y,zz=sz*z;
    const float xy=y*sx,xz=z*sx,yz=z*sy;
    const float wx=sx*w,wy=sy*w,wz=sz*w;
    out[0]=1.0f-(zz+yy);out[1]=wz+xy;out[2]=xz-wy;
    out[3]=xy-wz;out[4]=1.0f-(zz+xx);out[5]=yz+wx;
    out[6]=wy+xz;out[7]=yz-wx;out[8]=1.0f-(yy+xx);
}
// 14016BDA0..14016BF89. Complete six-face cubemap view setup.
void CalcCubemapViewValues(Byte*cg){
    const int n=R<int>(cg,0x10),size=I(U32(n)+2u);
    W(cg,0x4d8f0,std::uint64_t(0));W(cg,0x4d8f8,size);W(cg,0x4d8fc,size);
    const float slope=float(size)/float(n);
    W(cg,0x4d910,slope);W(cg,0x4d914,slope);
    for(unsigned i=0;i<3;++i)W(cg,0x4d924+4*i,R<U32>(cg,0x480d0+4*i));
    W(cg,0x4d92c,R<float>(cg,0x482b0)+R<float>(cg,0x4d92c));
    static constexpr float faces[6][9]={
        {1,0,0,0,1,0,0,0,1},
        {-1,0,0,0,-1,0,0,0,1},
        {0,1,0,-1,0,0,0,0,1},
        {0,-1,0,1,0,0,0,0,1},
        {0,0,1,0,1,0,-1,0,0},
        {0,0,-1,0,1,0,1,0,0},
    };
    const U32 face=U32(R<int>(cg,0xc))-1;
    if(face<6)for(unsigned i=0;i<9;++i)W(cg,0x4d934+4*i,faces[face][i]);
}
// 1400C33F0..1400C3645. Remote camera oscillation uses separate
// amplitude/rate/speed-window dvars for entity type 12 and other types.
void RemoteViewMotion(Byte*cg,const Byte*entity){
    const bool type12=R<unsigned short>(entity,0x2cc)==12;
    const Dvar *amplitude=type12?Basis_RemoteAmplitudeA:Basis_RemoteAmplitudeB;
    const Dvar *rate=type12?Basis_RemoteRateA:Basis_RemoteRateB;
    const Dvar *limits=type12?Basis_RemoteLimitsA:Basis_RemoteLimitsB;
    float a[3],r[3];
    for(unsigned i=0;i<3;++i){a[i]=amplitude?R<float>(amplitude,0x20+i*4):0;r[i]=rate?R<float>(rate,0x20+i*4):0;}
    float low=Max(limits?R<float>(limits,0x20):0.0f,0.0f);
    float high=limits?R<float>(limits,0x24):0.0f;
    if(!(high>low))high=low+1.0f;
    const float x=R<float>(entity,0x218),y=R<float>(entity,0x21c),z=R<float>(entity,0x220);
    const float speed=Sqrt((x*x+y*y)+z*z);
    if(speed>=low)low=Min(speed,high);
    const float ratio=low/high;
    const float weight=ratio>=1.0f?1.0f:ratio>0.0f?ratio:0.0f;
    const float time=float(R<int>(cg,0x4d95c))*F(0x3b4de32f);
    for(unsigned i=0;i<3;++i){
        float wave=Basis_Sin(r[i]*time);
        W(cg,0x649d0+4*i,(wave*a[i])*weight+R<float>(cg,0x649d0+4*i));
    }
}
}
#endif
