// Store MP CG_GetWeaponViewFov, 1400C12F0..1400C19F0 (460 instructions).
// The first ABI lane (local client) is not read by this Store LTCG body.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_weapon_fov {
using Byte=unsigned char;
using U32=std::uint32_t;
struct Dvar;
extern "C" {
extern Byte *WeaponFov_State; // 140E53A20
extern const Byte *WeaponFov_Defs[256]; // 14265D3B0
extern const Byte *WeaponFov_Sentinel; // 14265DBB0
extern const Dvar *WeaponFov_VariableZoom; // 140D78650, cg_useWeaponBasedVariableZoom
int WeaponFov_Atoi(const char *); // IAT 1408A7DC0
bool WeaponFov_Sub_14065B8C0(U32);
const void *WeaponFov_Sub_14065C600(U32);
int WeaponFov_Sub_140646920(const Byte *);
float WeaponFov_TransitionA(U32); // 14065B060
float WeaponFov_TransitionB(U32); // 14065B120
}
template<class T>static T Read(const void*p,unsigned off){
    T v;std::memcpy(&v,static_cast<const Byte*>(p)+off,sizeof(v));return v;
}
template<class T>static void Write(void*p,unsigned off,T v){
    std::memcpy(static_cast<Byte*>(p)+off,&v,sizeof(v));
}
static float F(U32 bits){float v;std::memcpy(&v,&bits,4);return v;}
static bool Different(float a,float b){return a<b || a>b;}
static bool HasIndex(U32 weapon){return (weapon&255u)!=0;}
static int Signed(U32 v){int i;std::memcpy(&i,&v,4);return i;}
static int Difference(int a,int b){return Signed(U32(a)-U32(b));}
static float Clamp01(float x){if(x>=1.0f)return 1.0f;return x>0.0f?x:0.0f;}
static bool VariableZoom() {
    const Dvar *d=WeaponFov_VariableZoom;
    if(!d)return false;
    if(Read<int>(d,0x18)==1)return Read<Byte>(d,0x20)!=0;
    return WeaponFov_Atoi(Read<const char*>(d,0x20))!=0;
}
struct AttachmentList {const Byte *ptr[3]{};};
static AttachmentList Attachments(const Byte*def,U32 weapon){
    AttachmentList out;
    for(unsigned i=0;i<3;++i){
        const unsigned n=(weapon>>(8+6*i))&63u;
        if(n)out.ptr[i]=Read<const Byte*const*>(def,0x30)[n];
    }
    return out;
}
static float Duration(U32 weapon,bool entering,const Byte*sentinel){
    const Byte *def=WeaponFov_Defs[weapon&255u];
    const Byte *props=Read<const Byte*>(def,0x10);
    const unsigned off=entering?0x2c:0x30;
    for(const Byte *a:Attachments(def,weapon).ptr){
        if(!a)break;
        const float v=Read<float>(a,off);
        if(Different(v,Read<float>(sentinel,off)))return v;
    }
    return Read<float>(props,entering?0xbf0:0xbf4);
}
struct Zooms{float first,second,third;};
static Zooms Zoom(U32 weapon,const Byte*sentinel){
    const Byte *def=WeaponFov_Defs[weapon&255u];
    Zooms z{Read<float>(def,0x270),Read<float>(def,0x274),Read<float>(def,0x278)};
    for(const Byte *a:Attachments(def,weapon).ptr){
        if(!a)break;
        const float x=Read<float>(a,0x40),y=Read<float>(a,0x44),w=Read<float>(a,0x48);
        if(Different(x,Read<float>(sentinel,0x40)))z.first=x;
        if(Different(y,Read<float>(sentinel,0x44)))z.second=y;
        if(Different(w,Read<float>(sentinel,0x48)))z.third=w;
    }
    return z;
}
static int WeaponZoomMode(const Byte*ps,U32 weapon){
    if(!HasIndex(weapon))return 0;
    for(unsigned i=0;i<15;++i)
        if(Read<U32>(ps,0x248+i*0x1c)==weapon)
            return Read<int>(ps,0x258+i*0x1c);
    return 0;
}
static float SelectZoom(Zooms z,int mode,float fov){
    const float selected=mode==1?z.second:mode==2?z.first:z.third;
    // The original compares the rounded difference, not selected directly.
    return selected-fov>=0.0f?fov:selected;
}
float GetWeaponViewFov(void*,const Byte*ps,U32 weapon,float fov,Byte forWeapon){
    const Byte *props=Read<const Byte*>(WeaponFov_Defs[weapon&255u],0x10);
    if(!(Read<Byte>(ps,0x18)&2) && HasIndex(weapon) && Read<int>(ps,0x1d0)==44){
        const float ratio=float(Read<int>(ps,0x4c))/float(Read<int>(props,0x6f4));
        return fov+(1.0f-ratio)*(55.0f-fov);
    }
    if(!Read<Byte>(props,0x808))return fov;
    const bool variable=VariableZoom();
    const bool entering=Read<int>(props,0x44)==3 && WeaponFov_Sub_14065B8C0(weapon);
    Byte *const cg=WeaponFov_State;
    const Byte *const sentinel=WeaponFov_Sentinel;
    const int now=Read<int>(cg,0x4808c);
    float dt=float(Difference(now,Read<int>(cg,0x88120)))*F(0x3a83126f);
    float progress=Read<float>(cg,0x8811c);
    Write(cg,0x88120,now);
    if(dt>0.1f)dt=0.1f;
    if(entering){
        if(!(progress>=1.0f)){
            progress=Clamp01(dt/Duration(weapon,true,sentinel)+progress);
            Write(cg,0x8811c,progress);
        }
    }else if(progress>0.0f){
        progress=Clamp01(progress-dt/Duration(weapon,false,sentinel));
        Write(cg,0x8811c,progress);
    }
    const Zooms z=Zoom(weapon,sentinel);
    float ads=Read<float>(ps,0x1e0);
    const U32 active=Read<U32>(ps,0x1b8);
    const int mode=variable?WeaponZoomMode(ps,active):Read<int>(ps,0x5c4)%2;
    float target=SelectZoom(z,forWeapon?0:mode,fov);
    progress=Read<float>(cg,0x8811c);
    if(progress>0.0f)target=(1.0f-progress)*target+progress*z.first;
    if(!forWeapon && WeaponFov_Defs[active&255u] && ads>=1.0f &&
       WeaponFov_Sub_14065C600(active) && WeaponFov_Sub_140646920(ps)){
        const int selected=WeaponZoomMode(ps,active);
        const int previous=Read<int>(ps,0x5d0);
        if(selected!=previous){
            const int elapsed=Difference(Difference(now,Read<int>(ps,0xbec)),Read<int>(ps,0x5d4));
            if(float(elapsed)<125.0f){
                const float old=SelectZoom(z,previous,fov);
                const float t=Clamp01(float(elapsed)*F(0x3c03126f));
                target=(1.0f-t)*old+t*target;
            }
        }
    }
    // The original reloads cgArray before the final transition choice.
    const Byte *view=WeaponFov_State;
    if(!Different(ads,1.0f))return target;
    if(!Different(ads,0.0f))return fov;
    const float transition=Read<int>(view,0x4ae94)?
        WeaponFov_TransitionA(weapon):WeaponFov_TransitionB(weapon);
    ads-=1.0f-transition;
    if(!(ads>0.0f))return fov;
    ads/=transition;
    if(!(ads>0.0f))return fov;
    return fov-(fov-target)*ads;
}
}
#endif
