// Store camera support bodies: shake attenuation, view origin offset,
// and remote-eyes tag resolution.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_camera_detail {
using Byte=unsigned char;using U32=std::uint32_t;
extern "C" {
bool Detail_SuppressShake(); // 1405E07E0
__m128 Detail_SinCos(float,float); // 140890DA3
extern Byte *Detail_State; // 140E53A20
extern Byte *Detail_Entities[]; // 140E4F8E0
extern const std::int16_t *Detail_ModelMap; // 143FCD390
extern Byte *Detail_Models; // 143F85390
extern unsigned short Detail_PrimaryTag,Detail_FallbackTag; // 141EF6B18,141EF6B12
extern const char *Detail_ErrorText; // 1408E2000
bool Detail_ErrorCheck(); // 1400A9960
void Detail_Error(int,const char*); // 14039AEB0
Byte *Detail_Tag(Byte*,Byte*,unsigned); // 1400E6370
void Detail_TagAxis(Byte*,float*); // 1400DE970
void Detail_RemoteSpecial(Byte*,Byte*); // 1400C33F0
bool Detail_RemoteFallback(int); // 1400C3650
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 v){float f;std::memcpy(&f,&v,4);return f;}
static float Abs(float f){U32 v;std::memcpy(&v,&f,4);return F(v&0x7fffffffu);}
static float Neg(float f){U32 v;std::memcpy(&v,&f,4);return F(v^0x80000000u);}
static int Diff(int a,int b){U32 v=U32(a)-U32(b);int i;std::memcpy(&i,&v,4);return i;}
static float Sqrt(float f){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(f)));}
static float Mul(float a,float b){__asm__("mulss %1,%0":"+x"(a):"x"(b));return a;}
// 1400C72B0..1400C73CF. Temporal and spatial attenuation.
// Negative spatial weight uses division, not multiplication or a clamp.
int UpdateShake(Byte*cg,Byte*slot){
    if(Detail_SuppressShake())return 0;
    const int delta=Diff(R<int>(cg,0x4808c),R<int>(slot,0));
    if(delta<0)return 0;
    const float elapsed=float(delta),duration=R<float>(slot,8);
    if(elapsed>=duration)return 0;
    float spatial=1.0f;
    const float radius=R<float>(slot,0xc);
    if(radius>0.0f){
        const float x=R<float>(cg,0x4d924)-R<float>(slot,0x10),
                    y=R<float>(cg,0x4d928)-R<float>(slot,0x14),
                    z=R<float>(cg,0x4d92c)-R<float>(slot,0x18);
        spatial=1.0f-Sqrt((y*y+x*x)+z*z)*(1.0f/radius);
    }
    const float temporal=(1.0f-elapsed/duration)*R<float>(slot,4);
    if(!(temporal>0.0f))return 0;
    W(slot,0x20,temporal);
    W(slot,0x1c,spatial>=0.0f?spatial*temporal:spatial/temporal);
    return 1;
}
// 140472FE0..14047309E. Offset is nonlinear and the angular input
// is passed to the original packed trigonometric routine.
void OffsetOrigin(float *origin,float yaw,float amount){
    if(!(amount<0.0f||amount>0.0f))return;
    const float scale=(2.0f-Abs(amount))*amount;
    alignas(16) float t[4];
    _mm_store_ps(t,Detail_SinCos(yaw*F(0x3c8efa35),scale*F(0x3e8efa35)));
    const float distance=scale*20.0f;
    origin[1]=Mul(Neg(Mul(t[1],t[3])),distance)+origin[1];
    origin[0]=Mul(Mul(t[0],t[3]),distance)+origin[0];
    origin[2]=Mul(Neg(t[2]),distance)+origin[2];
}
// 1400C3950..1400C3ADD. Linked remote-eye model tags and fallback.
bool CalcRemoteEyesView(int client){
    Byte *cg=Detail_State;
    if(!(R<Byte>(cg,0x4852c)&4))
        return (R<U32>(cg,0x480c0)&0x2000)&&Detail_RemoteFallback(client);
    const int n=R<std::int16_t>(cg,0x4851c);
    if(Detail_ErrorCheck())Detail_Error(1,Detail_ErrorText);
    Byte *entity=Detail_Entities[client]+n*0x3d8;
    if(!(R<U32>(entity,0x3cc)&2))return false;
    const int i=Detail_ModelMap[client*0x701+R<int>(entity,0x1f4)];
    if(!i)return false;
    Byte *model=Detail_Models+i*0x90;
    Byte *tag=Detail_Tag(entity,model,Detail_PrimaryTag);
    if(!tag){
        tag=Detail_Tag(entity,model,Detail_FallbackTag);
        if(tag){float axis[9];Detail_TagAxis(tag,axis);}
    }
    if(tag)for(unsigned j=0;j<3;++j)W(cg,0x4d924+4*j,R<U32>(tag,0x10+4*j));
    if(R<unsigned short>(entity,0x2cc)==12)Detail_RemoteSpecial(cg,entity);
    return tag!=nullptr;
}
}
#endif
