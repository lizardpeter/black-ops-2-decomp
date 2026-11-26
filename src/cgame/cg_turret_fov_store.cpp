// Store MP CG_GetTurretViewFov, 1400C1A00..1400C1B8B.
// The incoming float is unused in this LTCG revision: baseline is 55 degrees.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstring>
#include <cstdint>
namespace store_turret_fov {
using Byte=unsigned char;
using U32=std::uint32_t;
extern "C" {
extern const Byte *Turret_State; // 140E53A20
extern const Byte *Turret_WeaponDefs[256]; // 14265D3B0
extern const Byte *Turret_AttachmentSentinel; // 14265DBB0
U32 Turret_SelectWeapon(int); // 14018DA10
float Turret_TransitionA(U32); // 14065B060, original preserves live XMM1..4
float Turret_TransitionB(U32); // 14065B120
}
template<class T>static T Read(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
float GetTurretViewFov(int client,const Byte *ps,float) {
    float fov=55.0f;
    const U32 weapon=Turret_SelectWeapon(client);
    // The second selection is independent and controls the weapon property.
    const U32 check=Turret_SelectWeapon(client);
    const Byte *checkDef=Turret_WeaponDefs[check&255u];
    if(!Read<Byte>(Read<const Byte*>(checkDef,0x10),0x808))return fov;
    const Byte *def=Turret_WeaponDefs[weapon&255u];
    const Byte *selected[3]{};
    for(unsigned i=0;i<3;++i) {
        const unsigned index=(weapon>>(8+6*i))&63u;
        if(index)selected[i]=Read<const Byte *const *>(def,0x30)[index];
    }
    float zoom=Read<float>(def,0x270);
    for(const Byte *a:selected) {
        if(!a)break;
        const float next=Read<float>(a,0x40);
        const float sentinel=Read<float>(Turret_AttachmentSentinel,0x40);
        if(next<sentinel || next>sentinel)zoom=next;
    }
    if(zoom>=fov)zoom=fov;
    const float fraction=Read<float>(ps,0x1e0);
    // UCOMISS/JE also branches on unordered input.
    if(!(fraction<1.0f || fraction>1.0f))return zoom;
    if(!(fraction<0.0f || fraction>0.0f))return fov;
    const float duration=Read<int>(Turret_State,0x4ae94)?Turret_TransitionA(weapon):Turret_TransitionB(weapon);
    float t=fraction-(1.0f-duration);
    if(!(t>0.0f))return fov;
    t/=duration;
    if(!(t>0.0f))return fov;
    fov-=(fov-zoom)*t;
    return fov;
}
}
#endif
