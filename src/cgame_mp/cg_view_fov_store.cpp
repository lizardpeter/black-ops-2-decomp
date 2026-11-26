// Store MP camera FOV calculation, complete body 1400C1B90..1400C2057.
// Descriptive names for still-unidentified fields and callees are deliberately
// avoided. Preserve the Store offsets and exact early-return/scaling rules.
#define T6_SOURCE "cgame_mp/cg_view_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_camera {
using U32=std::uint32_t;
using Byte=unsigned char;
struct Dvar;
extern "C" {
extern Byte *Camera_State;                 // 140E53A20
extern const Dvar *Camera_Fov;             // 140E4F8E8
extern const Dvar *Camera_OverrideFov;     // 140E5A670
extern const Dvar *Camera_DefaultFov;      // 142161688
extern const Dvar *Camera_MinFov;          // 140D78648
extern const Dvar *Camera_Scale;           // 140D78658
extern const Dvar *Camera_EntityOverride;  // 14265CAF8
extern const Byte *Camera_WeaponDefs[256]; // 14265D3B0
extern const Byte *Camera_AttachmentSentinel; // 14265DBB0
extern const Byte *Camera_EntityDefs[256]; // 14265D0B0
extern Byte *Camera_EntityBase[];          // 140E4F8E0, indexed by local client
extern int Camera_Mode;                    // 1425A3308
extern Byte Camera_ModeFlags;              // 1425A330C
extern Byte Camera_AuxEnabled;             // 142653908
extern const Byte *Camera_AuxState;        // 142653900
extern const char *Camera_ErrorText;       // points to original 1408E2000
int Camera_HasWeaponState(const Byte *);   // 1406468B0
U32 Camera_SelectWeapon(const Byte *,int);// 14018DA90
U32 Camera_SelectOtherWeapon(int);         // 14018DA10
U32 Camera_DefaultWeapon(const Byte *);    // 1406448B0
int Camera_OverrideActive(int);            // 14018E600
// At both original call sites RCX has no intentional argument setup.
// The first ABI lane is provisional and the extracted callee must establish
// whether it is live; replay observes the four prepared lanes only.
float Camera_WeaponFov(void *,const Byte *,U32,float,Byte); // 1400C12F0
float Camera_AdjustFov(int,const Byte *,float); // 1400C1A00
bool Camera_ErrorCheck();                  // 1400A9960, no prepared args
void Camera_Error(int,const char *);       // 14039AEB0, observed call boundary
double Camera_Atof(const char *);           // IAT 1408A7DD8
int Camera_Atoi(const char *);              // IAT 1408A7DC0
}
template<class T> static T Read(const void *p,unsigned off) {
    T value;std::memcpy(&value,static_cast<const Byte *>(p)+off,sizeof(value));return value;
}
static float GetFloat(const Dvar *d) {
    if(!d)return 0.0f;
    const int type=Read<int>(d,0x18);
    if(type==2)return Read<float>(d,0x20);
    if(type==6)return float(Read<int>(d,0x20));
    return float(Camera_Atof(Read<const char *>(d,0x20)));
}
static bool GetBool(const Dvar *d) {
    if(!d)return false;
    if(Read<int>(d,0x18)==1)return Read<Byte>(d,0x20)!=0;
    return Camera_Atoi(Read<const char *>(d,0x20))!=0;
}
static bool HasIndex(U32 weapon) {return (weapon&255u)!=0;}
float CalculateFov(int client,Byte alternate) {
    Byte *const cg=Camera_State;
    const Byte *const ps=cg+0x480a8;
    U32 weapon=0;
    if(Camera_HasWeaponState(ps))weapon=Camera_SelectWeapon(ps,client);
    else if(Read<U32>(cg,0x481a4)&0x300)weapon=Camera_SelectOtherWeapon(client);
    if(!HasIndex(weapon))weapon=Camera_DefaultWeapon(ps);
    float fov=GetFloat(Camera_Fov);
    if(Camera_OverrideActive(client))return GetFloat(Camera_OverrideFov);
    if((Read<Byte>(cg,0x4852c)&4) && HasIndex(weapon) &&
       !(Read<U32>(cg,0x481a8)&0x10000000)) {
        const Byte *def=Camera_WeaponDefs[weapon&255u];
        const Byte *const *attachments=Read<const Byte *const *>(def,0x30);
        const Byte *selected[3]{};
        for(unsigned i=0;i<3;++i) {
            const unsigned index=(weapon>>(8+6*i))&63u;
            if(index)selected[i]=attachments[index];
        }
        float zoom=Read<float>(def,0x270);
        for(const Byte *attachment:selected) {
            if(!attachment)break;
            const float next=Read<float>(attachment,0x40);
            const float sentinel=Read<float>(Camera_AttachmentSentinel,0x40);
            // UCOMISS / JE treats unordered comparisons as equal.
            if(next<sentinel || next>sentinel)zoom=next;
        }
        if(Read<float>(cg,0x48288)>0.0f && zoom>0.0f)return zoom;
        return 45.0f;
    }
    // The original reloads the global after the weapon/override calls.
    const Byte *view=Camera_State;
    const int mode=Camera_Mode;
    if(view && Read<const Byte *>(view,0x30)) {
        const int idx=Read<int>(view,0);
        if(Read<int>(view,0x6b9b4+0x870*idx)==9 ||
            (mode==2 && (Camera_ModeFlags&4))) {
            if(Read<int>(cg,0x480a0)==1 &&
                (Read<U32>(Read<const Byte *>(cg,0x30),0x10c)&0x4000))
                return GetFloat(Camera_DefaultFov);
        } else if(mode!=2) {
            // Original skips the auxiliary-mode checks in this case.
        }
    }
    if(mode==2 && Camera_AuxEnabled) {
        const int state=Read<int>(Camera_AuxState,0xd4c00);
        if(state==1 || state==2)return fov;
    }
    if(!(Read<U32>(view,0x481a4)&0x4300) &&
        (Read<Byte>(view,0x480c0)&2) && Read<int>(view,0x480a0)==1)
        return fov;
    fov=Camera_WeaponFov(nullptr,ps,weapon,fov,alternate);
    if((Read<U32>(cg,0x481a4)&0x300) && !(Read<U32>(cg,0x481a8)&0x10000000))
        fov=Camera_AdjustFov(client,ps,fov);
    const float minimum=GetFloat(Camera_MinFov);
    if((Read<U32>(cg,0x481a4)&0x4000) && GetBool(Camera_EntityOverride)) {
        const int index=Read<std::int16_t>(cg,0x4851c);
        if(Camera_ErrorCheck())Camera_Error(1,Camera_ErrorText);
        const auto address=reinterpret_cast<std::uintptr_t>(Camera_EntityBase[client])+
            std::intptr_t(index)*0x3d8;
        const Byte *entity=reinterpret_cast<const Byte *>(address);
        if(entity && (Read<Byte>(entity,0x3cc)&2)) {
            const Byte *def=Camera_EntityDefs[Read<Byte>(entity,0x286)];
            const float zoom=Read<float>(def,0x74);
            if(zoom>minimum)fov=zoom;
            if(Camera_HasWeaponState(ps) && HasIndex(weapon))
                fov=Camera_WeaponFov(nullptr,ps,weapon,fov,alternate);
        }
    }
    fov*=GetFloat(Camera_Scale);
    // Original SUBSS then COMISS/JB: unordered keeps the product.
    if(minimum-fov>=0.0f)fov=minimum;
    return fov;
}
}
#endif
