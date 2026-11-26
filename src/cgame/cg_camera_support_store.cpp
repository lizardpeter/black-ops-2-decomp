// Store MP camera support: complete original extra mode, spike view and
// two attachment-aware ADS transition accessors.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_camera_support {
using Byte=unsigned char;
using U32=std::uint32_t;
extern "C" {
extern Byte *Support_State; // 140E53A20
extern int Support_DemoMode; // 1425A3308
extern Byte Support_PlaybackInited; // 142653908
extern const Byte *Support_Playback; // 142653900
extern const Byte *Support_WeaponDefs[256]; // 14265D3B0
extern const Byte *Support_Sentinel; // 14265DBB0
int Support_MissileActive(int); // 14018E600
}
template<class T>static T Read(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void Write(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
// 1400C1240..1400C12EF, CG_GetExtraCamMode.
int GetExtraCamMode(int client) {
    if(Support_MissileActive(client))return 6;
    const Byte *cg=Support_State;
    if(Read<U32>(cg,0x481a8)&0x40000)return 6;
    if(Support_DemoMode==2 && Support_PlaybackInited &&
       Read<int>(Support_Playback,0xd4c00)==2 &&
       Read<int>(Support_Playback,0xd4c04)==1 &&
       !Read<Byte>(Support_Playback,0x63579) &&
       Read<int>(Support_Playback,0x1017a4)!=4)return 7;
    if(Read<int>(cg,0x848f4) && (Read<U32>(cg,0x480b8)&0x800000))return 7;
    return Read<Byte>(cg,0x64b00)?8:0;
}
// 1400C3390..1400C33E8, saved extra-camera origin and angles.
void CopySpikeView() {
    Byte *cg=Support_State;
    for(unsigned i=0;i<3;++i)Write(cg,0x4d924+i*4,Read<U32>(cg,0x848fc+i*4));
    for(unsigned i=0;i<3;++i)Write(cg,0x649d0+i*4,Read<U32>(cg,0x84908+i*4));
}
static float Transition(U32 weapon,unsigned attachmentOffset,unsigned fallbackOffset) {
    const Byte *def=Support_WeaponDefs[weapon&255u];
    for(unsigned i=0;i<3;++i){
        unsigned index=(weapon>>(8+6*i))&63u;
        const Byte *a=index?Read<const Byte*const*>(def,0x30)[index]:nullptr;
        if(!a)break;
        float value=Read<float>(a,attachmentOffset),sentinel=Read<float>(Support_Sentinel,attachmentOffset);
        // UCOMISS/JNE: NaN compares as equal here.
        if(value<sentinel || value>sentinel)return value;
    }
    return Read<float>(def,fallbackOffset);
}
// 14065B060..14065B11D and 14065B120..14065B1DD.
float AdsTransitionA(U32 weapon){return Transition(weapon,0x4c,0x27c);}
float AdsTransitionB(U32 weapon){return Transition(weapon,0x50,0x280);}
}
#endif
