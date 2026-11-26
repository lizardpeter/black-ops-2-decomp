// Store MP cg_main.cpp: default view FOV selection and synchronization.
#define T6_SOURCE "cgame/cg_main.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstring>
namespace store_fov_defaults {
using Byte=unsigned char;
struct Dvar;
extern "C" {
extern Byte *FovDefaults_State; // 140E53A20
extern const Dvar *FovDefaults_Current; // 140E4F8E8
extern const Dvar *FovDefaults_First; // 142161688
extern const Dvar *FovDefaults_Third; // 142161678
double FovDefaults_Atof(const char *); // IAT 1408A7DD8
void FovDefaults_SetFloat(const Dvar *,float,int); // 14046CEF0
}
template<class T> static T Read(const void *p,unsigned o) {
    T v;std::memcpy(&v,static_cast<const Byte *>(p)+o,sizeof(v));return v;
}
static float GetFloat(const Dvar *d) {
    if(!d)return 0.0f;
    const int type=Read<int>(d,0x18);
    if(type==2)return Read<float>(d,0x20);
    if(type==6)return float(Read<int>(d,0x20));
    return float(FovDefaults_Atof(Read<const char *>(d,0x20)));
}
// 1400F9280, CG_GetDefaultFovForView.
float GetDefaultFovForView() {
    const Byte *cg=FovDefaults_State;
    return GetFloat((cg[0x4809c]|cg[0x4812c])?FovDefaults_Third:FovDefaults_First);
}
static void Sync(float target) {
    const float current=GetFloat(FovDefaults_Current);
    // Original UCOMISS/JE: unordered is also treated as equal.
    if(current<target || current>target)FovDefaults_SetFloat(FovDefaults_Current,target,0);
}
// 1400F92F0: clears one third-person flag, selects and synchronizes.
void ClearViewModeAndSync() {
    Byte *cg=FovDefaults_State;
    if(cg[0x4809c])cg[0x4809c]=0;
    Sync(GetDefaultFovForView());
}
// 1400F93E0: RCX not consumed (local client); DL is the exact byte.
void SetViewModeAndSync(int,Byte mode) {
    Byte *cg=FovDefaults_State;
    if(cg[0x4809c]!=mode)cg[0x4809c]=mode;
    Sync(GetDefaultFovForView());
}
// 1400F9470: synchronization without changing either view-mode flag.
void SyncViewMode() {Sync(GetDefaultFovForView());}
}
#endif
