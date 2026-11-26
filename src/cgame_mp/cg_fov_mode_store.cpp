// Store MP 140101410: mode-dependent grenade HUD geometry and FOV scale.
// Function identity is descriptive until original naming is corroborated.
#define T6_SOURCE "cgame_mp/cg_main_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CG_FOV_MODE_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_cg_fov {
struct Dvar;
extern "C" {
extern const Dvar *FovMode_DualPlay; // 1427E6E50, r_dualPlayEnable
extern const Dvar *FovMode_Height; // 140E4F8B8
extern const Dvar *FovMode_Width; // 140E4F8C8
extern const Dvar *FovMode_Offset; // 140E4F8A0
extern const Dvar *FovMode_PointerHeight; // 140E4F8B0
extern const Dvar *FovMode_PointerWidth; // 140E4F8A8
extern const Dvar *FovMode_PointerPivot; // 140E4F8D0
extern const Dvar *FovMode_Scale; // 140D78658, cg_fovScale
int FovMode_Atoi(const char *); // IAT 1408A7DC0
void FovMode_SetFloat(const Dvar *,float,int); // 14046CEF0
void FovMode_SetVec2(const Dvar *,float,float); // 14046D040, ABI as called here
}
template<class T> static T Read(const Dvar *d,unsigned offset) {
    T v; std::memcpy(&v,reinterpret_cast<const unsigned char *>(d)+offset,sizeof(T));return v;
}
void SetHudAndFovMode(bool alternate) {
    if(alternate) {
        // The integer result is not consumed. Keep the observed call when
        // the DualPlay dvar has a non-bool type rather than dropping it.
        if(const Dvar *d=FovMode_DualPlay;d && Read<int>(d,0x18)!=1)
            FovMode_Atoi(Read<const char *>(d,0x20));
    }
    const float iconSize=alternate?37.5f:25.0f;
    const float pointerHeight=alternate?18.0f:12.0f;
    FovMode_SetFloat(FovMode_Height,iconSize,0);
    FovMode_SetFloat(FovMode_Width,iconSize,0);
    FovMode_SetFloat(FovMode_Offset,alternate?75.0f:50.0f,0);
    FovMode_SetFloat(FovMode_PointerHeight,pointerHeight,0);
    FovMode_SetFloat(FovMode_PointerWidth,iconSize,0);
    FovMode_SetVec2(FovMode_PointerPivot,pointerHeight,alternate?40.5f:27.0f);
    FovMode_SetFloat(FovMode_Scale,alternate?0.75f:1.0f,0);
}
}
#endif
