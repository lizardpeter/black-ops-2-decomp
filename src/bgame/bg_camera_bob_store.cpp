// Store MP full vertical/horizontal view bob bodies at 1406602A0 and
// 140660470. Both use raw vec2 dvar current lanes, except the max-amplitude
// dvar whose original getter performs type conversion.
#define T6_SOURCE "bgame/bg_weapons.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_camera_bob {
using Byte=unsigned char;
struct Dvar;
extern "C" {
extern const Dvar *Bob_HighState; // 14265CCE8
extern const Dvar *Bob_Flag21; // 14265CB48
extern const Dvar *Bob_State11; // 14265CAF0
extern const Dvar *Bob_State40; // 14265CB08
extern const Dvar *Bob_State40Ads; // 14265CC40
extern const Dvar *Bob_Flag15; // 14265A8A0
extern const Dvar *Bob_Default; // 14265CCE0
extern const Dvar *Bob_DefaultAds; // 14265C9F0
extern const Dvar *Bob_Max; // 14265CB30
double Bob_Atof(const char*); // IAT 1408A7DD8
float Bob_Sin(float); // 140870BC8
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
static float Lane(const Dvar*d,unsigned lane){return d?R<float>(d,0x20+lane*4):0.0f;}
static float Float(const Dvar*d){
    if(!d)return 0.0f;
    const int type=R<int>(d,0x18);
    if(type==2)return R<float>(d,0x20);
    if(type==6)return float(R<int>(d,0x20));
    return float(Bob_Atof(R<const char*>(d,0x20)));
}
static float Amplitude(const Byte*ps,float speed,unsigned lane){
    if(R<int>(ps,0x5b8)>=4)return Lane(Bob_HighState,lane);
    const unsigned flags=R<unsigned>(ps,0xc);
    if(flags&0x200000)return Lane(Bob_Flag21,lane)*speed;
    const int state=R<int>(ps,0x204);
    if(state==11)return Lane(Bob_State11,lane)*speed;
    if(state!=40 && (flags&0x8000))return Lane(Bob_Flag15,lane)*speed;
    const float hip=Lane(state==40?Bob_State40:Bob_Default,lane);
    const float ads=Lane(state==40?Bob_State40Ads:Bob_DefaultAds,lane);
    const float fraction=R<float>(ps,0x1e0);
    return ((1.0f-fraction)*hip+ads*fraction)*speed;
}
static float MinOriginal(float a,float b){return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float VerticalBob(const Byte*ps,float phase,float speed){
    const float amplitude=Amplitude(ps,speed,1);
    const float limit=Float(Bob_Max);
    const float first=Bob_Sin(phase+phase)*0.75f;
    const float second=Bob_Sin(phase*4.0f+1.57079637050628662109375f)*0.15f;
    return (first+second)*MinOriginal(limit,amplitude);
}
float HorizontalBob(const Byte*ps,float phase,float speed){
    const float amplitude=Amplitude(ps,speed,0);
    const float limit=Float(Bob_Max);
    return Bob_Sin(phase)*MinOriginal(limit,amplitude);
}
}
#endif
