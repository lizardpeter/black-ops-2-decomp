// Store MP CG_CalcFov at 1400C2060..1400C23B2.
// Store LTCG inlines both CG_CalcFovInternal passes and camera tween FOV.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_camera_projection {
using Byte=unsigned char;
using U32=std::uint32_t;
struct Dvar;
union Value {U32 words[4];const char *string;};
extern "C" {
extern Byte *Projection_State; // 140E53A20
extern const Byte *Projection_Static; // 140E53B70 (viewAspect +0x10)
extern const Dvar *Projection_DualEnable; // 1427E6E50
extern const Dvar *Projection_DualActive; // 1427E6D70
extern const Dvar *Projection_Fov; // 140E4F8E8
extern int Projection_ForceFullscreen; // 142CC0A2C
extern const char *Projection_Zero; // original literal 14093ED9C
float Projection_GetViewFov(int,Byte); // 1400C1B90
float Projection_Tan(float); // 140892D02
int Projection_Atoi(const char *); // IAT 1408A7DC0
double Projection_Atof(const char *); // IAT 1408A7DD8
void Projection_SetVariant(const Dvar *,const Value *,int); // 14046B300
}
template<class T> static T Read(const void *p,unsigned o) {
    T v;std::memcpy(&v,static_cast<const Byte *>(p)+o,sizeof(v));return v;
}
template<class T> static void Write(void *p,unsigned o,T v) {
    std::memcpy(static_cast<Byte *>(p)+o,&v,sizeof(v));
}
static float F(U32 bits){float v;std::memcpy(&v,&bits,4);return v;}
static float GetFloat(const Dvar *d) {
    if(!d)return 0.0f;
    const int t=Read<int>(d,0x18);
    if(t==2)return Read<float>(d,0x20);
    if(t==6)return float(Read<int>(d,0x20));
    return float(Projection_Atof(Read<const char *>(d,0x20)));
}
static float Tween(Byte *cg,float target) {
    int start=Read<int>(cg,0x64a28);
    const int now=Read<int>(cg,0x4808c);
    if(start>now){start=0;Write(cg,0x64a28,start);}
    // SUB ECX,EDX wraps at 32 bits before conversion to float.
    const U32 delta=U32(now)-U32(start);
    int signedDelta;std::memcpy(&signedDelta,&delta,4);
    const float elapsed=float(signedDelta)*F(0x3a83126f);
    const float duration=Read<float>(cg,0x64a2c);
    if(!(elapsed>=duration)) {
        const float initial=Read<float>(cg,0x64a24);
        const float fraction=elapsed/duration;
        target=initial+fraction*(target-initial);
    }
    Write(cg,0x649f4,target);
    return target;
}
struct Slopes{float horizontal,vertical,reference;};
static Slopes Project(float fov,const Byte *statics) {
    const float t=Projection_Tan(fov*F(0x3c0efa35));
    const float aspect=Read<float>(statics,0x10);
    if(aspect<1.0f && aspect>0.0f)return {t,t/aspect,t};
    const float vertical=t*0.75f;
    return {aspect*vertical,vertical,t};
}
static void ObserveDualEnable() {
    // Store's single-client path discards the integer conversion result.
    if(const Dvar *d=Projection_DualEnable;d && Read<int>(d,0x18)!=1)
        Projection_Atoi(Read<const char *>(d,0x20));
}
static void ClearDualActive() {
    const Dvar *d=Projection_DualActive;
    if(!d || !Read<const char *>(d,0))return;
    alignas(16) Value value{};
    if(Read<int>(d,0x18)!=1)value.string=Projection_Zero;
    Projection_SetVariant(d,&value,0);
}
void CalcFov(int client,float requested) {
    const Byte *statics=Projection_Static;
    Byte *const output=Projection_State;
    float weapon=requested;
    if(!(requested>0.0f))weapon=Projection_GetViewFov(client,1);
    weapon=Tween(Projection_State,weapon);
    const Slopes weaponSlopes=Project(weapon,statics);
    ObserveDualEnable();
    ClearDualActive();
    Write(output,0x64950,weaponSlopes.horizontal);
    Write(output,0x64954,weaponSlopes.vertical);
    statics=Projection_Static;
    float world=requested;
    if(!(requested>0.0f))world=Projection_GetViewFov(client,0);
    world=Tween(Projection_State,world);
    const Slopes worldSlopes=Project(world,statics);
    ObserveDualEnable();
    if(Projection_ForceFullscreen)ClearDualActive();
    Write(output,0x4d910,worldSlopes.horizontal);
    Write(output,0x4d914,worldSlopes.vertical);
    Write(output,0x4d918,worldSlopes.vertical);
    Write(output,0x4d920,world);
    const float reference=Projection_Tan(GetFloat(Projection_Fov)*F(0x3c0efa35));
    Write(output,0x69fa8,worldSlopes.reference/reference);
}
}
#endif
