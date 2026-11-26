// Standalone differential probe; include candidate to share the provisional ABI types.
#define T6_STORE
#define T6_STORE_CG_PROJECT_REPLAY
#include "../src/cgame_mp/cg_register_dvars_store.cpp"
#include <array>
#include <cstdlib>
#include <cstddef>
#include <initializer_list>
using U = std::uint64_t;
using P = const dvar_t *;
#define MS __attribute__((ms_abi))
extern "C" {
const dvar_t *store_cgDvar_g_loadScripts = nullptr;
const dvar_t *store_cgDvar_cg_drawGun = nullptr;
const dvar_t *store_cgDvar_cg_cursorHints = nullptr;
const dvar_t *store_cgDvar_cg_retrieveHintTime = nullptr;
const dvar_t *store_cgDvar_cg_retrieveHintTimeStuck = nullptr;
const dvar_t *store_cgDvar_cg_weaponHintsCoD1Style = nullptr;
const dvar_t *store_cgDvar_cg_fov = nullptr;
const dvar_t *store_cgDvar_cg_fovScale = nullptr;
const dvar_t *store_cgDvar_cg_fovMin = nullptr;
const dvar_t *store_cgDvar_cg_fovExtraCam = nullptr;
const dvar_t *store_cgDvar_cg_useWeaponBasedVariableZoom = nullptr;
const dvar_t *store_cgDvar_cg_viewVehicleInfluenceGunner = nullptr;
const dvar_t *store_cgDvar_cg_viewVehicleInfluenceGunnerFiring = nullptr;
const dvar_t *store_cgDvar_cg_draw2D = nullptr;
const dvar_t *store_cgDvar_cg_disableplayernames = nullptr;
const dvar_t *store_cgDvar_cg_drawHealth = nullptr;
const dvar_t *store_cgDvar_cg_drawBreathHint = nullptr;
const dvar_t *store_cgDvar_cg_drawBigFPS = nullptr;
const dvar_t *store_cgDvar_cg_drawlagometer = nullptr;
const dvar_t *store_cgDvar_cg_drawFPS = nullptr;
const dvar_t *store_cgDvar_cg_drawCrosshair = nullptr;
const dvar_t *store_cgDvar_cg_drawCrosshairNames = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadeIconMaxRangeFrag = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadeIconMaxRangeFlash = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadeIconOffset = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadeIconHeight = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadeIconWidth = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadePointerHeight = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadePointerWidth = nullptr;
const dvar_t *store_cgDvar_cg_hudGrenadePointerPivot = nullptr;
const dvar_t *store_cgDvar_lagometer_pos = nullptr;
const dvar_t *store_cgDvar_cg_centertime = nullptr;
const dvar_t *store_cgDvar_cg_crosshairAlpha = nullptr;
const dvar_t *store_cgDvar_cg_crosshairAlphaMin = nullptr;
const dvar_t *store_cgDvar_cg_crosshairDynamic = nullptr;
const dvar_t *store_cgDvar_cg_crosshairEnemyColor = nullptr;
const dvar_t *store_cgDvar_cg_debugposition = nullptr;
const dvar_t *store_cgDvar_cg_debugevents = nullptr;
const dvar_t *store_cgDvar_cg_airburstLaseTime = nullptr;
const dvar_t *store_cgDvar_cg_airburstStrafeRestriction = nullptr;
const dvar_t *store_cgDvar_cg_airburstCameraRestriction = nullptr;
const dvar_t *store_cgDvar_cg_airburstLaseFlashTime = nullptr;
const dvar_t *store_cgDvar_cg_thirdPersonRange = nullptr;
const dvar_t *store_cgDvar_cg_thirdPersonAngle = nullptr;
const dvar_t *store_cgDvar_cg_thirdPerson = nullptr;
const dvar_t *store_cgDvar_cg_thirdPersonMode = nullptr;
const dvar_t *store_cgDvar_cg_fakefireWizbyChance = nullptr;
const dvar_t *store_cgDvar_cg_useWeaponSwitchReloadCancel = nullptr;
const dvar_t *store_cgDvar_cg_enableHelicopterNoCullLodOut = nullptr;
const dvar_t *store_cgDvar_cl_paused = nullptr;
const dvar_t *store_cgDvar_cg_drawpaused = nullptr;
const dvar_t *store_cgDvar_g_synchronousClients = nullptr;
const dvar_t *store_cgDvar_cg_dumpAnims = nullptr;
const dvar_t *store_cgDvar_cg_subtitleWidthStandard = nullptr;
const dvar_t *store_cgDvar_cg_subtitleWidthWidescreen = nullptr;
const dvar_t *store_cgDvar_cg_constantSizeHeadIcons = nullptr;
const dvar_t *store_cgDvar_cg_headIconMinScreenRadius = nullptr;
const dvar_t *store_cgDvar_cg_overheadNamesMaxDist = nullptr;
const dvar_t *store_cgDvar_cg_overheadNamesNearDist = nullptr;
const dvar_t *store_cgDvar_cg_overheadNamesFarDist = nullptr;
const dvar_t *store_cgDvar_cg_overheadNamesFarScale = nullptr;
const dvar_t *store_cgDvar_cg_overheadNamesSize = nullptr;
const dvar_t *store_cgDvar_cg_overheadIconSize = nullptr;
const dvar_t *store_cgDvar_cg_overheadRankSize = nullptr;
const dvar_t *store_cgDvar_cg_overheadNamesGlow = nullptr;
const dvar_t *store_cgDvar_cg_drawFriendlyNames = nullptr;
const dvar_t *store_cgDvar_cg_enemyNameFadeIn = nullptr;
const dvar_t *store_cgDvar_cg_friendlyNameFadeIn = nullptr;
const dvar_t *store_cgDvar_cg_enemyNameFadeOut = nullptr;
const dvar_t *store_cgDvar_cg_friendlyNameFadeOut = nullptr;
const dvar_t *store_cgDvar_cg_playerHighlightTargetSize = nullptr;
const dvar_t *store_cgDvar_cg_maxPlayerHighlightTargetSize = nullptr;
const dvar_t *store_cgDvar_cg_adsZScaleMax = nullptr;
const dvar_t *store_cgDvar_ui_email_address = nullptr;
const dvar_t *store_cgDvar_cg_viewZSmoothingMin = nullptr;
const dvar_t *store_cgDvar_cg_viewZSmoothingMax = nullptr;
const dvar_t *store_cgDvar_cg_viewZSmoothingTime = nullptr;
const dvar_t *store_cgDvar_overrideNVGModelWithKnife = nullptr;
const dvar_t *store_cgDvar_cg_visionSetLerpMaxIncreasePerFrame = nullptr;
const dvar_t *store_cgDvar_cg_visionSetLerpMaxDecreasePerFrame = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_Allies = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_Axis = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_MyTeam = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_EnemyTeam = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_MyTeamAlt = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_MyTeamAlt_Party = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_EnemyTeamAlt = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_Party = nullptr;
const dvar_t *store_cgDvar_g_TeamColor_Spectator = nullptr;
const dvar_t *store_cgDvar_cg_proneFeetCollisionHull = nullptr;
const dvar_t *store_cgDvar_cg_altDofTrace = nullptr;
const dvar_t *store_cgDvar_cameraShakeRemoteMissile_Angles = nullptr;
const dvar_t *store_cgDvar_cameraShakeRemoteMissile_Freqs = nullptr;
const dvar_t *store_cgDvar_cameraShakeRemoteMissile_SpeedRange = nullptr;
const dvar_t *store_cgDvar_cameraShakeRemoteHelo_Angles = nullptr;
const dvar_t *store_cgDvar_cameraShakeRemoteHelo_Freqs = nullptr;
const dvar_t *store_cgDvar_cameraShakeRemoteHelo_SpeedRange = nullptr;
const dvar_t *store_cgDvar_cg_missile_FOV = nullptr;
const dvar_t *store_cgDvar_cg_EnableAdaptiveSkinLodScale = nullptr;
const dvar_t *store_cgDvar_tu9_ingnoreValidateWeaponOnDeath = nullptr;
const dvar_t *store_cgDvar_nightVisionDisableEffects = nullptr;
const dvar_t *store_cgDvar_flareDisableEffects = nullptr;
const dvar_t *store_cgDvar_g_compassShowEnemies = nullptr;
const dvar_t *store_cgDvar_cg_timedDamageDuration = nullptr;
const dvar_t *store_cgDvar_cg_playerFrustumHalfHeight = nullptr;
const dvar_t *store_cgDvar_cg_overheadNamesTagUpdateInterval = nullptr;
const dvar_t *store_cgDvar_cg_canSeeFriendlyFrustumUpdateInterval = nullptr;
const dvar_t *store_cgDvar_cg_canSeeFriendlyFrustumExpand = nullptr;
const dvar_t *store_cgDvar_cg_canSeeFriendlyFrustumMinDistance = nullptr;
const dvar_t *store_cgDvar_cg_cameraWaterClip = nullptr;
const dvar_t *store_cgDvar_cg_cameraVehicleExitTweenTime = nullptr;
const dvar_t *store_cgDvar_vehicle_piece_damagesfx_threshold = nullptr;
const dvar_t *store_cgDvar_tu6_cg_destructible_radius_damage_enabled = nullptr;
const dvar_t *store_cgDvar_tu11_cg_killstreak_target_diamond_when_emped = nullptr;
const dvar_t *store_cgDvar_cg_useSafeSpectatorCam = nullptr;
const dvar_t *store_cgDvar_cg_fixupSmodelCullDists = nullptr;
const dvar_t *store_cgDvar_cg_alt_mode_killstreak_fix_tu6 = nullptr;
const dvar_t *store_cgDvar_tu7_cg_deathCamAboveWater = nullptr;
const dvar_t *store_cgDvar_tu9_tank_minimap_fix = nullptr;
const dvar_t *store_cgDvar_tu9_projectile_dobj_fix = nullptr;
const dvar_t *store_cgDvar_tu9_clearedMapPackFlagsFix = nullptr;
const dvar_t *store_cgDvar_tu12_cg_vehicleCamAboveWater = nullptr;
const dvar_t *store_cgDvar_tu14_aiAllowForceNoCullCheck = nullptr;
const dvar_t *store_cgDvar_cg_drawTalk = nullptr;
const dvar_t *store_cgDvar_tu12_zm_force_center_cg_cursorhint = nullptr;
const char *const Store_CG_EnumTable_cg_drawBigFPS[] = { "cg_drawBigFPS", nullptr };
const char *const Store_CG_EnumTable_cg_drawFPS[] = { "cg_drawFPS", nullptr };
const char *const Store_CG_EnumTable_snd_drawInfo[] = { "snd_drawInfo", nullptr };
const char *const Store_CG_EnumTable_snd_drawSort[] = { "snd_drawSort", nullptr };
const char *const Store_CG_EnumTable_cg_drawMaterial[] = { "cg_drawMaterial", nullptr };
const char *const Store_CG_EnumTable_cg_thirdPersonMode[] = { "cg_thirdPersonMode", nullptr };
const char *const Store_CG_EnumTable_cg_drawTalk[] = { "cg_drawTalk", nullptr };
}
namespace {
constexpr unsigned Capacity=256, Width=12;
struct Trace { U count; U events[Capacity][Width]; };
Trace trace;
unsigned seed;
U originalBase;
constexpr U enumRvas[] = {0xa59670,0xa59628,0xa59458,0xa595d0,0xa59600,0xa595b0,0xa59698};
const char *const *enumTables[] = {Store_CG_EnumTable_cg_drawBigFPS,Store_CG_EnumTable_cg_drawFPS,Store_CG_EnumTable_snd_drawInfo,Store_CG_EnumTable_snd_drawSort,Store_CG_EnumTable_cg_drawMaterial,Store_CG_EnumTable_cg_thirdPersonMode,Store_CG_EnumTable_cg_drawTalk};
U hash(const char *s) {
    if (!s) return 0;
    U h=14695981039346656037ull;
    for (; *s; ++s) { h ^= static_cast<unsigned char>(*s); h *= 1099511628211ull; }
    return h;
}
U bits(float x) { std::uint32_t r; std::memcpy(&r,&x,4); return r; }
P token(const char *name) { return reinterpret_cast<P>(0x10000000ull | (hash(name)&0xfffffff)); }
U ptr(P p) { return reinterpret_cast<U>(p); }
void event(std::initializer_list<U> args) {
    if (trace.count>=Capacity || args.size()>Width) std::abort();
    auto &dst=trace.events[trace.count++];
    unsigned i=0; for (U x:args) dst[i++]=x;
}
U table(const char *const *t) {
    for (unsigned i=0;i<7;++i)
        if (t==enumTables[i] || (originalBase && reinterpret_cast<U>(t)==originalBase+enumRvas[i])) return i+1;
    std::abort();
}
bool implExit() { bool b=(seed&1)!=0; event({1,b}); return b; }
P implFind(const char *n) {
    const unsigned mode=(seed>>1)%3;
    bool exists=mode==1 || (mode==2 && ((hash(n)^(seed*0x9e3779b97f4a7c15ull))>>((seed>>3)%32)&1));
    P p=exists?token(n):nullptr; event({2,hash(n),ptr(p)}); return p;
}
P implNew(const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *desc) {
    event({3,hash(n),t,f,v->words[0],d->words[0],d->words[1],hash(desc)}); return token(n);
}
void implReregister(P p,const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *desc) {
    event({4,hash(n),t,f,v->words[0],d->words[0],d->words[1],hash(desc),ptr(p)});
}
P implBool(const char *n,bool b,unsigned f,const char *desc) { event({5,hash(n),b,f,hash(desc)}); return token(n); }
P implFloat(const char *n,float a,float b,float c,unsigned f,const char *desc) { event({6,hash(n),bits(a),bits(b),bits(c),f,hash(desc)}); return token(n); }
P implVec2(const char *n,float a,float b,float c,float d,unsigned f) { event({7,hash(n),bits(a),bits(b),bits(c),bits(d),f}); return token(n); }
P implVec3(const char *n,float a,float b,float c,float d,float e,unsigned f) { event({8,hash(n),bits(a),bits(b),bits(c),bits(d),bits(e),f}); return token(n); }
P implColor(const char *n,float a,float b,float c,float d,unsigned f) { event({9,hash(n),bits(a),bits(b),bits(c),bits(d),f}); return token(n); }
P implEnum(const char *n,const char *const *t,int a,unsigned f) { event({10,hash(n),table(t),static_cast<U>(a),f}); return token(n); }
P implString(const char *n,const char *v,unsigned f) { event({11,hash(n),hash(v),f}); return token(n); }
}
extern "C" {
bool Store_CG_ExitAfterConnectPaths() { return implExit(); }
bool MS Mock_ExitAfterConnectPaths() { return implExit(); }
P Store_CG_FindDvar(const char *n) { return implFind(n); }
P MS Mock_FindDvar(const char *n) { return implFind(n); }
P Store_CG_RegisterNew(const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *s) { return implNew(n,t,f,v,d,s); }
P MS Mock_RegisterNew(const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *s) { return implNew(n,t,f,v,d,s); }
void Store_CG_Reregister(P p,const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *s) { implReregister(p,n,t,f,v,d,s); }
void MS Mock_Reregister(P p,const char *n,unsigned t,unsigned f,const Value *v,const Domain *d,const char *s) { implReregister(p,n,t,f,v,d,s); }
P Store_CG_Bool(const char *n,bool b,unsigned f,const char *s) { return implBool(n,b,f,s); }
P MS Mock_Bool(const char *n,bool b,unsigned f,const char *s) { return implBool(n,b,f,s); }
P Store_CG_Float(const char *n,float a,float b,float c,unsigned f,const char *s) { return implFloat(n,a,b,c,f,s); }
P MS Mock_Float(const char *n,float a,float b,float c,unsigned f,const char *s) { return implFloat(n,a,b,c,f,s); }
P Store_CG_Vec2(const char *n,float a,float b,float c,float d,unsigned f) { return implVec2(n,a,b,c,d,f); }
P MS Mock_Vec2(const char *n,float a,float b,float c,float d,unsigned f) { return implVec2(n,a,b,c,d,f); }
P Store_CG_Vec3(const char *n,float a,float b,float c,float d,float e,unsigned f) { return implVec3(n,a,b,c,d,e,f); }
P MS Mock_Vec3(const char *n,float a,float b,float c,float d,float e,unsigned f) { return implVec3(n,a,b,c,d,e,f); }
P Store_CG_Color(const char *n,float a,float b,float c,float d,unsigned f) { return implColor(n,a,b,c,d,f); }
P MS Mock_Color(const char *n,float a,float b,float c,float d,unsigned f) { return implColor(n,a,b,c,d,f); }
P Store_CG_Enum(const char *n,const char *const *t,int a,unsigned f) { return implEnum(n,t,a,f); }
P MS Mock_Enum(const char *n,const char *const *t,int a,unsigned f) { return implEnum(n,t,a,f); }
P Store_CG_String(const char *n,const char *v,unsigned f) { return implString(n,v,f); }
P MS Mock_String(const char *n,const char *v,unsigned f) { return implString(n,v,f); }
void Store_CG_Sub_1400fb7d0() { event({12}); }
void MS Mock_Sub_1400fb7d0() { event({12}); }
void Store_CG_Sub_14022ad80() { event({13}); }
void MS Mock_Sub_14022ad80() { event({13}); }
void Store_CG_Sub_1400c8290() { event({14}); }
void MS Mock_Sub_1400c8290() { event({14}); }
void Store_CG_Sub_1400ba5a0() { event({15}); }
void MS Mock_Sub_1400ba5a0() { event({15}); }
void Store_CG_Sub_1400f7b80() { event({16}); }
void MS Mock_Sub_1400f7b80() { event({16}); }
void Store_CG_Sub_1400f2230() { event({17}); }
void MS Mock_Sub_1400f2230() { event({17}); }
void Store_CG_Sub_14015df60() { event({18}); }
void MS Mock_Sub_14015df60() { event({18}); }
void Store_CG_Sub_140179890() { event({19}); }
void MS Mock_Sub_140179890() { event({19}); }
void Store_CG_Sub_14061baf0() { event({20}); }
void MS Mock_Sub_14061baf0() { event({20}); }
void Store_CG_Sub_140101410(int x) { event({21,static_cast<U>(x)}); }
void MS Mock_Sub_140101410(int x) { event({21,static_cast<U>(x)}); }
void *CgRegistrationMock(unsigned index) {
    void *const targets[] = {reinterpret_cast<void *>(&Mock_ExitAfterConnectPaths), reinterpret_cast<void *>(&Mock_FindDvar), reinterpret_cast<void *>(&Mock_RegisterNew), reinterpret_cast<void *>(&Mock_Reregister), reinterpret_cast<void *>(&Mock_Bool), reinterpret_cast<void *>(&Mock_Float), reinterpret_cast<void *>(&Mock_Vec2), reinterpret_cast<void *>(&Mock_Vec3), reinterpret_cast<void *>(&Mock_Color), reinterpret_cast<void *>(&Mock_Enum), reinterpret_cast<void *>(&Mock_String), reinterpret_cast<void *>(&Mock_Sub_1400fb7d0), reinterpret_cast<void *>(&Mock_Sub_14022ad80), reinterpret_cast<void *>(&Mock_Sub_1400c8290), reinterpret_cast<void *>(&Mock_Sub_1400ba5a0), reinterpret_cast<void *>(&Mock_Sub_1400f7b80), reinterpret_cast<void *>(&Mock_Sub_1400f2230), reinterpret_cast<void *>(&Mock_Sub_14015df60), reinterpret_cast<void *>(&Mock_Sub_140179890), reinterpret_cast<void *>(&Mock_Sub_14061baf0), reinterpret_cast<void *>(&Mock_Sub_140101410)};
    return index<sizeof(targets)/sizeof(*targets)?targets[index]:nullptr;
}
unsigned CgRegistrationSize() { return sizeof(trace); }
unsigned CgRegistrationGlobals(U *out) {
    out[0]=ptr(store_cgDvar_g_loadScripts);
    out[1]=ptr(store_cgDvar_cg_drawGun);
    out[2]=ptr(store_cgDvar_cg_cursorHints);
    out[3]=ptr(store_cgDvar_cg_retrieveHintTime);
    out[4]=ptr(store_cgDvar_cg_retrieveHintTimeStuck);
    out[5]=ptr(store_cgDvar_cg_weaponHintsCoD1Style);
    out[6]=ptr(store_cgDvar_cg_fov);
    out[7]=ptr(store_cgDvar_cg_fovScale);
    out[8]=ptr(store_cgDvar_cg_fovMin);
    out[9]=ptr(store_cgDvar_cg_fovExtraCam);
    out[10]=ptr(store_cgDvar_cg_useWeaponBasedVariableZoom);
    out[11]=ptr(store_cgDvar_cg_viewVehicleInfluenceGunner);
    out[12]=ptr(store_cgDvar_cg_viewVehicleInfluenceGunnerFiring);
    out[13]=ptr(store_cgDvar_cg_draw2D);
    out[14]=ptr(store_cgDvar_cg_disableplayernames);
    out[15]=ptr(store_cgDvar_cg_drawHealth);
    out[16]=ptr(store_cgDvar_cg_drawBreathHint);
    out[17]=ptr(store_cgDvar_cg_drawBigFPS);
    out[18]=ptr(store_cgDvar_cg_drawlagometer);
    out[19]=ptr(store_cgDvar_cg_drawFPS);
    out[20]=ptr(store_cgDvar_cg_drawCrosshair);
    out[21]=ptr(store_cgDvar_cg_drawCrosshairNames);
    out[22]=ptr(store_cgDvar_cg_hudGrenadeIconMaxRangeFrag);
    out[23]=ptr(store_cgDvar_cg_hudGrenadeIconMaxRangeFlash);
    out[24]=ptr(store_cgDvar_cg_hudGrenadeIconOffset);
    out[25]=ptr(store_cgDvar_cg_hudGrenadeIconHeight);
    out[26]=ptr(store_cgDvar_cg_hudGrenadeIconWidth);
    out[27]=ptr(store_cgDvar_cg_hudGrenadePointerHeight);
    out[28]=ptr(store_cgDvar_cg_hudGrenadePointerWidth);
    out[29]=ptr(store_cgDvar_cg_hudGrenadePointerPivot);
    out[30]=ptr(store_cgDvar_lagometer_pos);
    out[31]=ptr(store_cgDvar_cg_centertime);
    out[32]=ptr(store_cgDvar_cg_crosshairAlpha);
    out[33]=ptr(store_cgDvar_cg_crosshairAlphaMin);
    out[34]=ptr(store_cgDvar_cg_crosshairDynamic);
    out[35]=ptr(store_cgDvar_cg_crosshairEnemyColor);
    out[36]=ptr(store_cgDvar_cg_debugposition);
    out[37]=ptr(store_cgDvar_cg_debugevents);
    out[38]=ptr(store_cgDvar_cg_airburstLaseTime);
    out[39]=ptr(store_cgDvar_cg_airburstStrafeRestriction);
    out[40]=ptr(store_cgDvar_cg_airburstCameraRestriction);
    out[41]=ptr(store_cgDvar_cg_airburstLaseFlashTime);
    out[42]=ptr(store_cgDvar_cg_thirdPersonRange);
    out[43]=ptr(store_cgDvar_cg_thirdPersonAngle);
    out[44]=ptr(store_cgDvar_cg_thirdPerson);
    out[45]=ptr(store_cgDvar_cg_thirdPersonMode);
    out[46]=ptr(store_cgDvar_cg_fakefireWizbyChance);
    out[47]=ptr(store_cgDvar_cg_useWeaponSwitchReloadCancel);
    out[48]=ptr(store_cgDvar_cg_enableHelicopterNoCullLodOut);
    out[49]=ptr(store_cgDvar_cl_paused);
    out[50]=ptr(store_cgDvar_cg_drawpaused);
    out[51]=ptr(store_cgDvar_g_synchronousClients);
    out[52]=ptr(store_cgDvar_cg_dumpAnims);
    out[53]=ptr(store_cgDvar_cg_subtitleWidthStandard);
    out[54]=ptr(store_cgDvar_cg_subtitleWidthWidescreen);
    out[55]=ptr(store_cgDvar_cg_constantSizeHeadIcons);
    out[56]=ptr(store_cgDvar_cg_headIconMinScreenRadius);
    out[57]=ptr(store_cgDvar_cg_overheadNamesMaxDist);
    out[58]=ptr(store_cgDvar_cg_overheadNamesNearDist);
    out[59]=ptr(store_cgDvar_cg_overheadNamesFarDist);
    out[60]=ptr(store_cgDvar_cg_overheadNamesFarScale);
    out[61]=ptr(store_cgDvar_cg_overheadNamesSize);
    out[62]=ptr(store_cgDvar_cg_overheadIconSize);
    out[63]=ptr(store_cgDvar_cg_overheadRankSize);
    out[64]=ptr(store_cgDvar_cg_overheadNamesGlow);
    out[65]=ptr(store_cgDvar_cg_drawFriendlyNames);
    out[66]=ptr(store_cgDvar_cg_enemyNameFadeIn);
    out[67]=ptr(store_cgDvar_cg_friendlyNameFadeIn);
    out[68]=ptr(store_cgDvar_cg_enemyNameFadeOut);
    out[69]=ptr(store_cgDvar_cg_friendlyNameFadeOut);
    out[70]=ptr(store_cgDvar_cg_playerHighlightTargetSize);
    out[71]=ptr(store_cgDvar_cg_maxPlayerHighlightTargetSize);
    out[72]=ptr(store_cgDvar_cg_adsZScaleMax);
    out[73]=ptr(store_cgDvar_ui_email_address);
    out[74]=ptr(store_cgDvar_cg_viewZSmoothingMin);
    out[75]=ptr(store_cgDvar_cg_viewZSmoothingMax);
    out[76]=ptr(store_cgDvar_cg_viewZSmoothingTime);
    out[77]=ptr(store_cgDvar_overrideNVGModelWithKnife);
    out[78]=ptr(store_cgDvar_cg_visionSetLerpMaxIncreasePerFrame);
    out[79]=ptr(store_cgDvar_cg_visionSetLerpMaxDecreasePerFrame);
    out[80]=ptr(store_cgDvar_g_TeamColor_Allies);
    out[81]=ptr(store_cgDvar_g_TeamColor_Axis);
    out[82]=ptr(store_cgDvar_g_TeamColor_MyTeam);
    out[83]=ptr(store_cgDvar_g_TeamColor_EnemyTeam);
    out[84]=ptr(store_cgDvar_g_TeamColor_MyTeamAlt);
    out[85]=ptr(store_cgDvar_g_TeamColor_MyTeamAlt_Party);
    out[86]=ptr(store_cgDvar_g_TeamColor_EnemyTeamAlt);
    out[87]=ptr(store_cgDvar_g_TeamColor_Party);
    out[88]=ptr(store_cgDvar_g_TeamColor_Spectator);
    out[89]=ptr(store_cgDvar_cg_proneFeetCollisionHull);
    out[90]=ptr(store_cgDvar_cg_altDofTrace);
    out[91]=ptr(store_cgDvar_cameraShakeRemoteMissile_Angles);
    out[92]=ptr(store_cgDvar_cameraShakeRemoteMissile_Freqs);
    out[93]=ptr(store_cgDvar_cameraShakeRemoteMissile_SpeedRange);
    out[94]=ptr(store_cgDvar_cameraShakeRemoteHelo_Angles);
    out[95]=ptr(store_cgDvar_cameraShakeRemoteHelo_Freqs);
    out[96]=ptr(store_cgDvar_cameraShakeRemoteHelo_SpeedRange);
    out[97]=ptr(store_cgDvar_cg_missile_FOV);
    out[98]=ptr(store_cgDvar_cg_EnableAdaptiveSkinLodScale);
    out[99]=ptr(store_cgDvar_tu9_ingnoreValidateWeaponOnDeath);
    out[100]=ptr(store_cgDvar_nightVisionDisableEffects);
    out[101]=ptr(store_cgDvar_flareDisableEffects);
    out[102]=ptr(store_cgDvar_g_compassShowEnemies);
    out[103]=ptr(store_cgDvar_cg_timedDamageDuration);
    out[104]=ptr(store_cgDvar_cg_playerFrustumHalfHeight);
    out[105]=ptr(store_cgDvar_cg_overheadNamesTagUpdateInterval);
    out[106]=ptr(store_cgDvar_cg_canSeeFriendlyFrustumUpdateInterval);
    out[107]=ptr(store_cgDvar_cg_canSeeFriendlyFrustumExpand);
    out[108]=ptr(store_cgDvar_cg_canSeeFriendlyFrustumMinDistance);
    out[109]=ptr(store_cgDvar_cg_cameraWaterClip);
    out[110]=ptr(store_cgDvar_cg_cameraVehicleExitTweenTime);
    out[111]=ptr(store_cgDvar_vehicle_piece_damagesfx_threshold);
    out[112]=ptr(store_cgDvar_tu6_cg_destructible_radius_damage_enabled);
    out[113]=ptr(store_cgDvar_tu11_cg_killstreak_target_diamond_when_emped);
    out[114]=ptr(store_cgDvar_cg_useSafeSpectatorCam);
    out[115]=ptr(store_cgDvar_cg_fixupSmodelCullDists);
    out[116]=ptr(store_cgDvar_cg_alt_mode_killstreak_fix_tu6);
    out[117]=ptr(store_cgDvar_tu7_cg_deathCamAboveWater);
    out[118]=ptr(store_cgDvar_tu9_tank_minimap_fix);
    out[119]=ptr(store_cgDvar_tu9_projectile_dobj_fix);
    out[120]=ptr(store_cgDvar_tu9_clearedMapPackFlagsFix);
    out[121]=ptr(store_cgDvar_tu12_cg_vehicleCamAboveWater);
    out[122]=ptr(store_cgDvar_tu14_aiAllowForceNoCullCheck);
    out[123]=ptr(store_cgDvar_cg_drawTalk);
    out[124]=ptr(store_cgDvar_tu12_zm_force_center_cg_cursorhint);
    return 125;
}
void CgRegistrationRun(void *base, unsigned input, void *out) {
    trace={}; seed=input; originalBase=reinterpret_cast<U>(base);
    if (base) {
        using Entry=void (MS *)();
        reinterpret_cast<Entry>(originalBase+0x101580)();
    } else {
        store_cgDvar_g_loadScripts=nullptr;
        store_cgDvar_cg_drawGun=nullptr;
        store_cgDvar_cg_cursorHints=nullptr;
        store_cgDvar_cg_retrieveHintTime=nullptr;
        store_cgDvar_cg_retrieveHintTimeStuck=nullptr;
        store_cgDvar_cg_weaponHintsCoD1Style=nullptr;
        store_cgDvar_cg_fov=nullptr;
        store_cgDvar_cg_fovScale=nullptr;
        store_cgDvar_cg_fovMin=nullptr;
        store_cgDvar_cg_fovExtraCam=nullptr;
        store_cgDvar_cg_useWeaponBasedVariableZoom=nullptr;
        store_cgDvar_cg_viewVehicleInfluenceGunner=nullptr;
        store_cgDvar_cg_viewVehicleInfluenceGunnerFiring=nullptr;
        store_cgDvar_cg_draw2D=nullptr;
        store_cgDvar_cg_disableplayernames=nullptr;
        store_cgDvar_cg_drawHealth=nullptr;
        store_cgDvar_cg_drawBreathHint=nullptr;
        store_cgDvar_cg_drawBigFPS=nullptr;
        store_cgDvar_cg_drawlagometer=nullptr;
        store_cgDvar_cg_drawFPS=nullptr;
        store_cgDvar_cg_drawCrosshair=nullptr;
        store_cgDvar_cg_drawCrosshairNames=nullptr;
        store_cgDvar_cg_hudGrenadeIconMaxRangeFrag=nullptr;
        store_cgDvar_cg_hudGrenadeIconMaxRangeFlash=nullptr;
        store_cgDvar_cg_hudGrenadeIconOffset=nullptr;
        store_cgDvar_cg_hudGrenadeIconHeight=nullptr;
        store_cgDvar_cg_hudGrenadeIconWidth=nullptr;
        store_cgDvar_cg_hudGrenadePointerHeight=nullptr;
        store_cgDvar_cg_hudGrenadePointerWidth=nullptr;
        store_cgDvar_cg_hudGrenadePointerPivot=nullptr;
        store_cgDvar_lagometer_pos=nullptr;
        store_cgDvar_cg_centertime=nullptr;
        store_cgDvar_cg_crosshairAlpha=nullptr;
        store_cgDvar_cg_crosshairAlphaMin=nullptr;
        store_cgDvar_cg_crosshairDynamic=nullptr;
        store_cgDvar_cg_crosshairEnemyColor=nullptr;
        store_cgDvar_cg_debugposition=nullptr;
        store_cgDvar_cg_debugevents=nullptr;
        store_cgDvar_cg_airburstLaseTime=nullptr;
        store_cgDvar_cg_airburstStrafeRestriction=nullptr;
        store_cgDvar_cg_airburstCameraRestriction=nullptr;
        store_cgDvar_cg_airburstLaseFlashTime=nullptr;
        store_cgDvar_cg_thirdPersonRange=nullptr;
        store_cgDvar_cg_thirdPersonAngle=nullptr;
        store_cgDvar_cg_thirdPerson=nullptr;
        store_cgDvar_cg_thirdPersonMode=nullptr;
        store_cgDvar_cg_fakefireWizbyChance=nullptr;
        store_cgDvar_cg_useWeaponSwitchReloadCancel=nullptr;
        store_cgDvar_cg_enableHelicopterNoCullLodOut=nullptr;
        store_cgDvar_cl_paused=nullptr;
        store_cgDvar_cg_drawpaused=nullptr;
        store_cgDvar_g_synchronousClients=nullptr;
        store_cgDvar_cg_dumpAnims=nullptr;
        store_cgDvar_cg_subtitleWidthStandard=nullptr;
        store_cgDvar_cg_subtitleWidthWidescreen=nullptr;
        store_cgDvar_cg_constantSizeHeadIcons=nullptr;
        store_cgDvar_cg_headIconMinScreenRadius=nullptr;
        store_cgDvar_cg_overheadNamesMaxDist=nullptr;
        store_cgDvar_cg_overheadNamesNearDist=nullptr;
        store_cgDvar_cg_overheadNamesFarDist=nullptr;
        store_cgDvar_cg_overheadNamesFarScale=nullptr;
        store_cgDvar_cg_overheadNamesSize=nullptr;
        store_cgDvar_cg_overheadIconSize=nullptr;
        store_cgDvar_cg_overheadRankSize=nullptr;
        store_cgDvar_cg_overheadNamesGlow=nullptr;
        store_cgDvar_cg_drawFriendlyNames=nullptr;
        store_cgDvar_cg_enemyNameFadeIn=nullptr;
        store_cgDvar_cg_friendlyNameFadeIn=nullptr;
        store_cgDvar_cg_enemyNameFadeOut=nullptr;
        store_cgDvar_cg_friendlyNameFadeOut=nullptr;
        store_cgDvar_cg_playerHighlightTargetSize=nullptr;
        store_cgDvar_cg_maxPlayerHighlightTargetSize=nullptr;
        store_cgDvar_cg_adsZScaleMax=nullptr;
        store_cgDvar_ui_email_address=nullptr;
        store_cgDvar_cg_viewZSmoothingMin=nullptr;
        store_cgDvar_cg_viewZSmoothingMax=nullptr;
        store_cgDvar_cg_viewZSmoothingTime=nullptr;
        store_cgDvar_overrideNVGModelWithKnife=nullptr;
        store_cgDvar_cg_visionSetLerpMaxIncreasePerFrame=nullptr;
        store_cgDvar_cg_visionSetLerpMaxDecreasePerFrame=nullptr;
        store_cgDvar_g_TeamColor_Allies=nullptr;
        store_cgDvar_g_TeamColor_Axis=nullptr;
        store_cgDvar_g_TeamColor_MyTeam=nullptr;
        store_cgDvar_g_TeamColor_EnemyTeam=nullptr;
        store_cgDvar_g_TeamColor_MyTeamAlt=nullptr;
        store_cgDvar_g_TeamColor_MyTeamAlt_Party=nullptr;
        store_cgDvar_g_TeamColor_EnemyTeamAlt=nullptr;
        store_cgDvar_g_TeamColor_Party=nullptr;
        store_cgDvar_g_TeamColor_Spectator=nullptr;
        store_cgDvar_cg_proneFeetCollisionHull=nullptr;
        store_cgDvar_cg_altDofTrace=nullptr;
        store_cgDvar_cameraShakeRemoteMissile_Angles=nullptr;
        store_cgDvar_cameraShakeRemoteMissile_Freqs=nullptr;
        store_cgDvar_cameraShakeRemoteMissile_SpeedRange=nullptr;
        store_cgDvar_cameraShakeRemoteHelo_Angles=nullptr;
        store_cgDvar_cameraShakeRemoteHelo_Freqs=nullptr;
        store_cgDvar_cameraShakeRemoteHelo_SpeedRange=nullptr;
        store_cgDvar_cg_missile_FOV=nullptr;
        store_cgDvar_cg_EnableAdaptiveSkinLodScale=nullptr;
        store_cgDvar_tu9_ingnoreValidateWeaponOnDeath=nullptr;
        store_cgDvar_nightVisionDisableEffects=nullptr;
        store_cgDvar_flareDisableEffects=nullptr;
        store_cgDvar_g_compassShowEnemies=nullptr;
        store_cgDvar_cg_timedDamageDuration=nullptr;
        store_cgDvar_cg_playerFrustumHalfHeight=nullptr;
        store_cgDvar_cg_overheadNamesTagUpdateInterval=nullptr;
        store_cgDvar_cg_canSeeFriendlyFrustumUpdateInterval=nullptr;
        store_cgDvar_cg_canSeeFriendlyFrustumExpand=nullptr;
        store_cgDvar_cg_canSeeFriendlyFrustumMinDistance=nullptr;
        store_cgDvar_cg_cameraWaterClip=nullptr;
        store_cgDvar_cg_cameraVehicleExitTweenTime=nullptr;
        store_cgDvar_vehicle_piece_damagesfx_threshold=nullptr;
        store_cgDvar_tu6_cg_destructible_radius_damage_enabled=nullptr;
        store_cgDvar_tu11_cg_killstreak_target_diamond_when_emped=nullptr;
        store_cgDvar_cg_useSafeSpectatorCam=nullptr;
        store_cgDvar_cg_fixupSmodelCullDists=nullptr;
        store_cgDvar_cg_alt_mode_killstreak_fix_tu6=nullptr;
        store_cgDvar_tu7_cg_deathCamAboveWater=nullptr;
        store_cgDvar_tu9_tank_minimap_fix=nullptr;
        store_cgDvar_tu9_projectile_dobj_fix=nullptr;
        store_cgDvar_tu9_clearedMapPackFlagsFix=nullptr;
        store_cgDvar_tu12_cg_vehicleCamAboveWater=nullptr;
        store_cgDvar_tu14_aiAllowForceNoCullCheck=nullptr;
        store_cgDvar_cg_drawTalk=nullptr;
        store_cgDvar_tu12_zm_force_center_cg_cursorhint=nullptr;
        CG_RegisterDvars_Project();
    }
    std::memcpy(out,&trace,sizeof(trace));
}
}
