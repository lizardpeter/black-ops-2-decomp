// Store MP water surface query at 14038BAD0..14038BCDB.
// The two underlying engine geometry queries remain explicit boundaries.
#define T6_SOURCE "qcommon/cm_trace.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_camera_water {
using Byte=unsigned char;using U32=std::uint32_t;
struct Dvar;
struct alignas(16) Ray {Byte bytes[64];};
struct alignas(16) GeometryResult {Byte bytes[64];};
extern "C" {
extern const Dvar *Water_Count; // 14265CE78
extern const Byte *Water_GeometryTable; // address 14210CCD0, stride 32
int Water_Atoi(const char*); // IAT 1408A7DC0
void Water_GeometryTrace(const Ray*,const Byte*,GeometryResult*); // 1403E2950
int Water_BrushTrace(const float*,const float*,int,void*,float*,void*,void*); // 14038B680
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static int Count(const Dvar*d){
    if(!d)return 0;
    const int t=R<int>(d,0x18);
    if(t==6||t==7)return R<int>(d,0x20);
    if(t==1)return R<Byte>(d,0x20)!=0;
    if(t==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Water_Atoi(R<const char*>(d,0x20));
}
float WaterHeight(const float*origin,float above,float below){
    float top[3]={origin[0],origin[1],origin[2]+above};
    float bottom[3]={origin[0],origin[1],origin[2]+below};
    float height=-131072.0f;
    const int count=Count(Water_Count);
    if(count>0){
        Ray ray{};
        GeometryResult result{};
        for(unsigned i=0;i<3;++i){W(&ray,4*i,top[i]);W(&ray,16+4*i,bottom[i]);}
        const float dz=top[2]-bottom[2];
        W(&ray,40,(dz<0||dz>0)?1.0f/dz:0.0f);
        W(&ray,56,-1);W(&result,16,1.0f);
        Water_GeometryTrace(&ray,Water_GeometryTable+std::uintptr_t(count)*32,&result);
        const float fraction=R<float>(&result,16);
        if(!(fraction>=1.0f)){
            const float surface=(bottom[2]-top[2])*fraction+top[2];
            height=_mm_cvtss_f32(_mm_max_ss(_mm_set_ss(surface),_mm_set_ss(height)));
        }
    }
    float hit[4];
    if(Water_BrushTrace(top,bottom,32,nullptr,hit,nullptr,nullptr)){
        if(hit[2]-height>=0.0f)return hit[2];
    }
    return height;
}
}
#endif
