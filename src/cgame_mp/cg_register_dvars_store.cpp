// Full candidate Store MP CG_RegisterDvars_Project (140101580..140103C9F).
// All registration operations, global stores and subsidiary calls are present.
// Replay-only until original machine-code differential testing and ZM/SP ABI bindings are complete.
#define T6_SOURCE "cgame_mp/cg_main_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CG_PROJECT_REPLAY)
#include <cstdint>
#include <cstring>
struct dvar_t;
namespace {
union alignas(16) Value { std::uint32_t words[4]; const char *string; };
struct alignas(16) Domain { std::uint32_t words[4]; };
float F(std::uint32_t bits) { float result; std::memcpy(&result, &bits, sizeof(result)); return result; }
}
extern "C" {
bool Store_CG_ExitAfterConnectPaths(); // 140297EB0; provisional reference identity
const dvar_t *Store_CG_FindDvar(const char *);
const dvar_t *Store_CG_RegisterNew(const char *, unsigned, unsigned, const Value *, const Domain *, const char *);
void Store_CG_Reregister(const dvar_t *, const char *, unsigned, unsigned, const Value *, const Domain *, const char *);
const dvar_t *Store_CG_Bool(const char *, bool, unsigned, const char *);
const dvar_t *Store_CG_Float(const char *, float, float, float, unsigned, const char *);
const dvar_t *Store_CG_Vec2(const char *, float, float, float, float, unsigned);
const dvar_t *Store_CG_Vec3(const char *, float, float, float, float, float, unsigned);
const dvar_t *Store_CG_Color(const char *, float, float, float, float, unsigned);
const dvar_t *Store_CG_Enum(const char *, const char *const *, int, unsigned);
const dvar_t *Store_CG_String(const char *, const char *, unsigned);
void Store_CG_Sub_1400fb7d0();
void Store_CG_Sub_14022ad80();
void Store_CG_Sub_1400c8290();
void Store_CG_Sub_1400ba5a0();
void Store_CG_Sub_1400f7b80();
void Store_CG_Sub_1400f2230();
void Store_CG_Sub_14015df60();
void Store_CG_Sub_140179890();
void Store_CG_Sub_14061baf0();
void Store_CG_Sub_140101410(int);
extern const char *const Store_CG_EnumTable_cg_drawBigFPS[]; // 140A59670
extern const char *const Store_CG_EnumTable_cg_drawFPS[]; // 140A59628
extern const char *const Store_CG_EnumTable_snd_drawInfo[]; // 140A59458
extern const char *const Store_CG_EnumTable_snd_drawSort[]; // 140A595D0
extern const char *const Store_CG_EnumTable_cg_drawMaterial[]; // 140A59600
extern const char *const Store_CG_EnumTable_cg_thirdPersonMode[]; // 140A595B0
extern const char *const Store_CG_EnumTable_cg_drawTalk[]; // 140A59698
extern const dvar_t *store_cgDvar_g_loadScripts; // 140D76888
extern const dvar_t *store_cgDvar_cg_drawGun; // 140E53B20
extern const dvar_t *store_cgDvar_cg_cursorHints; // 140E4F838
extern const dvar_t *store_cgDvar_cg_retrieveHintTime; // 140E53B60
extern const dvar_t *store_cgDvar_cg_retrieveHintTimeStuck; // 140E53B68
extern const dvar_t *store_cgDvar_cg_weaponHintsCoD1Style; // 140D78660
extern const dvar_t *store_cgDvar_cg_fov; // 140E4F8E8
extern const dvar_t *store_cgDvar_cg_fovScale; // 140D78658
extern const dvar_t *store_cgDvar_cg_fovMin; // 140D78648
extern const dvar_t *store_cgDvar_cg_fovExtraCam; // 140D78638
extern const dvar_t *store_cgDvar_cg_useWeaponBasedVariableZoom; // 140D78650
extern const dvar_t *store_cgDvar_cg_viewVehicleInfluenceGunner; // 140D78628
extern const dvar_t *store_cgDvar_cg_viewVehicleInfluenceGunnerFiring; // 140D78618
extern const dvar_t *store_cgDvar_cg_draw2D; // 140E53A40
extern const dvar_t *store_cgDvar_cg_disableplayernames; // 140E53A38
extern const dvar_t *store_cgDvar_cg_drawHealth; // 140E4BAA8
extern const dvar_t *store_cgDvar_cg_drawBreathHint; // 140E4F828
extern const dvar_t *store_cgDvar_cg_drawBigFPS; // 140E4F890
extern const dvar_t *store_cgDvar_cg_drawlagometer; // 140E53AE8
extern const dvar_t *store_cgDvar_cg_drawFPS; // 140E4F888
extern const dvar_t *store_cgDvar_cg_drawCrosshair; // 140E53B30
extern const dvar_t *store_cgDvar_cg_drawCrosshairNames; // 140E53AC8
extern const dvar_t *store_cgDvar_cg_hudGrenadeIconMaxRangeFrag; // 140E4F8C0
extern const dvar_t *store_cgDvar_cg_hudGrenadeIconMaxRangeFlash; // 140E4F898
extern const dvar_t *store_cgDvar_cg_hudGrenadeIconOffset; // 140E4F8A0
extern const dvar_t *store_cgDvar_cg_hudGrenadeIconHeight; // 140E4F8B8
extern const dvar_t *store_cgDvar_cg_hudGrenadeIconWidth; // 140E4F8C8
extern const dvar_t *store_cgDvar_cg_hudGrenadePointerHeight; // 140E4F8B0
extern const dvar_t *store_cgDvar_cg_hudGrenadePointerWidth; // 140E4F8A8
extern const dvar_t *store_cgDvar_cg_hudGrenadePointerPivot; // 140E4F8D0
extern const dvar_t *store_cgDvar_lagometer_pos; // 140E53AE0
extern const dvar_t *store_cgDvar_cg_centertime; // 140E53A28
extern const dvar_t *store_cgDvar_cg_crosshairAlpha; // 140E53B08
extern const dvar_t *store_cgDvar_cg_crosshairAlphaMin; // 140E53B00
extern const dvar_t *store_cgDvar_cg_crosshairDynamic; // 140E53B28
extern const dvar_t *store_cgDvar_cg_crosshairEnemyColor; // 140E53B18
extern const dvar_t *store_cgDvar_cg_debugposition; // 140E4BAE0
extern const dvar_t *store_cgDvar_cg_debugevents; // 140E53B80
extern const dvar_t *store_cgDvar_cg_airburstLaseTime; // 140E49A38
extern const dvar_t *store_cgDvar_cg_airburstStrafeRestriction; // 140E4F870
extern const dvar_t *store_cgDvar_cg_airburstCameraRestriction; // 140E4F860
extern const dvar_t *store_cgDvar_cg_airburstLaseFlashTime; // 140E4BAC8
extern const dvar_t *store_cgDvar_cg_thirdPersonRange; // 140E4BA98
extern const dvar_t *store_cgDvar_cg_thirdPersonAngle; // 140E4BA78
extern const dvar_t *store_cgDvar_cg_thirdPerson; // 140E4BA90
extern const dvar_t *store_cgDvar_cg_thirdPersonMode; // 140E4BAE8
extern const dvar_t *store_cgDvar_cg_fakefireWizbyChance; // 140E4BAD0
extern const dvar_t *store_cgDvar_cg_useWeaponSwitchReloadCancel; // 140E4F878
extern const dvar_t *store_cgDvar_cg_enableHelicopterNoCullLodOut; // 140D78610
extern const dvar_t *store_cgDvar_cl_paused; // 140E53A30
extern const dvar_t *store_cgDvar_cg_drawpaused; // 140E53B40
extern const dvar_t *store_cgDvar_g_synchronousClients; // 140E53AF0
extern const dvar_t *store_cgDvar_cg_dumpAnims; // 140E4F868
extern const dvar_t *store_cgDvar_cg_subtitleWidthStandard; // 140E4BAC0
extern const dvar_t *store_cgDvar_cg_subtitleWidthWidescreen; // 140D78668
extern const dvar_t *store_cgDvar_cg_constantSizeHeadIcons; // 140E4BAB0
extern const dvar_t *store_cgDvar_cg_headIconMinScreenRadius; // 140E4F830
extern const dvar_t *store_cgDvar_cg_overheadNamesMaxDist; // 140E53A48
extern const dvar_t *store_cgDvar_cg_overheadNamesNearDist; // 140E53AB0
extern const dvar_t *store_cgDvar_cg_overheadNamesFarDist; // 140E53A90
extern const dvar_t *store_cgDvar_cg_overheadNamesFarScale; // 140E53AA8
extern const dvar_t *store_cgDvar_cg_overheadNamesSize; // 140E53AA0
extern const dvar_t *store_cgDvar_cg_overheadIconSize; // 140E53AB8
extern const dvar_t *store_cgDvar_cg_overheadRankSize; // 140E53A80
extern const dvar_t *store_cgDvar_cg_overheadNamesGlow; // 140E53A88
extern const dvar_t *store_cgDvar_cg_drawFriendlyNames; // 140E53A70
extern const dvar_t *store_cgDvar_cg_enemyNameFadeIn; // 140E53A50
extern const dvar_t *store_cgDvar_cg_friendlyNameFadeIn; // 140E53AD0
extern const dvar_t *store_cgDvar_cg_enemyNameFadeOut; // 140E53AC0
extern const dvar_t *store_cgDvar_cg_friendlyNameFadeOut; // 140E53A58
extern const dvar_t *store_cgDvar_cg_playerHighlightTargetSize; // 140D78620
extern const dvar_t *store_cgDvar_cg_maxPlayerHighlightTargetSize; // 140E49A40
extern const dvar_t *store_cgDvar_cg_adsZScaleMax; // 140E49A20
extern const dvar_t *store_cgDvar_ui_email_address; // 140E4BA88
extern const dvar_t *store_cgDvar_cg_viewZSmoothingMin; // 140E4F880
extern const dvar_t *store_cgDvar_cg_viewZSmoothingMax; // 140E4BA68
extern const dvar_t *store_cgDvar_cg_viewZSmoothingTime; // 140E4BA58
extern const dvar_t *store_cgDvar_overrideNVGModelWithKnife; // 140E4F840
extern const dvar_t *store_cgDvar_cg_visionSetLerpMaxIncreasePerFrame; // 140E49A00
extern const dvar_t *store_cgDvar_cg_visionSetLerpMaxDecreasePerFrame; // 140E4F850
extern const dvar_t *store_cgDvar_g_TeamColor_Allies; // 140E4BAD8
extern const dvar_t *store_cgDvar_g_TeamColor_Axis; // 140E49A48
extern const dvar_t *store_cgDvar_g_TeamColor_MyTeam; // 140E53B10
extern const dvar_t *store_cgDvar_g_TeamColor_EnemyTeam; // 140E53AF8
extern const dvar_t *store_cgDvar_g_TeamColor_MyTeamAlt; // 140E53B48
extern const dvar_t *store_cgDvar_g_TeamColor_MyTeamAlt_Party; // 140E49A10
extern const dvar_t *store_cgDvar_g_TeamColor_EnemyTeamAlt; // 140E53B38
extern const dvar_t *store_cgDvar_g_TeamColor_Party; // 140E53B50
extern const dvar_t *store_cgDvar_g_TeamColor_Spectator; // 140E53B58
extern const dvar_t *store_cgDvar_cg_proneFeetCollisionHull; // 140E4BA80
extern const dvar_t *store_cgDvar_cg_altDofTrace; // 143C08D38
extern const dvar_t *store_cgDvar_cameraShakeRemoteMissile_Angles; // 140E5A660
extern const dvar_t *store_cgDvar_cameraShakeRemoteMissile_Freqs; // 140E5A668
extern const dvar_t *store_cgDvar_cameraShakeRemoteMissile_SpeedRange; // 140E5A658
extern const dvar_t *store_cgDvar_cameraShakeRemoteHelo_Angles; // 140E5A680
extern const dvar_t *store_cgDvar_cameraShakeRemoteHelo_Freqs; // 140E5A688
extern const dvar_t *store_cgDvar_cameraShakeRemoteHelo_SpeedRange; // 140E5A678
extern const dvar_t *store_cgDvar_cg_missile_FOV; // 140E5A670
extern const dvar_t *store_cgDvar_cg_EnableAdaptiveSkinLodScale; // 140E5A650
extern const dvar_t *store_cgDvar_tu9_ingnoreValidateWeaponOnDeath; // 140E5A648
extern const dvar_t *store_cgDvar_nightVisionDisableEffects; // 143C08D98
extern const dvar_t *store_cgDvar_flareDisableEffects; // 143C0A628
extern const dvar_t *store_cgDvar_g_compassShowEnemies; // 141D05950
extern const dvar_t *store_cgDvar_cg_timedDamageDuration; // 140E4F8D8
extern const dvar_t *store_cgDvar_cg_playerFrustumHalfHeight; // 140E4BA60
extern const dvar_t *store_cgDvar_cg_overheadNamesTagUpdateInterval; // 140E53A60
extern const dvar_t *store_cgDvar_cg_canSeeFriendlyFrustumUpdateInterval; // 140E53A98
extern const dvar_t *store_cgDvar_cg_canSeeFriendlyFrustumExpand; // 140E53AD8
extern const dvar_t *store_cgDvar_cg_canSeeFriendlyFrustumMinDistance; // 140E53A78
extern const dvar_t *store_cgDvar_cg_cameraWaterClip; // 140E4BA70
extern const dvar_t *store_cgDvar_cg_cameraVehicleExitTweenTime; // 140D78630
extern const dvar_t *store_cgDvar_vehicle_piece_damagesfx_threshold; // 140E49A08
extern const dvar_t *store_cgDvar_tu6_cg_destructible_radius_damage_enabled; // 140E49A18
extern const dvar_t *store_cgDvar_tu11_cg_killstreak_target_diamond_when_emped; // 140E53A68
extern const dvar_t *store_cgDvar_cg_useSafeSpectatorCam; // 140E499F8
extern const dvar_t *store_cgDvar_cg_fixupSmodelCullDists; // 140E4BAA0
extern const dvar_t *store_cgDvar_cg_alt_mode_killstreak_fix_tu6; // 140E4BAB8
extern const dvar_t *store_cgDvar_tu7_cg_deathCamAboveWater; // 140E4F848
extern const dvar_t *store_cgDvar_tu9_tank_minimap_fix; // 140E53B78
extern const dvar_t *store_cgDvar_tu9_projectile_dobj_fix; // 140E49A28
extern const dvar_t *store_cgDvar_tu9_clearedMapPackFlagsFix; // 140E49A30
extern const dvar_t *store_cgDvar_tu12_cg_vehicleCamAboveWater; // 140D78640
extern const dvar_t *store_cgDvar_tu14_aiAllowForceNoCullCheck; // 140D76880
extern const dvar_t *store_cgDvar_cg_drawTalk; // 140E53B88
extern const dvar_t *store_cgDvar_tu12_zm_force_center_cg_cursorhint; // 140E53B98
}
namespace {
const dvar_t *Int(const char *name, unsigned value, unsigned min, unsigned max, unsigned flags) {
    const Value v{{value}};
    const Domain domain{{min, max}};
    if (const auto *dvar = Store_CG_FindDvar(name)) {
        Store_CG_Reregister(dvar, name, 6, flags, &v, &domain, "");
        return dvar;
    }
    return Store_CG_RegisterNew(name, 6, flags, &v, &domain, "");
}
}
void CG_RegisterDvars_Project() {
    store_cgDvar_g_loadScripts = Store_CG_Bool("g_loadScripts", !Store_CG_ExitAfterConnectPaths(), 0x0u, "");
    Store_CG_Bool("cg_usingClientScripts", true, 0x80u, "");
    store_cgDvar_cg_drawGun = Store_CG_Bool("cg_drawGun", true, 0x80u, "");
    store_cgDvar_cg_cursorHints = Int("cg_cursorHints", 0x4u, 0x0u, 0x4u, 0x1u);
    store_cgDvar_cg_retrieveHintTime = Int("cg_retrieveHintTime", 0x0u, 0x0u, 0x7fffffffu, 0x80u);
    store_cgDvar_cg_retrieveHintTimeStuck = Int("cg_retrieveHintTimeStuck", 0x7d0u, 0x0u, 0x7fffffffu, 0x80u);
    store_cgDvar_cg_weaponHintsCoD1Style = Store_CG_Bool("cg_weaponHintsCoD1Style", true, 0x1000u, "");
    store_cgDvar_cg_fov = Store_CG_Float("cg_fov", F(0x42820000u), F(0x3f800000u), F(0x43200000u), 0x80u, "");
    store_cgDvar_cg_fovScale = Store_CG_Float("cg_fovScale", F(0x3f800000u), F(0x3e4ccccdu), F(0x40000000u), 0x80u, "");
    store_cgDvar_cg_fovMin = Store_CG_Float("cg_fovMin", F(0x41200000u), F(0x3f800000u), F(0x43200000u), 0x80u, "");
    store_cgDvar_cg_fovExtraCam = Store_CG_Float("cg_fovExtraCam", F(0x41f00000u), F(0x3f800000u), F(0x43200000u), 0x80u, "");
    store_cgDvar_cg_useWeaponBasedVariableZoom = Store_CG_Bool("cg_useWeaponBasedVariableZoom", true, 0x80u, "");
    store_cgDvar_cg_viewVehicleInfluenceGunner = Store_CG_Vec3("cg_viewVehicleInfluenceGunner", F(0x3f800000u), F(0x3f800000u), F(0x3f800000u), F(0x0u), F(0x3f800000u), 0x1080u);
    store_cgDvar_cg_viewVehicleInfluenceGunnerFiring = Store_CG_Vec3("cg_viewVehicleInfluenceGunnerFiring", F(0x0u), F(0x0u), F(0x0u), F(0x0u), F(0x3f800000u), 0x1080u);
    store_cgDvar_cg_draw2D = Store_CG_Bool("cg_draw2D", true, 0x80u, "");
    store_cgDvar_cg_disableplayernames = Store_CG_Bool("cg_disableplayernames", false, 0x1080u, "");
    store_cgDvar_cg_drawHealth = Store_CG_Bool("cg_drawHealth", false, 0x80u, "");
    store_cgDvar_cg_drawBreathHint = Store_CG_Bool("cg_drawBreathHint", true, 0x1u, "");
    store_cgDvar_cg_drawBigFPS = Store_CG_Enum("cg_drawBigFPS", Store_CG_EnumTable_cg_drawBigFPS, 0, 0x20000u);
    store_cgDvar_cg_drawlagometer = Store_CG_Bool("cg_drawlagometer", false, 0x20000u, "");
    store_cgDvar_cg_drawFPS = Store_CG_Enum("cg_drawFPS", Store_CG_EnumTable_cg_drawFPS, 0, 0x1u);
    Store_CG_Bool("cg_development", false, 0x0u, "");
    Store_CG_Bool("cg_drawFPSOnly", false, 0x1u, "");
    Store_CG_Enum("snd_drawInfo", Store_CG_EnumTable_snd_drawInfo, 0, 0x0u);
    Store_CG_Enum("snd_drawSort", Store_CG_EnumTable_snd_drawSort, 0, 0x0u);
    Store_CG_Enum("cg_drawMaterial", Store_CG_EnumTable_cg_drawMaterial, 0, 0x80u);
    Int("cg_drawMaterialImageNum", 0xffffffffu, 0xfffffffeu, 0x80u, 0x80u);
    Store_CG_String("cg_drawMaterialImageName", "", 0x80u);
    Int("cg_drawModelAxis", 0xffffffffu, 0xffffffffu, 0x100u, 0x80u);
    store_cgDvar_cg_drawCrosshair = Store_CG_Bool("cg_drawCrosshair", true, 0x80u, "");
    Store_CG_Bool("cg_drawHoldBreathHint", true, 0x1u, "");
    store_cgDvar_cg_drawCrosshairNames = Store_CG_Bool("cg_drawCrosshairNames", true, 0x80u, "");
    Int("cg_drawCrosshairNamesPosX", 0x12cu, 0x0u, 0x280u, 0x80u);
    Int("cg_drawCrosshairNamesPosY", 0xb4u, 0x0u, 0x1e0u, 0x80u);
    Store_CG_Bool("cg_drawShellshock", true, 0x80u, "");
    Store_CG_Bool("cg_lastSpectatorSelectedThirdPerson", true, 0x1u, "");
    store_cgDvar_cg_hudGrenadeIconMaxRangeFrag = Store_CG_Float("cg_hudGrenadeIconMaxRangeFrag", F(0x437a0000u), F(0x0u), F(0x447a0000u), 0x1080u, "");
    store_cgDvar_cg_hudGrenadeIconMaxRangeFlash = Store_CG_Float("cg_hudGrenadeIconMaxRangeFlash", F(0x43fa0000u), F(0x0u), F(0x44fa0000u), 0x1080u, "");
    store_cgDvar_cg_hudGrenadeIconOffset = Store_CG_Float("cg_hudGrenadeIconOffset", F(0x42480000u), F(0x0u), F(0x44000000u), 0x1u, "");
    store_cgDvar_cg_hudGrenadeIconHeight = Store_CG_Float("cg_hudGrenadeIconHeight", F(0x41c80000u), F(0x0u), F(0x44000000u), 0x1u, "");
    store_cgDvar_cg_hudGrenadeIconWidth = Store_CG_Float("cg_hudGrenadeIconWidth", F(0x41c80000u), F(0x0u), F(0x44000000u), 0x1u, "");
    store_cgDvar_cg_hudGrenadePointerHeight = Store_CG_Float("cg_hudGrenadePointerHeight", F(0x41400000u), F(0x0u), F(0x44000000u), 0x1u, "");
    store_cgDvar_cg_hudGrenadePointerWidth = Store_CG_Float("cg_hudGrenadePointerWidth", F(0x41c80000u), F(0x0u), F(0x44000000u), 0x1u, "");
    store_cgDvar_cg_hudGrenadePointerPivot = Store_CG_Vec2("cg_hudGrenadePointerPivot", F(0x41400000u), F(0x41d80000u), F(0x0u), F(0x44000000u), 0x1u);
    store_cgDvar_lagometer_pos = Store_CG_Vec2("lagometer_pos", F(0xc2480000u), F(0x42700000u), F(0xff7fffffu), F(0x7f7fffffu), 0x0u);
    store_cgDvar_cg_centertime = Store_CG_Float("cg_centertime", F(0x40a00000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_crosshairAlpha = Store_CG_Float("cg_crosshairAlpha", F(0x3f800000u), F(0x0u), F(0x3f800000u), 0x81u, "");
    store_cgDvar_cg_crosshairAlphaMin = Store_CG_Float("cg_crosshairAlphaMin", F(0x3f000000u), F(0x0u), F(0x3f800000u), 0x81u, "");
    store_cgDvar_cg_crosshairDynamic = Store_CG_Bool("cg_crosshairDynamic", false, 0x81u, "");
    store_cgDvar_cg_crosshairEnemyColor = Store_CG_Bool("cg_crosshairEnemyColor", true, 0x81u, "");
    store_cgDvar_cg_debugposition = Store_CG_Bool("cg_debugposition", false, 0x80u, "");
    store_cgDvar_cg_debugevents = Store_CG_Bool("cg_debugevents", false, 0x80u, "");
    Store_CG_Bool("cg_footsteps", true, 0x80u, "");
    Store_CG_Bool("cg_footprints", true, 0x80u, "");
    Int("cg_footprintsDistortWater", 0x0u, 0x0u, 0x1u, 0x80u);
    Int("cg_footprintsDebug", 0x0u, 0x0u, 0x1u, 0x80u);
    Store_CG_Bool("cg_treadmarks", false, 0x80u, "");
    Store_CG_Float("cg_firstPersonTracerChance", F(0x0u), F(0x0u), F(0x3f800000u), 0x80u, "");
    store_cgDvar_cg_airburstLaseTime = Store_CG_Float("cg_airburstLaseTime", F(0x3f000000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_airburstStrafeRestriction = Store_CG_Float("cg_airburstStrafeRestriction", F(0x3d4ccccdu), F(0x0u), F(0x3f800000u), 0x80u, "");
    store_cgDvar_cg_airburstCameraRestriction = Store_CG_Float("cg_airburstCameraRestriction", F(0x3d4ccccdu), F(0x0u), F(0x3f800000u), 0x80u, "");
    store_cgDvar_cg_airburstLaseFlashTime = Store_CG_Float("cg_airburstLaseFlashTime", F(0x3e4ccccdu), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_thirdPersonRange = Store_CG_Float("cg_thirdPersonRange", F(0x42f00000u), F(0x0u), F(0x44800000u), 0x80u, "");
    store_cgDvar_cg_thirdPersonAngle = Store_CG_Float("cg_thirdPersonAngle", F(0x0u), F(0xc3340000u), F(0x43b40000u), 0x80u, "");
    Store_CG_Float("cg_thirdPersonFocusDist", F(0x44000000u), F(0x0u), F(0x44800000u), 0x80u, "");
    store_cgDvar_cg_thirdPerson = Int("cg_thirdPerson", 0x0u, 0x0u, 0x2u, 0x80u);
    store_cgDvar_cg_thirdPersonMode = Store_CG_Enum("cg_thirdPersonMode", Store_CG_EnumTable_cg_thirdPersonMode, 1, 0x80u);
    Store_CG_Bool("cg_use_colored_smoke", false, 0x80u, "");
    store_cgDvar_cg_fakefireWizbyChance = Store_CG_Float("cg_fakefireWizbyChance", F(0x3e4ccccdu), F(0x0u), F(0x3f800000u), 0x80u, "");
    store_cgDvar_cg_useWeaponSwitchReloadCancel = Store_CG_Bool("cg_useWeaponSwitchReloadCancel", true, 0x80u, "");
    store_cgDvar_cg_enableHelicopterNoCullLodOut = Store_CG_Bool("cg_enableHelicopterNoCullLodOut", true, 0x4080u, "");
    store_cgDvar_cl_paused = Int("cl_paused", 0x0u, 0x0u, 0x1u, 0x40u);
    store_cgDvar_cg_drawpaused = Store_CG_Bool("cg_drawpaused", true, 0x0u, "");
    store_cgDvar_g_synchronousClients = Store_CG_Bool("g_synchronousClients", false, 0x100u, "");
    Store_CG_Bool("cg_debug_overlay_viewport", false, 0x80u, "");
    store_cgDvar_cg_dumpAnims = Int("cg_dumpAnims", 0xffffffffu, 0xffffffffu, 0x6ffu, 0x80u);
    Int("developer", 0x0u, 0x0u, 0x2u, 0x0u);
    store_cgDvar_cg_subtitleWidthStandard = Int("cg_subtitleWidthStandard", 0x208u, 0x82u, 0x680u, 0x1u);
    store_cgDvar_cg_subtitleWidthWidescreen = Int("cg_subtitleWidthWidescreen", 0x208u, 0x82u, 0x680u, 0x1u);
    store_cgDvar_cg_constantSizeHeadIcons = Store_CG_Bool("cg_constantSizeHeadIcons", false, 0x80u, "");
    store_cgDvar_cg_headIconMinScreenRadius = Store_CG_Float("cg_headIconMinScreenRadius", F(0x3ca3d70au), F(0x0u), F(0x3f800000u), 0x1u, "");
    store_cgDvar_cg_overheadNamesMaxDist = Store_CG_Float("cg_overheadNamesMaxDist", F(0x461c4000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_overheadNamesNearDist = Store_CG_Float("cg_overheadNamesNearDist", F(0x42800000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_overheadNamesFarDist = Store_CG_Float("cg_overheadNamesFarDist", F(0x44000000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_overheadNamesFarScale = Store_CG_Float("cg_overheadNamesFarScale", F(0x3f333333u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_overheadNamesSize = Store_CG_Float("cg_overheadNamesSize", F(0x3f000000u), F(0x0u), F(0x42c80000u), 0x1u, "");
    store_cgDvar_cg_overheadIconSize = Store_CG_Float("cg_overheadIconSize", F(0x3f333333u), F(0x0u), F(0x42c80000u), 0x1u, "");
    store_cgDvar_cg_overheadRankSize = Store_CG_Float("cg_overheadRankSize", F(0x3f000000u), F(0x0u), F(0x7f7fffffu), 0x1u, "");
    store_cgDvar_cg_overheadNamesGlow = Store_CG_Color("cg_overheadNamesGlow", F(0x0u), F(0x0u), F(0x0u), F(0x3f800000u), 0x80u);
    store_cgDvar_cg_drawFriendlyNames = Store_CG_Bool("cg_drawFriendlyNames", true, 0x80u, "");
    store_cgDvar_cg_enemyNameFadeIn = Int("cg_enemyNameFadeIn", 0xfau, 0x0u, 0x7fffffffu, 0x80u);
    store_cgDvar_cg_friendlyNameFadeIn = Int("cg_friendlyNameFadeIn", 0x0u, 0x0u, 0x7fffffffu, 0x80u);
    store_cgDvar_cg_enemyNameFadeOut = Int("cg_enemyNameFadeOut", 0xfau, 0x0u, 0x7fffffffu, 0x80u);
    store_cgDvar_cg_friendlyNameFadeOut = Int("cg_friendlyNameFadeOut", 0x5dcu, 0x0u, 0x7fffffffu, 0x80u);
    Store_CG_Bool("cg_drawThroughWalls", false, 0x80u, "");
    store_cgDvar_cg_playerHighlightTargetSize = Store_CG_Float("cg_playerHighlightTargetSize", F(0x451c4000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_maxPlayerHighlightTargetSize = Store_CG_Float("cg_maxPlayerHighlightTargetSize", F(0x42b40000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    Store_CG_Color("cg_playerHighlightEnemyColor", F(0x3f800000u), F(0x3dcccccdu), F(0x3dcccccdu), F(0x3f800000u), 0x80u);
    Store_CG_Float("cg_playerHighlightBrightness", F(0x40400000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    Store_CG_Float("cg_playerHighlightMinFade", F(0x3ecccccdu), F(0x0u), F(0x3f800000u), 0x80u, "");
    Int("cg_playerHighlightBlinkTime", 0x4e2u, 0x0u, 0x7fffffffu, 0x80u);
    Store_CG_Float("cg_corpseHighlightFadeTime", F(0x40000000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    Store_CG_Float("cg_cameraSpikeHighlightBrightness", F(0x40a00000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    Store_CG_Color("cg_cameraSpikeEnemyColor", F(0x3f800000u), F(0x0u), F(0x0u), F(0x3f800000u), 0x80u);
    store_cgDvar_cg_adsZScaleMax = Store_CG_Float("cg_adsZScaleMax", F(0x3fa00000u), F(0x3f8147aeu), F(0x3ffeb852u), 0x80u, "");
    Store_CG_Bool("cg_allow_mature", true, 0x40u, "");
    store_cgDvar_ui_email_address = Store_CG_String("ui_email_address", "", 0x1u);
    store_cgDvar_cg_viewZSmoothingMin = Store_CG_Float("cg_viewZSmoothingMin", F(0x3f800000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_viewZSmoothingMax = Store_CG_Float("cg_viewZSmoothingMax", F(0x41800000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_cg_viewZSmoothingTime = Store_CG_Float("cg_viewZSmoothingTime", F(0x3dcccccdu), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_overrideNVGModelWithKnife = Store_CG_Bool("overrideNVGModelWithKnife", false, 0x1000u, "");
    store_cgDvar_cg_visionSetLerpMaxIncreasePerFrame = Store_CG_Float("cg_visionSetLerpMaxIncreasePerFrame", F(0x3ca3d70au), F(0x0u), F(0x3f800000u), 0x80u, "");
    store_cgDvar_cg_visionSetLerpMaxDecreasePerFrame = Store_CG_Float("cg_visionSetLerpMaxDecreasePerFrame", F(0x3c23d70au), F(0x0u), F(0x3f800000u), 0x80u, "");
    Int("cg_flareVisionSetFadeDuration", 0x7d0u, 0x0u, 0x7fffffffu, 0x80u);
    Store_CG_Float("cg_turretBipodOffset", F(0x41880000u), F(0xc2480000u), F(0x42480000u), 0x80u, "");
    Store_CG_Bool("cg_allPlayerNamesVisible", false, 0x80u, "");
    Store_CG_Color("g_ScoresColor_MyTeam", F(0x3e800000u), F(0x3f3851ecu), F(0x3e800000u), F(0x3f800000u), 0x80u);
    Store_CG_Color("g_ScoresColor_EnemyTeam", F(0x3f30a3d7u), F(0x3d8f5c29u), F(0x3d4ccccdu), F(0x3f800000u), 0x80u);
    Store_CG_Color("g_ScoresColor_Spectator", F(0x3e800000u), F(0x3e800000u), F(0x3e800000u), F(0x3f800000u), 0x80u);
    Store_CG_Color("g_ScoresColor_Free", F(0x3f428f5cu), F(0x3f47ae14u), F(0x3dcccccdu), F(0x3f800000u), 0x80u);
    Store_CG_Color("g_ScoresColor_Allies", F(0x3db851ecu), F(0x3eeb851fu), F(0x3d8f5c29u), F(0x3f800000u), 0x80u);
    Store_CG_Color("g_ScoresColor_Axis", F(0x3f30a3d7u), F(0x3d8f5c29u), F(0x3d4ccccdu), F(0x3f800000u), 0x80u);
    Store_CG_String("g_TeamName_Allies", "GAME_ALLIES", 0x80u);
    Store_CG_String("g_TeamName_Axis", "GAME_AXIS", 0x80u);
    store_cgDvar_g_TeamColor_Allies = Store_CG_Color("g_TeamColor_Allies", F(0x3f19999au), F(0x3f23d70au), F(0x3f30a3d7u), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_Axis = Store_CG_Color("g_TeamColor_Axis", F(0x3f266666u), F(0x3f11eb85u), F(0x3ed1eb85u), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_MyTeam = Store_CG_Color("g_TeamColor_MyTeam", F(0x3ecccccdu), F(0x3f333333u), F(0x3ecccccdu), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_EnemyTeam = Store_CG_Color("g_TeamColor_EnemyTeam", F(0x3f800000u), F(0x3ea147aeu), F(0x3eb33333u), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_MyTeamAlt = Store_CG_Color("g_TeamColor_MyTeamAlt", F(0x3ea0a0a1u), F(0x3f6dedeeu), F(0x3f79f9fau), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_MyTeamAlt_Party = Store_CG_Color("g_TeamColor_MyTeamAlt_Party", F(0x3f109091u), F(0x3f1d9d9eu), F(0x3f800000u), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_EnemyTeamAlt = Store_CG_Color("g_TeamColor_EnemyTeamAlt", F(0x3f800000u), F(0x3f000000u), F(0x0u), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_Party = Store_CG_Color("g_TeamColor_Party", F(0x3e8a8a8bu), F(0x3f36b6b7u), F(0x3f56d6d7u), F(0x3f800000u), 0x80u);
    store_cgDvar_g_TeamColor_Spectator = Store_CG_Color("g_TeamColor_Spectator", F(0x3e800000u), F(0x3e800000u), F(0x3e800000u), F(0x3f800000u), 0x80u);
    Store_CG_Color("g_TeamColor_Free", F(0x3f400000u), F(0x3e800000u), F(0x3e800000u), F(0x3f800000u), 0x80u);
    store_cgDvar_cg_proneFeetCollisionHull = Store_CG_Bool("cg_proneFeetCollisionHull", true, 0x80u, "");
    store_cgDvar_cg_altDofTrace = Store_CG_Bool("cg_altDofTrace", true, 0x0u, "");
    store_cgDvar_cameraShakeRemoteMissile_Angles = Store_CG_Vec3("cameraShakeRemoteMissile_Angles", F(0x3d4ccccdu), F(0x3f333333u), F(0x3d8f5c29u), F(0x0u), F(0x42b40000u), 0x80u);
    store_cgDvar_cameraShakeRemoteMissile_Freqs = Store_CG_Vec3("cameraShakeRemoteMissile_Freqs", F(0x42280000u), F(0x3f333333u), F(0x42c80000u), F(0x0u), F(0x7f7fffffu), 0x80u);
    store_cgDvar_cameraShakeRemoteMissile_SpeedRange = Store_CG_Vec2("cameraShakeRemoteMissile_SpeedRange", F(0x447a0000u), F(0x457a0000u), F(0x0u), F(0x7f7fffffu), 0x80u);
    store_cgDvar_cameraShakeRemoteHelo_Angles = Store_CG_Vec3("cameraShakeRemoteHelo_Angles", F(0x3e624dd3u), F(0x3e19999au), F(0x40000000u), F(0x0u), F(0x42b40000u), 0x80u);
    store_cgDvar_cameraShakeRemoteHelo_Freqs = Store_CG_Vec3("cameraShakeRemoteHelo_Freqs", F(0x41f80000u), F(0x42b40000u), F(0x3f000000u), F(0x0u), F(0x7f7fffffu), 0x80u);
    store_cgDvar_cameraShakeRemoteHelo_SpeedRange = Store_CG_Vec2("cameraShakeRemoteHelo_SpeedRange", F(0x44fa0000u), F(0x459c4000u), F(0x0u), F(0x7f7fffffu), 0x80u);
    Store_CG_Float("cg_laserSightMaxDistance", F(0x455ac000u), F(0x0u), F(0x461c4000u), 0x80u, "");
    store_cgDvar_cg_missile_FOV = Store_CG_Float("cg_missile_FOV", F(0x41700000u), F(0x0u), F(0x42820000u), 0x80u, "");
    store_cgDvar_cg_EnableAdaptiveSkinLodScale = Store_CG_Bool("cg_EnableAdaptiveSkinLodScale", true, 0x0u, "");
    store_cgDvar_tu9_ingnoreValidateWeaponOnDeath = Store_CG_Bool("tu9_ingnoreValidateWeaponOnDeath", true, 0x20000u, "");
    Store_CG_Sub_1400fb7d0();
    Store_CG_Sub_14022ad80();
    Store_CG_Sub_1400c8290();
    Store_CG_Sub_1400ba5a0();
    Store_CG_Float("nightVisionFadeInOutTime", F(0x3dcccccdu), F(0x0u), F(0x461c4000u), 0x1000u, "");
    Store_CG_Float("nightVisionPowerOnTime", F(0x3e99999au), F(0x0u), F(0x461c4000u), 0x1000u, "");
    store_cgDvar_nightVisionDisableEffects = Store_CG_Bool("nightVisionDisableEffects", false, 0x1000u, "");
    store_cgDvar_flareDisableEffects = Store_CG_Bool("flareDisableEffects", false, 0x80u, "");
    Int("showVisionSetDebugInfo", 0xffffffffu, 0xffffffffu, 0x3u, 0x80u);
    Store_CG_Sub_1400f7b80();
    Store_CG_Color("cg_ScoresPing_HighColor", F(0x3f4ccccdu), F(0x0u), F(0x0u), F(0x3f800000u), 0x1u);
    Store_CG_Color("cg_ScoresPing_MedColor", F(0x3f4ccccdu), F(0x3f4ccccdu), F(0x0u), F(0x3f800000u), 0x1u);
    Store_CG_Color("cg_ScoresPing_LowColor", F(0x0u), F(0x3f400000u), F(0x0u), F(0x3f800000u), 0x1u);
    Store_CG_Color("cg_ScoresPing_BgColor", F(0x3e800000u), F(0x3e800000u), F(0x3e800000u), F(0x3f000000u), 0x1u);
    Store_CG_Color("cg_scoreboardMyColor", F(0x3f800000u), F(0x3f4ccccdu), F(0x3ecccccdu), F(0x3f800000u), 0x0u);
    Store_CG_Sub_1400f2230();
    Store_CG_Sub_14015df60();
    Store_CG_Sub_140179890();
    store_cgDvar_g_compassShowEnemies = Store_CG_Bool("g_compassShowEnemies", false, 0x80u, "");
    Store_CG_Sub_14061baf0();
    store_cgDvar_cg_timedDamageDuration = Int("cg_timedDamageDuration", 0x1f4u, 0x1u, 0x1388u, 0x80u);
    store_cgDvar_cg_playerFrustumHalfHeight = Store_CG_Float("cg_playerFrustumHalfHeight", F(0x42100000u), F(0x0u), F(0x43000000u), 0x80u, "");
    store_cgDvar_cg_overheadNamesTagUpdateInterval = Int("cg_overheadNamesTagUpdateInterval", 0x1u, 0x1u, 0x1388u, 0x80u);
    store_cgDvar_cg_canSeeFriendlyFrustumUpdateInterval = Int("cg_canSeeFriendlyFrustumUpdateInterval", 0xfau, 0x1u, 0x1388u, 0x80u);
    store_cgDvar_cg_canSeeFriendlyFrustumExpand = Store_CG_Float("cg_canSeeFriendlyFrustumExpand", F(0xc1f00000u), F(0xc47a0000u), F(0x447a0000u), 0x80u, "");
    store_cgDvar_cg_canSeeFriendlyFrustumMinDistance = Store_CG_Float("cg_canSeeFriendlyFrustumMinDistance", F(0x42c80000u), F(0x0u), F(0x461c4000u), 0x80u, "");
    Store_CG_Bool("cg_watersheeting", false, 0x80u, "");
    store_cgDvar_cg_cameraWaterClip = Store_CG_Float("cg_cameraWaterClip", F(0x40900000u), F(0xc2c80000u), F(0x42c80000u), 0x1080u, "");
    store_cgDvar_cg_cameraVehicleExitTweenTime = Store_CG_Float("cg_cameraVehicleExitTweenTime", F(0x3ecccccdu), F(0x0u), F(0x40a00000u), 0x80u, "");
    store_cgDvar_vehicle_piece_damagesfx_threshold = Store_CG_Float("vehicle_piece_damagesfx_threshold", F(0x41200000u), F(0x0u), F(0x7f7fffffu), 0x80u, "");
    store_cgDvar_tu6_cg_destructible_radius_damage_enabled = Store_CG_Bool("tu6_cg_destructible_radius_damage_enabled", false, 0x20000u, "");
    store_cgDvar_tu11_cg_killstreak_target_diamond_when_emped = Store_CG_Bool("tu11_cg_killstreak_target_diamond_when_emped", true, 0x20000u, "");
    store_cgDvar_cg_useSafeSpectatorCam = Store_CG_Bool("cg_useSafeSpectatorCam", true, 0x20000u, "");
    store_cgDvar_cg_fixupSmodelCullDists = Store_CG_Bool("cg_fixupSmodelCullDists", false, 0x0u, "");
    store_cgDvar_cg_alt_mode_killstreak_fix_tu6 = Store_CG_Bool("cg_alt_mode_killstreak_fix_tu6", true, 0x20000u, "");
    store_cgDvar_tu7_cg_deathCamAboveWater = Store_CG_Float("tu7_cg_deathCamAboveWater", F(0x0u), F(0x0u), F(0x43800000u), 0x4000u, "");
    store_cgDvar_tu9_tank_minimap_fix = Store_CG_Bool("tu9_tank_minimap_fix", true, 0x20000u, "");
    store_cgDvar_tu9_projectile_dobj_fix = Store_CG_Bool("tu9_projectile_dobj_fix", true, 0x20000u, "");
    store_cgDvar_tu9_clearedMapPackFlagsFix = Store_CG_Bool("tu9_clearedMapPackFlagsFix", true, 0x20000u, "");
    store_cgDvar_tu12_cg_vehicleCamAboveWater = Store_CG_Float("tu12_cg_vehicleCamAboveWater", F(0x0u), F(0x0u), F(0x43800000u), 0x4000u, "");
    store_cgDvar_tu14_aiAllowForceNoCullCheck = Store_CG_Bool("tu14_aiAllowForceNoCullCheck", false, 0x20000u, "");
    store_cgDvar_cg_drawTalk = Store_CG_Enum("cg_drawTalk", Store_CG_EnumTable_cg_drawTalk, 1, 0x100u);
    store_cgDvar_tu12_zm_force_center_cg_cursorhint = Store_CG_Bool("tu12_zm_force_center_cg_cursorhint", false, 0x20000u, "");
    Store_CG_Sub_140101410(0);
}
#endif
