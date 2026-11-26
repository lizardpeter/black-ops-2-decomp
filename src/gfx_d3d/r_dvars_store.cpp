// Store MP renderer registration: recovered entry prefix 14070E2B5..14070F91E.
// This is NOT the complete 14070E230 function. Replay-only until all paths are recovered.
#define T6_SOURCE "gfx_d3d/r_dvars.cpp"
#if defined(T6_STORE) && defined(T6_STORE_RENDERER_DVAR_REPLAY)
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <xmmintrin.h>

namespace store_renderer_dvar {
using U32 = std::uint32_t;
struct Dvar;
struct alignas(16) Value { U32 words[4]; };
struct alignas(16) Domain { U32 words[4]; };
extern "C" {
extern const Dvar *store_r_aaSamples; // 1427E7158
extern const Dvar *store_r_aaSamplesMax; // 1427E7150
extern const Dvar *store_r_vsync; // 1427E70C8
extern const Dvar *store_r_clearColor; // 1427E6E78
extern const Dvar *store_r_clearColor2; // 1427E7128
extern const Dvar *store_r_viewportBackingColor; // 1427E7198
extern const Dvar *store_r_dlightLimit; // 1427E6C18
extern const Dvar *store_r_dobjLimit; // 1427E6E38
extern const Dvar *store_r_modelLimit; // 1427E6C90
extern const Dvar *store_r_brushLimit; // 1427E6DC0
extern const Dvar *store_r_flashLightFovInnerFraction; // 1427E6D00
extern const Dvar *store_r_flashLightStartRadius; // 1427E6D58
extern const Dvar *store_r_flashLightEndRadius; // 1427E6C88
extern const Dvar *store_r_flashLightBrightness; // 1427E6E08
extern const Dvar *store_r_flashLightOffset; // 1427E7218
extern const Dvar *store_r_flashLightRange; // 1427E6C68
extern const Dvar *store_r_flashLightColor; // 1427E6DA8
extern const Dvar *store_r_flashLightBobAmount; // 1427E71E8
extern const Dvar *store_r_flashLightBobRate; // 1427E71F8
extern const Dvar *store_r_flashLightFlickerAmount; // 1427E71F0
extern const Dvar *store_r_flashLightFlickerRate; // 1427E7210
extern const Dvar *store_r_lockPvs; // 14266D4C8
extern const Dvar *store_r_lockPvsInCode; // 14266D4B0
extern const Dvar *store_r_skipPvs; // 1427E6BE8
extern const Dvar *store_r_singleCell; // 1427E6B88
extern const Dvar *store_r_portalWalkLimit; // 1427E6BD0
extern const Dvar *store_r_portalMinRecurseDepth; // 1427E6BC8
extern const Dvar *store_r_enableOccluders; // 1427E6B58
extern const Dvar *store_r_highLodDist; // 1427E7140
extern const Dvar *store_r_mediumLodDist; // 1427E6F58
extern const Dvar *store_r_lowLodDist; // 1427E6FC8
extern const Dvar *store_r_lowestLodDist; // 1427E6FD8
extern const Dvar *store_r_forceLod; // 1427E7250
extern const Dvar *store_r_modelVertColor; // 1427E71A0
extern const Dvar *store_sm_sunEnable; // 1427E6DD8
extern const Dvar *store_sm_sunQuality; // 1427E6C78
extern const Dvar *store_sm_spotQuality; // 1427E6F80
extern const Dvar *store_sm_spotEnable; // 1427E6DF8
extern const Dvar *store_sm_maxLights; // 1427E7038
extern const Dvar *store_sm_spotShadowFadeTime; // 1427E7088
extern const Dvar *store_sm_sunAlwaysCastsShadow; // 1427E6D38
extern const Dvar *store_sm_polygonOffsetBias; // 1427E6E90
extern const Dvar *store_sm_sunSampleSizeNear; // 1427E6E60
extern const Dvar *store_sm_sunShadowCenter; // 1427E7040
extern const Dvar *store_sm_sunShadowScale; // 1427E6D90
extern const Dvar *store_sm_sunShadowSmall; // 1427E6C98
extern const Dvar *store_sm_sunShadowSmallEnable; // 1427E6DA0
extern const Dvar *store_r_flameScaler; // 1427E6EA8
extern const Dvar *store_r_zombieNameAllowFriendsList; // 1427E7268
extern const Dvar *store_r_zombieNameAllowDevList; // 1427E7270
extern const Dvar *store_sm_strictCull; // 1427E6BF0
extern const Dvar *store_sm_fastSunShadow; // 1427E6F30
extern const Dvar *store_r_noipak; // 14265F2B8
extern const Dvar *store_r_dualPlayEnable; // 1427E6E50
extern const Dvar *store_r_dualPlayActive; // 1427E6D70
extern const Dvar *store_r_blur; // 1427E7060
extern const Dvar *store_r_flame_allowed; // 1427E6FF8
extern const Dvar *store_r_enablePlayerShadow; // 14266D490
extern const Dvar *store_r_enableFlashlight; // 1427E7200
extern const Dvar *store_r_brightness; // 1427E7050
extern const Dvar *store_r_filmUseTweaks; // 1427E6C80
extern const Dvar *store_r_filmTweakEnable; // 1427E6B48
extern const Dvar *store_exp_softclip; // 1427E6FE8
extern const Dvar *store_vc_LIB; // 1427E6B00
extern const Dvar *store_vc_LIG; // 1427E6BA8
extern const Dvar *store_vc_LIW; // 1427E6B28
extern const Dvar *store_vc_LOB; // 1427E6BB8
extern const Dvar *store_vc_LOW; // 1427E6B78
extern const Dvar *store_vc_RGBH; // 1427E6B38
extern const Dvar *store_vc_RGBL; // 1427E6B50
extern const Dvar *store_vc_YH; // 14266D4E8
extern const Dvar *store_vc_YL; // 1427E6BA0
const Dvar *Store_Renderer_Vec4(const char *,float,float,float,float,float,float,U32);
extern const Dvar *store_r_bloomTweaks; // 1427E7238
extern const Dvar *store_r_bloomHiQuality; // 1427E6F40
extern const Dvar *store_r_dofHDR; // 14266D4D8
extern const Dvar *store_r_zombieDarkness; // 1427E6F20
extern const Dvar *store_vc_RS; // 1427E6B40
extern const Dvar *store_vc_RE; // 1427E6B90
extern const Dvar *store_vc_SMR; // 1427E6B10
extern const Dvar *store_vc_SMG; // 1427E6BC0
extern const Dvar *store_vc_SMB; // 1427E6B30
extern const Dvar *store_vc_MMR; // 1427E6B20
extern const Dvar *store_vc_MMG; // 1427E6BE0
extern const Dvar *store_vc_MMB; // 1427E6B70
extern const Dvar *store_vc_HMR; // 14266D4F8
extern const Dvar *store_vc_HMG; // 14266D4E0
extern const Dvar *store_vc_HMB; // 1427E6B68
extern const Dvar *store_vc_FGM; // 1427E6B18
extern const Dvar *store_vc_FSM; // 14266D4F0
extern const Dvar *store_vc_FBM; // 1427E6B60
extern const Dvar *store_vc_LUT; // 1427E6FB0
extern const Dvar *store_r_superFlareDraw; // 1427E7248
extern const Dvar *store_r_waterWaveAngle; // 1427E6E28
extern const Dvar *store_r_waterWaveWavelength; // 1427E6E30
extern const Dvar *store_r_waterWaveAmplitude; // 1427E6C60
extern const Dvar *store_r_waterWavePhase; // 1427E6D80
extern const Dvar *store_r_waterWaveSteepness; // 1427E6C40
extern const Dvar *store_r_waterWaveSpeed; // 1427E6C48
extern const Dvar *store_r_waterWaveBase; // 1427E6E48
extern const Dvar *store_r_waterWaveNormalScale; // 1427E6CD0
extern const Dvar *store_r_waterWaveScriptShader0; // 1427E6D28
extern const Dvar *store_r_waterWaveScriptShader1; // 1427E6C30
extern const Dvar *store_r_dof_enable; // 1427E6C10
extern const Dvar *store_r_dof_tweak; // 1427E6D78
extern const Dvar *store_r_dof_nearBlur; // 1427E6C38
extern const Dvar *store_r_dof_farBlur; // 1427E6DB0
extern const Dvar *store_r_dof_viewModelStart; // 1427E6CE0
extern const Dvar *store_r_dof_viewModelEnd; // 1427E6CC0
extern const Dvar *store_r_dof_nearStart; // 1427E6CA0
extern const Dvar *store_r_dof_nearEnd; // 1427E6C28
extern const Dvar *store_r_dof_farStart; // 1427E6E20
extern const Dvar *store_r_dof_farEnd; // 1427E6C08
extern const Dvar *store_r_dof_bias; // 1427E6E70
extern const Dvar *store_r_dof_showdebug; // 1427E6E00
extern const Dvar *store_r_flameFX_distortionScaleFactor; // 14266D4C0
extern const Dvar *store_r_flameFX_magnitude; // 14266D4A0
extern const Dvar *store_r_flameFX_FPS; // 14266D498
extern const Dvar *store_r_flameFX_fadeDuration; // 14266D4A8
extern const Dvar *store_r_waterSheetingFX_allowed; // 1427E6F60
extern const Dvar *store_r_waterSheetingFX_enable; // 1427E6D40
extern const Dvar *store_r_waterSheetingFX_distortionScaleFactor; // 14266D4B8
extern const Dvar *store_r_waterSheetingFX_magnitude; // 1427E7260
extern const Dvar *store_r_waterSheetingFX_radius; // 14266D4D0
extern const Dvar *store_r_anaglyphFX_enable; // 1427E7020
extern const Dvar *store_r_reviveFX_debug; // 1427E6E10
extern const Dvar *store_r_reviveFX_edgeColorTemp; // 1427E6C70
extern const Dvar *store_r_reviveFX_edgeSaturation; // 1427E6C00
extern const Dvar *store_r_reviveFX_edgeScale; // 1427E6DC8
extern const Dvar *store_r_reviveFX_edgeContrast; // 1427E6DD0
extern const Dvar *store_r_reviveFX_edgeOffset; // 1427E6BF8
extern const Dvar *store_r_reviveFX_edgeAmount; // 1427E6CF0
extern const Dvar *store_r_reviveFX_edgeMaskAdjust; // 1427E6DB8
extern const Dvar *store_r_poisonFX_debug_enable; // 1427E7240
extern const Dvar *store_r_poisonFX_pulse; // 1427E6D20
extern const Dvar *store_r_poisonFX_dvisionX; // 1427E70E0
extern const Dvar *store_r_poisonFX_blurMin; // 1427E6C50
extern const Dvar *store_r_poisonFX_blurMax; // 1427E6DE8
void Store_Renderer_SetFloat(const Dvar *,float,U32); // 14046CEF0
extern const Dvar *store_r_exposureTweak; // 1427E6CD8
extern const Dvar *store_r_exposureValue; // 1427E6D68
extern const Dvar *store_r_primaryLightUseTweaks; // 1427E6B08
extern const Dvar *store_r_primaryLightTweakDiffuseStrength; // 1427E6B80
extern const Dvar *store_r_primaryLightTweakSpecularStrength; // 1427E6B98
extern const Dvar *store_r_lightGridEnableTweaks; // 1427E6F38
extern const Dvar *store_r_lightGridIntensity; // 1427E6F48
extern const Dvar *store_r_lightGridContrast; // 1427E70F0
extern const Dvar *store_r_heroLighting; // 1427E6CB0
extern const Dvar *store_r_heroLightScale; // 1427E6CB8
extern const Dvar *store_r_num_viewports; // 1427E71C0
extern const Dvar *store_r_outdoorAwayBias; // 1427E70A8
extern const Dvar *store_r_outdoorDownBias; // 1427E7180
extern const Dvar *store_r_outdoorFeather; // 1427E6E80
extern const Dvar *store_r_sun_from_dvars; // 1427E70B8
extern const Dvar *store_developer; // 1427E71D8
extern const Dvar *store_sv_cheats; // 1420FA628
extern const Dvar *store_r_defaultMode; // 1427E7120
extern const Dvar *store_r_monitor; // 1427E6FF0
extern const Dvar *store_r_currentMonitor; // 1427E6F08
extern const Dvar *store_r_monitorCount; // 1427E7008
extern const Dvar *store_r_aspectRatio; // 1427E6F90
extern const Dvar *store_r_aspectRatioWindow; // 1427E7100
extern const Dvar *store_r_customMode; // 1427E6E98
extern const Dvar *store_r_superflare_drawlist; // 1427E7010
extern const Dvar *store_r_grassEnable; // 1427E71B8
extern const Dvar *store_r_skyTransition; // 1427E6C58
extern const Dvar *store_r_skyRotation; // 1427E7110
extern const Dvar *store_r_shaderDebugA; // 1427E6D60
extern const Dvar *store_r_shaderDebugB; // 1427E6D50
extern const Dvar *store_r_shaderDebugC; // 1427E6DF0
extern const Dvar *store_r_skyColorTemp; // 1427E6D30
extern const Dvar *store_r_fullPrepass; // 1427E6F78
extern const Dvar *store_r_lightmapOnlyBspSurfs; // 1427E6F50
extern const Dvar *store_r_lightmapOnlySModelSurfs; // 1427E7160
extern const Dvar *store_r_lightmapOnlyEntSurfs; // 1427E6BD8
extern const Dvar *store_r_sortDrawSurfsBsp; // 1427E6E88
extern const Dvar *store_r_sortDrawSurfsStaticModel; // 1427E7068
extern const Dvar *store_r_swrk_override_characterDissolveColor; // 1427E70C0
extern const Dvar *store_r_debugShowPrimaryLights; // 1427E6C20
extern const Dvar *store_r_seethru_decal_enable; // 1427E70D8
extern const Dvar *store_r_tension_enable; // 1427E7130
extern const Dvar *store_r_missile_cam_debug_display; // 1427E6DE0
extern const Dvar *store_r_extracam_custom_aspectratio; // 1427E6E58
extern const Dvar *store_r_extracam_lod_scale; // 1427E7138
extern const Dvar *store_r_shader_constant_set_enable; // 1427E6D08
extern const Dvar *store_r_sky_intensity_showDebugDisplay; // 1427E6D18
extern const Dvar *store_r_sky_intensity_angle0; // 1427E6D48
extern const Dvar *store_r_sky_intensity_angle1; // 1427E6E18
extern const Dvar *store_r_sky_intensity_factor0; // 1427E6CF8
extern const Dvar *store_r_sky_intensity_factor1; // 1427E6CE8
extern const Dvar *store_r_vc_compile; // 1427E7208
void Store_Renderer_SetVariant(const Dvar *,const Value *,U32); // 14046B300
extern unsigned char Store_Renderer_ModeByte; // 1427E7331
const Dvar *Store_Renderer_Color(const char *,float,float,float,float,U32);
const Dvar *Store_Renderer_Vec3(const char *,float,float,float,float,float,U32);
extern const char *const Store_Renderer_DebugShader[]={"none","normal","basisTangent","basisBinormal","basisNormal","lmapDir",nullptr}; // 140D3A1A8
extern const char *const Store_Renderer_DebugPerformance[]={"none","current","average",nullptr}; // 140D3A230
extern const char *const Store_Renderer_LightMapSecondary[]={"Black","Unchanged","Intensity 1","Intensity 2","Intensity 3","Intensity 4","Intensity 5","Intensity 6","Intensity 7","Intensity 8","Intensity 9","Intensity 10","Intensity 11","Intensity 12","Intensity 13","Intensity 14","Intensity 15","Intensity 16","Intensity 17","Intensity 18","Intensity 19","Intensity 20","Intensity 21","Intensity 22","Intensity 23","Intensity 24","Intensity 25","Intensity 26","Intensity 27","Intensity 28","Intensity 29","Intensity 30","Intensity 31",nullptr}; // 140D39F50
extern const char *const Store_Renderer_ColorMap[]={"Black","Unchanged","White","Gray Linear","Gray sRGB",nullptr}; // 140D3A088
extern const char *const Store_Renderer_AlphaMap[]={"Unchanged","255","192","128",nullptr}; // 140D3A0B8
extern const char *const Store_Renderer_SpecularMap[]={"Non metal (sRGB 56)","Unchanged","Metal (sRGB 255)",nullptr}; // 140D3A250
extern const char *const Store_Renderer_GlossMap[]={"0","Unchanged","255","204","153","102","51",nullptr}; // 140D3A270
extern const char *const Store_Renderer_OcclusionMap[]={"0","Unchanged","255","128",nullptr}; // 140D3A208
extern const char *const Store_Renderer_Penetration[]={"off","flash penetrable materials","flash non-penetrable materials",nullptr}; // 140D3A160
extern const char *const Store_Renderer_ReflectionProbe[]={"off","bsp","static models","ents",nullptr}; // 140D3A110
extern const Dvar *store_r_texFilterMipBias; // 1427E7098
extern const Dvar *store_r_fullbright; // 1427E70D0
extern const Dvar *store_r_debugShader; // 1427E6FC0
extern const Dvar *store_r_debugPerformance; // 1427E70F8
extern const Dvar *store_r_loadingScreen; // 1427E6FB8
extern const Dvar *store_r_multiGpu; // 1427E7030
extern const Dvar *store_r_texFilterQuality; // 1427E71A8
extern const Dvar *store_r_lodScaleRigid; // 1427E7258
extern const Dvar *store_r_lodBiasRigid; // 1427E6E40
extern const Dvar *store_r_lodScaleSkinned; // 1427E6D10
extern const Dvar *store_r_lodBiasSkinned; // 1427E6D98
extern const Dvar *store_r_skinnedCacheOverflowLodMaxScale; // 1427E6CA8
extern const Dvar *store_r_znear; // 1427E6F28
extern const Dvar *store_r_zfar; // 1427E6BB0
extern const Dvar *store_r_picmip_manual; // 1427E7108
extern const Dvar *store_r_picmip; // 1427E6E68
extern const Dvar *store_r_picmip_bump; // 1427E7058
extern const Dvar *store_r_picmip_spec; // 1427E6EF0
extern const Dvar *store_r_colorMap; // 1427E7048
extern const Dvar *store_r_normalMap; // 1427E7078
extern const Dvar *store_r_alphaMap; // 1427E6FA8
extern const Dvar *store_r_specularMap; // 1427E7000
extern const Dvar *store_r_glossMap; // 1427E70B0
extern const Dvar *store_r_occlusionMap; // 1427E70E8
extern const Dvar *store_r_warmShaders; // 1427E7228
extern const Dvar *store_r_loadForRenderer; // 1427E7070
extern const Dvar *store_r_shaderWarmingPerFrame; // 1427E71B0
extern const Dvar *store_r_showMissingLightGrid; // 1427E7148
extern const Dvar *store_r_lightTweakAmbient; // 1427E7178
extern const Dvar *store_r_lightTweakSunLight; // 1427E7028
extern const Dvar *store_r_lightTweakAmbientColor; // 1427E7168
extern const Dvar *store_r_lightTweakSunColor; // 1427E70A0
extern const Dvar *store_r_lightTweakSunDiffuseColor; // 1427E6F18
extern const Dvar *store_r_lightTweakSunDirection; // 1427E6CC8
extern const Dvar *store_r_norefresh; // 1427E6D88
extern const Dvar *store_r_scaleViewport; // 1427E7018
extern U32 Store_Renderer_HardwareCount; // 14406FC3C (semantic name provisional)
extern const Dvar *store_r_smp_worker_threads; // 1427E71E0
extern const Dvar *store_r_smp_worker_thread[8]; // 1427E6EB0
void Store_Renderer_Init(); // 140763730
const Dvar *Store_Renderer_Find(const char *);
const Dvar *Store_Renderer_New(const char *,int,U32,const Value *,const Domain *,const char *);
void Store_Renderer_Reregister(const Dvar *,const char *,int,U32,const Value *,const Domain *,const char *);
const Dvar *Store_Renderer_Bool(const char *,bool,U32,const char *);
const Dvar *Store_Renderer_Float(const char *,float,float,float,U32,const char *);
const Dvar *Store_Renderer_Enum(const char *,const char *const *,int,U32);
extern const char *const Store_Renderer_ClipCodec[]={"MJPEG","VP8",nullptr}; // 140D39F30
extern const char *const Store_Renderer_ClipSize[]={"360","480","720","1080",nullptr}; // 140D3A1E0
extern const char *const Store_Renderer_MipMode[]={"Unchanged","Force Trilinear","Force Bilinear","Force Mipmaps Off",nullptr}; // 140D3A138
extern const Dvar *store_r_txaa; // 1427E7188
extern const Dvar *store_r_txaaSupported; // 1427E6EF8
extern const Dvar *store_r_txaaDepth1; // 1427E6F10
extern const Dvar *store_r_txaaDepth2; // 1427E6FE0
extern const Dvar *store_r_txaaMotion1; // 1427E6F68
extern const Dvar *store_r_txaaMotion2; // 1427E6EA0
extern const Dvar *store_vid_xpos; // 1427E7288
extern const Dvar *store_vid_ypos; // 1427E7278
extern const Dvar *store_r_fullscreen; // 1427E7230
extern const Dvar *store_r_previousFullscreen; // 1427E7280
extern const Dvar *store_r_convergence; // 1427E71C8
extern const Dvar *store_r_ignorehwgamma; // 1427E7118
extern const Dvar *store_r_texFilterAnisoMax; // 1427E7170
extern const Dvar *store_r_texFilterAnisoMin; // 1427E7080
extern const Dvar *store_r_texFilterMipMode; // 1427E6F00
extern const Dvar *store_r_fontResolution; // 1427E71D0
extern const Dvar *store_r_resolution; // 1427E6F88
extern const Dvar *store_r_ssao; // 1427E6FA0
extern const Dvar *store_r_ssaoScale; // 1427E6FD0
extern const Dvar *store_r_ssaoRadius; // 1427E6F70
extern const Dvar *store_r_ssaoBias; // 1427E7190
extern const Dvar *store_r_ssaoIntensity; // 1427E7090
}
static float F(U32 bits) { float result; std::memcpy(&result,&bits,4); return result; }
static const Dvar *InlineInt(const char *name,int value,int minimum,int maximum,U32 flags) {
    // Unused stack lanes normalized as in the common replay. Only meaningful
    // integer and domain words are compared against original call boundaries.
    Value v{{U32(value),0,0,0}};
    Domain d{{U32(minimum),U32(maximum),0,0}};
    if (const auto *old=Store_Renderer_Find(name)) {
        Store_Renderer_Reregister(old,name,6,flags,&v,&d,"");
        return old;
    }
    return Store_Renderer_New(name,6,flags,&v,&d,"");
}

static const Dvar *InlineVec(const char *name,int type,U32 flags,const Value &v,const Domain &d) {
    if(const auto *old=Store_Renderer_Find(name)) {
        Store_Renderer_Reregister(old,name,type,flags,&v,&d,"");
        return old;
    }
    return Store_Renderer_New(name,type,flags,&v,&d,"");
}
void RegisterRendererEntryPrefix() {
    Store_Renderer_Init();
    // 14070E2D8..14070E365
    store_r_fontResolution=InlineInt("r_fontResolution",720,720,1080,0x40);
    // 14070E38A..14070E413
    store_r_resolution=InlineInt("r_resolution",720,480,1080,1);
    // 14070E426..14070E452
    store_r_ssao=Store_Renderer_Bool("r_ssao",false,0,"");
    // 14070E462..14070E486
    store_r_ssaoScale=Store_Renderer_Float("r_ssaoScale",F(0x42480000),F(0),F(0x447a0000),0,"");
    // 14070E492..14070E4B6
    store_r_ssaoRadius=Store_Renderer_Float("r_ssaoRadius",F(0x41400000),F(0),F(0x43480000),0,"");
    // 14070E4C2..14070E4EB
    store_r_ssaoBias=Store_Renderer_Float("r_ssaoBias",F(0x3f0ccccd),F(0),F(0x40a00000),0,"");
    // 14070E4F7..14070E4FF
    store_r_ssaoIntensity=Store_Renderer_Float("r_ssaoIntensity",F(0x3f07ae14),F(0),F(0x3f800000),0,"");
    // 14070E512
    Store_Renderer_Bool("r_ssaoDebug",false,0,"");
    // 14070E526..14070E6A3: TXAA.
    store_r_txaa=Store_Renderer_Bool("r_txaa",false,0,"");
    store_r_txaaSupported=Store_Renderer_Bool("r_txaaSupported",false,0,"");
    InlineInt("r_txaaMode",-1,-1,7,0);
    store_r_txaaDepth1=Store_Renderer_Float("r_txaaDepth1",F(0x3ca3d70a),F(0),F(0x3f800000),0,"");
    store_r_txaaDepth2=Store_Renderer_Float("r_txaaDepth2",F(0x3f800000),F(0),F(0x3f800000),0,"");
    store_r_txaaMotion1=Store_Renderer_Float("r_txaaMotion1",F(0x42000000),F(0),F(0x43000000),0,"");
    store_r_txaaMotion2=Store_Renderer_Float("r_txaaMotion2",F(0x3c23d70a),F(0),F(0x43000000),0,"");
    // 14070E6B6..14070E77F: clip settings; no global result assignment.
    Store_Renderer_Enum("r_clipCodec",Store_Renderer_ClipCodec,1,0);
    Store_Renderer_Enum("r_clipSize",Store_Renderer_ClipSize,1,0);
    InlineInt("r_clipFPS",30,10,60,0);
    // 14070E79E..14070EAF7: window placement, fullscreen and stereo convergence.
    store_vid_xpos=InlineInt("vid_xpos",3,-4096,4096,1);
    store_vid_ypos=InlineInt("vid_ypos",22,-4096,4096,1);
    Store_Renderer_Bool("r_autopriority",false,1,"");
    store_r_fullscreen=InlineInt("r_fullscreen",1,0,2,0x21);
    store_r_previousFullscreen=InlineInt("r_previousFullscreen",1,0,2,1);
    store_r_convergence=Store_Renderer_Float("r_convergence",F(0x40c00000),F(0x3dcccccd),F(0x42c80000),0,"");
    store_r_ignorehwgamma=Store_Renderer_Bool("r_ignorehwgamma",false,0x21,"");
    // 14070EB0F..14070ECA5: texture filtering. Mip enum result is stored
    // at 14070ECC9, so the replay prefix stops after that store, before the
    // next float call rather than at the enum's return.
    store_r_texFilterAnisoMax=InlineInt("r_texFilterAnisoMax",16,1,16,1);
    Store_Renderer_Bool("r_texFilterDisable",false,0x80,"");
    store_r_texFilterAnisoMin=InlineInt("r_texFilterAnisoMin",1,1,16,1);
    store_r_texFilterMipMode=Store_Renderer_Enum("r_texFilterMipMode",Store_Renderer_MipMode,0,1);
    // 14070ECD8..14070F91A: texture, LOD, clipping, material overrides, lighting.
    store_r_texFilterMipBias=Store_Renderer_Float("r_texFilterMipBias",F(0),F(0xc1800000),F(0x417fd70a),0x80,"");
    store_r_fullbright=Store_Renderer_Bool("r_fullbright",false,0x80,"");
    store_r_debugShader=Store_Renderer_Enum("r_debugShader",Store_Renderer_DebugShader,0,0x80);
    store_r_debugPerformance=Store_Renderer_Enum("r_debugPerformance",Store_Renderer_DebugPerformance,0,0x80);
    Store_Renderer_Bool("r_lightConflicts",false,0x80,"");
    InlineInt("r_zebrastripe",0,0,3,0x80);
    store_r_loadingScreen=Store_Renderer_Bool("r_loadingScreen",false,0x4000,"");
    store_r_multiGpu=Store_Renderer_Bool("r_multiGpu",true,1,"");
    store_r_texFilterQuality=InlineInt("r_texFilterQuality",1,0,2,0x21);
    store_r_lodScaleRigid=Store_Renderer_Float("r_lodScaleRigid",F(0x3f800000),F(0x3f800000),F(0x40800000),0x80,"");
    store_r_lodBiasRigid=Store_Renderer_Float("r_lodBiasRigid",F(0),F(0xc47a0000),F(0),0x80,"");
    store_r_lodScaleSkinned=Store_Renderer_Float("r_lodScaleSkinned",F(0x3f800000),F(0x3f800000),F(0x40800000),0x80,"");
    store_r_lodBiasSkinned=Store_Renderer_Float("r_lodBiasSkinned",F(0),F(0xc47a0000),F(0),0x80,"");
    store_r_skinnedCacheOverflowLodMaxScale=Store_Renderer_Float("r_skinnedCacheOverflowLodMaxScale",F(0x40800000),F(0x3f800000),F(0x41200000),0,"");
    Store_Renderer_Float("r_skinnedCacheOverflowLodScaleDecay",F(0x40400000),F(0x3c23d70a),F(0x41200000),0,"");
    Store_Renderer_Bool("r_autoLodScale",false,0x80,"");
    store_r_znear=Store_Renderer_Float("r_znear",F(0x40800000),F(0x3f800000),F(0x461c4000),Store_Renderer_ModeByte?0x5080:0x5000,"");
    store_r_zfar=Store_Renderer_Float("r_zfar",F(0),F(0),F(0x7f7fffff),0x80,"");
    Store_Renderer_Bool("r_fog",true,0x80,"");
    store_r_picmip_manual=Store_Renderer_Bool("r_picmip_manual",false,1,"");
    store_r_picmip=InlineInt("r_picmip",0,0,3,1);
    store_r_picmip_bump=InlineInt("r_picmip_bump",0,0,3,1);
    store_r_picmip_spec=InlineInt("r_picmip_spec",0,0,3,1);
    Store_Renderer_Enum("r_lightMapSecondary",Store_Renderer_LightMapSecondary,1,0x80);
    Store_Renderer_Bool("r_lightMapFilterDisable",false,0x80,"");
    store_r_colorMap=Store_Renderer_Enum("r_colorMap",Store_Renderer_ColorMap,1,0x80);
    store_r_normalMap=Store_Renderer_Float("r_normalMap",F(0),F(0),F(0x3f800000),0x80,"");
    Store_Renderer_Bool("r_drawLitDrawlists",true,0x80,"");
    Store_Renderer_Bool("r_drawLightmapDrawlists",true,0x80,"");
    store_r_alphaMap=Store_Renderer_Enum("r_alphaMap",Store_Renderer_AlphaMap,0,0x80);
    store_r_specularMap=Store_Renderer_Enum("r_specularMap",Store_Renderer_SpecularMap,1,0x80);
    store_r_glossMap=Store_Renderer_Enum("r_glossMap",Store_Renderer_GlossMap,1,0x80);
    store_r_occlusionMap=Store_Renderer_Enum("r_occlusionMap",Store_Renderer_OcclusionMap,1,0x80);
    store_r_warmShaders=Store_Renderer_Bool("r_warmShaders",false,0,"");
    store_r_loadForRenderer=Store_Renderer_Bool("r_loadForRenderer",true,0x20,"");
    store_r_shaderWarmingPerFrame=InlineInt("r_shaderWarmingPerFrame",5,1,50,0);
    Store_Renderer_Bool("r_resampleScene",true,0x80,"");
    Store_Renderer_Enum("r_showPenetration",Store_Renderer_Penetration,0,0x80);
    Store_Renderer_Bool("r_showLightGrid",false,0x80,"");
    Store_Renderer_Bool("r_showLightingOrigins",false,0x80,"");
    store_r_showMissingLightGrid=Store_Renderer_Bool("r_showMissingLightGrid",false,0x80,"");
    Store_Renderer_Enum("r_showReflectionProbeSelection",Store_Renderer_ReflectionProbe,0,0x80);
    store_r_lightTweakAmbient=Store_Renderer_Float("r_lightTweakAmbient",F(0x3dcccccd),F(0),F(0x40800000),0x240,"");
    store_r_lightTweakSunLight=Store_Renderer_Float("r_lightTweakSunLight",F(0x3f800000),F(0),F(0x42000000),0x1200,"");
    store_r_lightTweakAmbientColor=Store_Renderer_Color("r_lightTweakAmbientColor",F(0x3f800000),F(0),F(0),F(0x3f800000),0x240);
    store_r_lightTweakSunColor=InlineVec("r_lightTweakSunColor",11,0x1200,Value{{0x3f800000,0x3f800000,0,0}},Domain{{0,0x3f800000,0,0}});
    store_r_lightTweakSunDiffuseColor=Store_Renderer_Color("r_lightTweakSunDiffuseColor",F(0),F(0),F(0x3f800000),F(0x3f800000),0x240);
    store_r_lightTweakSunDirection=Store_Renderer_Vec3("r_lightTweakSunDirection",F(0),F(0),F(0),F(0xc3b40000),F(0x43b40000),0x1200);
    Store_Renderer_Bool("r_showSunDirectionDebug",false,0x80,"");
    Store_Renderer_Bool("r_showOutdoorVolumeDebug",false,0x80,"");
    InlineVec("r_materialXYZ",12,0,Value{{0x40770a3d,0x407eb852,0x4091999a,0}},Domain{{0,0x42c80000,0,0}});
    store_r_norefresh=Store_Renderer_Bool("r_norefresh",false,0x80,"");
    store_r_scaleViewport=Store_Renderer_Float("r_scaleViewport",F(0x3f800000),F(0),F(0x3f800000),0x80,"");
    Store_Renderer_Bool("r_smp_worker",true,0,"");

}

// Independent recovered fragment 14070F91F..14070FACD. The hardware
// value is unsigned (JA), but the loop bound uses signed JGE.
static int HardwareWorkers(U32 hardware) {
    if (hardware<=2) return 1;
    if (hardware<=4) return 2;
    if (hardware<=10) return int(hardware)-2;
    return 8;
}
template<class T> static T Read(const Dvar *d,unsigned offset) {
    T v; std::memcpy(&v,reinterpret_cast<const unsigned char *>(d)+offset,sizeof(T)); return v;
}
static int WorkerCount(const Dvar *d) {
    if (!d) return 0;
    const int type=Read<int>(d,0x18);
    if (type==6 || type==7) return Read<int>(d,0x20);
    if (type==1) return Read<unsigned char>(d,0x20)!=0;
    if (type==2) return _mm_cvttss_si32(_mm_set_ss(Read<float>(d,0x20)));
    return std::atoi(Read<const char *>(d,0x20));
}
void RegisterRendererWorkerFragment() {
    static const char *const names[]={
        "r_smp_worker_thread0","r_smp_worker_thread1","r_smp_worker_thread2","r_smp_worker_thread3",
        "r_smp_worker_thread4","r_smp_worker_thread5","r_smp_worker_thread6","r_smp_worker_thread7"};
    store_r_smp_worker_threads=InlineInt("r_smp_worker_threads",
                                          HardwareWorkers(Store_Renderer_HardwareCount),2,8,0);
    // Original reloads both the dvar pointer and hardware value on each
    // iteration. Do not silently clamp or replace its current-value bound.
    // Isolated replay requires valid original registry state (count <= 8).
    for (int i=0;i<WorkerCount(store_r_smp_worker_threads);++i) {
        const bool enabled=i<HardwareWorkers(Store_Renderer_HardwareCount);
        store_r_smp_worker_thread[i]=Store_Renderer_Bool(names[i],enabled,enabled?0x4000:0x40,"");
    }
}

static const Dvar *InlineBool(const char *name,bool value,U32 flags) {
    return InlineVec(name,1,flags,Value{{value?1u:0u,0,0,0}},Domain{});
}

// 14070FACE..140710BFF. The two original flashlight paths differ in
// flags; their ordered registrations and result slots are identical.
void RegisterRendererPostWorkerFragment() {
    store_r_aaSamples=InlineInt("r_aaSamples",1,1,16,0x21);
    store_r_aaSamplesMax=InlineInt("r_aaSamplesMax",8,1,16,0);
    store_r_vsync=InlineBool("r_vsync",true,1);
    InlineInt("r_vsyncThreshold",10,0,100,0);
    store_r_clearColor=Store_Renderer_Color("r_clearColor",F(0),F(0),F(0),F(0),0);
    store_r_clearColor2=Store_Renderer_Color("r_clearColor2",F(0),F(0),F(0),F(0),0);
    store_r_viewportBackingColor=Store_Renderer_Color("r_viewportBackingColor",F(0x3da3d70a),F(0x3da3d70a),F(0x3da3d70a),F(0x3f800000),0);
    store_r_dlightLimit=InlineInt("r_dlightLimit",4,0,4,0);
    store_r_dobjLimit=InlineInt("r_dobjLimit",512,0,1024,0x80);
    store_r_modelLimit=InlineInt("r_modelLimit",1024,0,1024,0x80);
    store_r_brushLimit=InlineInt("r_brushLimit",512,0,1024,0x80);
    InlineBool("r_spotLightShadows",true,0x80);
    InlineBool("r_spotLightSModelShadows",true,0x80);
    InlineBool("r_spotLightEntityShadows",true,0x80);
    const U32 flashFlags=Store_Renderer_ModeByte?0x5000:0x1000;
    store_r_flashLightFovInnerFraction=Store_Renderer_Float("r_flashLightFovInnerFraction",F(0x3f333333),F(0),F(0x3f7d70a4),flashFlags,"");
    store_r_flashLightStartRadius=Store_Renderer_Float("r_flashLightStartRadius",F(0),F(0),F(0x44960000),flashFlags,"");
    store_r_flashLightEndRadius=Store_Renderer_Float("r_flashLightEndRadius",F(0x43960000),F(0x3f800000),F(0x44960000),flashFlags,"");
    InlineBool("r_flashLightShadows",true,flashFlags);
    store_r_flashLightBrightness=Store_Renderer_Float("r_flashLightBrightness",F(0x41200000),F(0),F(0x41c80000),flashFlags,"");
    store_r_flashLightOffset=Store_Renderer_Vec3("r_flashLightOffset",F(0),F(0x40000000),F(0x3f800000),F(0xc2c80000),F(0x42c80000),flashFlags);
    store_r_flashLightRange=Store_Renderer_Float("r_flashLightRange",F(0x43af0000),F(0x3f800000),F(0x44960000),flashFlags,"");
    store_r_flashLightColor=Store_Renderer_Vec3("r_flashLightColor",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0),F(0x3f800000),flashFlags);
    store_r_flashLightBobAmount=Store_Renderer_Vec3("r_flashLightBobAmount",F(0),F(0),F(0),F(0),F(0x42480000),flashFlags);
    store_r_flashLightBobRate=Store_Renderer_Vec3("r_flashLightBobRate",F(0),F(0x3e4ccccd),F(0x3e99999a),F(0),F(0x40000000),flashFlags);
    Store_Renderer_Float("r_flashLightSpecularScale",F(0x3f800000),F(0),F(0x41200000),flashFlags,"");
    store_r_flashLightFlickerAmount=Store_Renderer_Float("r_flashLightFlickerAmount",F(0),F(0),F(0x3f800000),flashFlags,"");
    store_r_flashLightFlickerRate=Store_Renderer_Float("r_flashLightFlickerRate",F(0x42820000),F(0x3dcccccd),F(0x42820000),flashFlags,"");
    InlineBool("r_drawWater",true,0x80);
    store_r_lockPvs=InlineBool("r_lockPvs",false,0x80);
    store_r_lockPvsInCode=InlineBool("r_lockPvsInCode",false,0x80);
    store_r_skipPvs=InlineBool("r_skipPvs",false,0x80);
    store_r_singleCell=InlineBool("r_singleCell",false,0x80);
}

// Contiguous recovered body so far, not the original function's final return.
void RegisterRendererRecoveredPrefix() {
    RegisterRendererEntryPrefix();
    RegisterRendererWorkerFragment();
    RegisterRendererPostWorkerFragment();
}

extern const char *const Store_Renderer_ForceLod[]={"high","medium","low","lowest","none",nullptr};
static const Dvar *InlineForceLod(int value) {
    Domain domain{};
    const char *const *table=Store_Renderer_ForceLod;
    U32 count=0;
    while(table[count]) ++count;
    domain.words[0]=count;
    std::memcpy(&domain.words[2],&table,sizeof(table));
    return InlineVec("r_forceLod",7,0x80,Value{{U32(value),0,0,0}},domain);
}

// 140710C00..1407126CF: portal visibility, shadow maps, stereo and camera.
// r_forceLod uses an inlined enum registration, not the enum wrapper.
void RegisterRendererVisibilityShadowFragment() {
    InlineBool("r_showForcedInvisibleCells",false,0x80);
    store_r_portalWalkLimit=InlineInt("r_portalWalkLimit",0,0,100,0x80);
    store_r_portalMinRecurseDepth=InlineInt("r_portalMinRecurseDepth",2,0,100,0x80);
    store_r_enableOccluders=InlineBool("r_enableOccluders",true,0x80);
    store_r_highLodDist=Store_Renderer_Float("r_highLodDist",F(0xbf800000),F(0xbf800000),F(0x7f7fffff),0x80,"");
    store_r_mediumLodDist=Store_Renderer_Float("r_mediumLodDist",F(0xbf800000),F(0xbf800000),F(0x7f7fffff),0x80,"");
    store_r_lowLodDist=Store_Renderer_Float("r_lowLodDist",F(0xbf800000),F(0xbf800000),F(0x7f7fffff),0x80,"");
    store_r_lowestLodDist=Store_Renderer_Float("r_lowestLodDist",F(0xbf800000),F(0xbf800000),F(0x7f7fffff),0x80,"");
    store_r_forceLod=InlineForceLod(4);
    store_r_modelVertColor=InlineBool("r_modelVertColor",true,0xa0);
    store_sm_sunEnable=InlineBool("sm_sunEnable",true,0x1000);
    store_sm_sunQuality=InlineInt("sm_sunQuality",0,0,2,1);
    store_sm_spotQuality=InlineInt("sm_spotQuality",0,0,2,1);
    store_sm_spotEnable=InlineBool("sm_spotEnable",true,0x1000);
    store_sm_maxLights=InlineInt("sm_maxLights",4,0,4,1);
    store_sm_spotShadowFadeTime=Store_Renderer_Float("sm_spotShadowFadeTime",F(0x3f800000),F(0x3c23d70a),F(0x40a00000),0,"");
    store_sm_sunAlwaysCastsShadow=InlineBool("sm_sunAlwaysCastsShadow",false,0x1080);
    store_sm_polygonOffsetBias=InlineInt("sm_polygonOffsetBias",8192,0,65536,0x10a0);
    store_sm_sunSampleSizeNear=Store_Renderer_Float("sm_sunSampleSizeNear",F(0x3f000000),F(0x3d800000),F(0x40800000),0x5000,"");
    store_sm_sunShadowCenter=Store_Renderer_Vec3("sm_sunShadowCenter",F(0),F(0),F(0),F(0xff7fffff),F(0x7f7fffff),0x1080);
    store_sm_sunShadowScale=Store_Renderer_Float("sm_sunShadowScale",F(0x3f800000),F(0x3e800000),F(0x3f800000),0x1080,"");
    store_sm_sunShadowSmall=InlineBool("sm_sunShadowSmall",false,0x1000);
    store_sm_sunShadowSmallEnable=InlineBool("sm_sunShadowSmallEnable",false,0x5000);
    store_r_flameScaler=Store_Renderer_Float("r_flameScaler",F(0x3f800000),F(0),F(0x3f800000),0x1000,"");
    InlineBool("r_zombieDisableSlideEffect",false,0x5000);
    InlineBool("r_zombieDisableEarthEffect",false,0x5000);
    store_r_zombieNameAllowFriendsList=InlineBool("r_zombieNameAllowFriendsList",true,0x5000);
    store_r_zombieNameAllowDevList=InlineBool("r_zombieNameAllowDevList",true,0x5000);
    Store_Renderer_Float("sm_spotShadowLargeRadiusScale",F(0x3f800000),F(0),F(0x40a00000),0x1080,"");
    store_sm_strictCull=InlineBool("sm_strictCull",false,0x80);
    store_sm_fastSunShadow=InlineBool("sm_fastSunShadow",true,0x80);
    store_r_noipak=InlineBool("r_noipak",false,0x10);
    InlineBool("r_stereo3DAvailable",false,1);
    InlineBool("r_stereo3DOn",false,0x201);
    InlineInt("r_stereo3DMode",0,0,1,1);
    Store_Renderer_Float("r_stereo3DEyeSeparation",F(0x3e99999a),F(0),F(0x42c80000),0x1000,"");
    Store_Renderer_Float("r_stereo3DEyeSeparationScaler",F(0x3f800000),F(0),F(0x3f800000),0x1000,"");
    Store_Renderer_Float("r_stereoFocusDepth",F(0x41a00000),F(0x41a00000),F(0x428c0000),1,"");
    Store_Renderer_Float("r_stereoGunShift",F(0x3f19999a),F(0),F(0x3f6b851f),1,"");
    Store_Renderer_Float("r_stereoTurretShift",F(0),F(0),F(0x461c4000),0x1000,"");
    Store_Renderer_Float("r_stereo3DIPD",F(0x3d94c947),F(0),F(0x3f800000),0x1000,"");
    store_r_dualPlayEnable=InlineBool("r_dualPlayEnable",false,1);
    store_r_dualPlayActive=InlineBool("r_dualPlayActive",false,0x1000);
    InlineBool("r_dualPlayTestCount",true,0x20000);
    store_r_blur=Store_Renderer_Float("r_blur",F(0),F(0),F(0x42000000),0x80,"");
    store_r_flame_allowed=InlineBool("r_flame_allowed",true,1);
    Store_Renderer_Float("r_filmLut",F(0xbf800000),F(0xbf800000),F(0x41100000),0,"");
    InlineInt("r_filmTweakLut",-1,-1,9,0x1000);
    InlineInt("r_forceCameraNode",-1,-1,3000,0x80);
    InlineInt("r_forceCameraHdg",0,0,3,0x80);
    InlineBool("r_forceCamerarecord",false,0x80);
}
// 1407126D0..140712E97: film, exposure and color-vector controls.
void RegisterRendererFilmColorFragment() {
    store_r_enablePlayerShadow=InlineBool("r_enablePlayerShadow",false,Store_Renderer_ModeByte?0x5080:0x5000);
    store_r_enableFlashlight=InlineInt("r_enableFlashlight",0,0,2,Store_Renderer_ModeByte?0x5000:0x1000);
    InlineBool("r_waterFogTest",false,0x80);
    Store_Renderer_Float("r_contrast",F(0x3f800000),F(0),F(0x40800000),0x80,"");
    store_r_brightness=Store_Renderer_Float("r_brightness",F(0),F(0x3f000000),F(0x3fc00000),0x80,"");
    Store_Renderer_Float("r_desaturation",F(0x3f800000),F(0),F(0x40800000),0x80,"");
    Store_Renderer_Float("r_filmTweakBrightness",F(0),F(0xbf800000),F(0x3f800000),0x1000,"");
    Store_Renderer_Float("r_filmTweakDesaturation",F(0x3e4ccccd),F(0),F(0x3f800000),0x1000,"");
    InlineBool("r_filmTweakInvert",false,0x1000);
    store_r_filmUseTweaks=InlineBool("r_filmUseTweaks",false,0x80);
    store_r_filmTweakEnable=InlineBool("r_filmTweakEnable",false,0x1000);
    store_exp_softclip=Store_Renderer_Float("exp_softclip",F(0x3e800000),F(0),F(0x3f800000),0x5000,"");
    store_vc_LIB=Store_Renderer_Vec4("vc_LIB",F(0),F(0),F(0),F(0),F(0),F(0x42000000),0x5000);
    store_vc_LIG=Store_Renderer_Vec4("vc_LIG",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x3c23d70a),F(0x41200000),0x5000);
    store_vc_LIW=Store_Renderer_Vec4("vc_LIW",F(0x42000000),F(0x42000000),F(0x42000000),F(0x42000000),F(0),F(0x42000000),0x5000);
    store_vc_LOB=Store_Renderer_Vec4("vc_LOB",F(0),F(0),F(0),F(0),F(0),F(0x42000000),0x5000);
    store_vc_LOW=Store_Renderer_Vec4("vc_LOW",F(0x42000000),F(0x42000000),F(0x42000000),F(0x42000000),F(0),F(0x42000000),0x5000);
    store_vc_RGBH=Store_Renderer_Vec4("vc_RGBH",F(0),F(0),F(0),F(0),F(0),F(0x42000000),0x5000);
    store_vc_RGBL=Store_Renderer_Vec4("vc_RGBL",F(0),F(0),F(0),F(0),F(0),F(0x42000000),0x5000);
    store_vc_YH=Store_Renderer_Vec4("vc_YH",F(0),F(0),F(0),F(0),F(0),F(0x42000000),0x5000);
    store_vc_YL=Store_Renderer_Vec4("vc_YL",F(0),F(0),F(0),F(0),F(0),F(0x42000000),0x5000);
}
// 140712E98..14071376E: bloom, DOF HDR, color matrices, flare controls.
void RegisterRendererBloomColorMatrixFragment() {
    store_r_bloomTweaks=InlineBool("r_bloomTweaks",false,0x5000);
    store_r_bloomHiQuality=InlineBool("r_bloomHiQuality",false,0x4001);
    store_r_dofHDR=InlineInt("r_dofHDR",0,0,2,0x4001);
    store_r_zombieDarkness=InlineBool("r_zombieDarkness",false,0x5000);
    store_vc_RS=Store_Renderer_Vec4("vc_RS",F(0),F(0x3f000000),F(0),F(0x3f400000),F(0),F(0x3f800000),0x5000);
    store_vc_RE=Store_Renderer_Vec4("vc_RE",F(0x3f000000),F(0x3f800000),F(0x3e800000),F(0x3f800000),F(0),F(0x3f800000),0x5000);
    store_vc_SMR=Store_Renderer_Vec4("vc_SMR",F(0x3f800000),F(0),F(0),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_SMG=Store_Renderer_Vec4("vc_SMG",F(0),F(0x3f800000),F(0),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_SMB=Store_Renderer_Vec4("vc_SMB",F(0),F(0),F(0x3f800000),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_MMR=Store_Renderer_Vec4("vc_MMR",F(0x3f800000),F(0),F(0),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_MMG=Store_Renderer_Vec4("vc_MMG",F(0),F(0x3f800000),F(0),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_MMB=Store_Renderer_Vec4("vc_MMB",F(0),F(0),F(0x3f800000),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_HMR=Store_Renderer_Vec4("vc_HMR",F(0x3f800000),F(0),F(0),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_HMG=Store_Renderer_Vec4("vc_HMG",F(0),F(0x3f800000),F(0),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_HMB=Store_Renderer_Vec4("vc_HMB",F(0),F(0),F(0x3f800000),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_FGM=Store_Renderer_Vec4("vc_FGM",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_FSM=Store_Renderer_Vec4("vc_FSM",F(0),F(0),F(0),F(0x3f800000),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_FBM=Store_Renderer_Vec4("vc_FBM",F(0),F(0),F(0),F(0),F(0xc2c80000),F(0x42c80000),0x5000);
    store_vc_LUT=InlineInt("vc_LUT",0,-32,32,0x5000);
    InlineInt("vc_SNAP",0,0,32,0x5000);
    Store_Renderer_Vec4("r_sunFlareTint",F(0),F(0),F(0x3f800000),F(0x3f800000),F(0),F(0x3f800000),0x1000);
    store_r_superFlareDraw=InlineBool("r_superFlareDraw",true,0x5000);
}
// 14071376F..140713DA0: water surface parameters and depth of field.
void RegisterRendererWaterDofFragment() {
    store_r_waterWaveAngle=Store_Renderer_Vec4("r_waterWaveAngle",F(0),F(0),F(0),F(0),F(0),F(0x43b40000),0x5000);
    store_r_waterWaveWavelength=Store_Renderer_Vec4("r_waterWaveWavelength",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x44800000),0x5000);
    store_r_waterWaveAmplitude=Store_Renderer_Vec4("r_waterWaveAmplitude",F(0),F(0),F(0),F(0),F(0),F(0x43800000),0x5000);
    store_r_waterWavePhase=Store_Renderer_Vec4("r_waterWavePhase",F(0),F(0),F(0),F(0),F(0),F(0x40c90fda),0x5000);
    store_r_waterWaveSteepness=Store_Renderer_Vec4("r_waterWaveSteepness",F(0),F(0),F(0),F(0),F(0),F(0x3f800000),0x5000);
    store_r_waterWaveSpeed=Store_Renderer_Vec4("r_waterWaveSpeed",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0),F(0x40000000),0x5000);
    store_r_waterWaveBase=Store_Renderer_Float("r_waterWaveBase",F(0),F(0xc2400000),F(0x42400000),0x5000,"");
    store_r_waterWaveNormalScale=Store_Renderer_Float("r_waterWaveNormalScale",F(0x3f800000),F(0x3e800000),F(0x40800000),0x5000,"");
    store_r_waterWaveScriptShader0=Store_Renderer_Vec4("r_waterWaveScriptShader0",F(0),F(0),F(0),F(0),F(0xff7fffff),F(0x7f7fffff),0x5000);
    store_r_waterWaveScriptShader1=Store_Renderer_Vec4("r_waterWaveScriptShader1",F(0),F(0),F(0),F(0),F(0xff7fffff),F(0x7f7fffff),0x5000);
    store_r_dof_enable=InlineBool("r_dof_enable",true,0);
    store_r_dof_tweak=InlineBool("r_dof_tweak",false,0x4080);
    store_r_dof_nearBlur=Store_Renderer_Float("r_dof_nearBlur",F(0x40c00000),F(0x40800000),F(0x41200000),0x4080,"");
    store_r_dof_farBlur=Store_Renderer_Float("r_dof_farBlur",F(0x3fe66666),F(0),F(0x41200000),0x4080,"");
    store_r_dof_viewModelStart=Store_Renderer_Float("r_dof_viewModelStart",F(0x40000000),F(0),F(0x43000000),0x4080,"");
    store_r_dof_viewModelEnd=Store_Renderer_Float("r_dof_viewModelEnd",F(0x41000000),F(0),F(0x43000000),0x4080,"");
    store_r_dof_nearStart=Store_Renderer_Float("r_dof_nearStart",F(0x41200000),F(0),F(0x447a0000),0x4080,"");
    store_r_dof_nearEnd=Store_Renderer_Float("r_dof_nearEnd",F(0x42700000),F(0),F(0x447a0000),0x4080,"");
    store_r_dof_farStart=Store_Renderer_Float("r_dof_farStart",F(0x447a0000),F(0),F(0x469c4000),0x4080,"");
    store_r_dof_farEnd=Store_Renderer_Float("r_dof_farEnd",F(0x45dac000),F(0),F(0x469c4000),0x4080,"");
    store_r_dof_bias=Store_Renderer_Float("r_dof_bias",F(0x3f000000),F(0x3dcccccd),F(0x40400000),0x4080,"");
    store_r_dof_showdebug=InlineBool("r_dof_showdebug",false,0x80);
}
// 140713DA1..14071478A. Includes the original conditional post-registration
// writes, not just the defaults. These setters target pulse, dvisionX and
// blurMin (NOT blurMax); the flag byte is read again after registration.
void RegisterRendererScreenEffectsFragment() {
    store_r_flameFX_distortionScaleFactor=Store_Renderer_Vec4("r_flameFX_distortionScaleFactor",F(0),F(0x3f800000),F(0x3f800000),F(0x3f030d0f),F(0),F(0x3f800000),0x1081);
    store_r_flameFX_magnitude=Store_Renderer_Float("r_flameFX_magnitude",F(0x3cb03f99),F(0),F(0x3f800000),0x1081,"");
    store_r_flameFX_FPS=InlineInt("r_flameFX_FPS",15,1,60,0x1081);
    store_r_flameFX_fadeDuration=Store_Renderer_Float("r_flameFX_fadeDuration",F(0x3f000000),F(0),F(0x41700000),0x1001,"");
    store_r_waterSheetingFX_allowed=InlineBool("r_waterSheetingFX_allowed",true,1);
    store_r_waterSheetingFX_enable=InlineBool("r_waterSheetingFX_enable",false,0);
    store_r_waterSheetingFX_distortionScaleFactor=Store_Renderer_Vec4("r_waterSheetingFX_distortionScaleFactor",F(0x3cb3e78e),F(0x3f800000),F(0),F(0),F(0),F(0x3f800000),0);
    store_r_waterSheetingFX_magnitude=Store_Renderer_Float("r_waterSheetingFX_magnitude",F(0x3d863935),F(0),F(0x3f800000),0,"");
    store_r_waterSheetingFX_radius=Store_Renderer_Float("r_waterSheetingFX_radius",F(0x408e18a8),F(0),F(0x42000000),0,"");
    store_r_anaglyphFX_enable=InlineBool("r_anaglyphFX_enable",false,1);
    InlineBool("r_rimIntensity_debug",false,0x5000);
    Store_Renderer_Float("r_rimIntensity",F(0),F(0),F(0x42480000),0x5000,"");
    InlineBool("r_genericSceneVector_debug",false,0x5000);
    Store_Renderer_Vec4("r_genericSceneVector0",F(0),F(0),F(0),F(0),F(0),F(0x42c80000),0x5000);
    Store_Renderer_Vec4("r_genericSceneVector1",F(0),F(0),F(0),F(0),F(0),F(0x42c80000),0x5000);
    Store_Renderer_Vec4("r_genericSceneVector2",F(0),F(0),F(0),F(0),F(0),F(0x42c80000),0x5000);
    Store_Renderer_Vec4("r_genericSceneVector3",F(0),F(0),F(0),F(0),F(0),F(0x42c80000),0x5000);
    store_r_reviveFX_debug=InlineBool("r_reviveFX_debug",false,0x80);
    store_r_reviveFX_edgeColorTemp=Store_Renderer_Float("r_reviveFX_edgeColorTemp",F(0x45cb2000),F(0x44ce4000),F(0x46c35000),0x5000,"");
    store_r_reviveFX_edgeSaturation=Store_Renderer_Float("r_reviveFX_edgeSaturation",F(0x3f800000),F(0),F(0x40000000),0x5000,"");
    store_r_reviveFX_edgeScale=Store_Renderer_Vec3("r_reviveFX_edgeScale",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0),F(0x40000000),0x5000);
    store_r_reviveFX_edgeContrast=Store_Renderer_Vec3("r_reviveFX_edgeContrast",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0),F(0x40000000),0x5000);
    store_r_reviveFX_edgeOffset=Store_Renderer_Vec3("r_reviveFX_edgeOffset",F(0),F(0),F(0),F(0xc0000000),F(0x40000000),0x5000);
    store_r_reviveFX_edgeAmount=Store_Renderer_Float("r_reviveFX_edgeAmount",F(0),F(0),F(0x3f800000),0x5000,"");
    store_r_reviveFX_edgeMaskAdjust=Store_Renderer_Float("r_reviveFX_edgeMaskAdjust",F(0),F(0xbf800000),F(0x3f800000),0x5000,"");
    store_r_poisonFX_debug_enable=InlineBool("r_poisonFX_debug_enable",false,0x5000);
    store_r_poisonFX_pulse=Store_Renderer_Float("r_poisonFX_pulse",F(0x3f8ccccd),F(0x3f800000),F(0x40000000),0x4000,"");
    store_r_poisonFX_dvisionX=Store_Renderer_Float("r_poisonFX_dvisionX",F(0),F(0xc1a00000),F(0x41a00000),0x4000,"");
    store_r_poisonFX_blurMin=Store_Renderer_Float("r_poisonFX_blurMin",F(0),F(0),F(0x41a00000),0x5000,"");
    store_r_poisonFX_blurMax=Store_Renderer_Float("r_poisonFX_blurMax",F(0x40800000),F(0),F(0x41a00000),Store_Renderer_ModeByte?0x5080:0x5000,"");
    if(Store_Renderer_ModeByte) {
        Store_Renderer_SetFloat(store_r_poisonFX_pulse,F(0x40000000),0);
        Store_Renderer_SetFloat(store_r_poisonFX_dvisionX,F(0x41a00000),0);
        Store_Renderer_SetFloat(store_r_poisonFX_blurMin,F(0x3fc00000),0);
    }
}
extern const char *const Store_Renderer_AspectRatio[]={"auto","standard","wide 16:10","wide 16:9",nullptr};
extern const char *const Store_Renderer_Superflare[]={"off(with fx)","pre viewmodel","post viewmodel","post blur",nullptr};
static const Dvar *InlineRendererString(const char *name,const char *str,U32 flags) {
    Value v{}; std::memcpy(v.words,&str,sizeof(str));
    return InlineVec(name,8,flags,v,Domain{});
}
static const Dvar *InlineRendererEnum(const char *name,int value,const char *const *table,U32 flags) {
    Domain d{}; while(table[d.words[0]]) ++d.words[0];
    std::memcpy(&d.words[2],&table,sizeof(table));
    return InlineVec(name,7,flags,Value{{U32(value),0,0,0}},d);
}
// 14071478B..14071757E: final registration sequence before the genuine
// epilogue. Keep the see-through decal setter and its type/name checks.
void RegisterRendererFinalFragment() {
    InlineBool("r_zbufferDebug",false,0x80);
    Store_Renderer_Float("r_zbufferDebugRange",F(0x461c4000),F(0x3f800000),F(0x466a6000),0x80,"");
    InlineBool("r_fogTweak",false,0x80);
    Store_Renderer_Float("r_fogBaseDist",F(0x0),F(0x0),F(0x469c4000),0x80,"");
    Store_Renderer_Float("r_fogHalfDist",F(0x3f800000),F(0x3f800000),F(0x471c4000),0x80,"");
    Store_Renderer_Float("r_fogBaseHeight",F(0x0),F(0xc7435000),F(0x47435000),0x80,"");
    Store_Renderer_Float("r_fogHalfHeight",F(0x3f800000),F(0x3f800000),F(0x47435000),0x80,"");
    Store_Renderer_Vec3("r_fogColor",F(0x3f000000),F(0x3f000000),F(0x3f000000),F(0x0),F(0x42000000),0x80);
    Store_Renderer_Float("r_fogOpacity",F(0x3e800000),F(0x0),F(0x3f800000),0x80,"");
    Store_Renderer_Vec3("r_fogSunColor",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x0),F(0x42000000),0x80);
    Store_Renderer_Float("r_fogSunOpacity",F(0x3f000000),F(0x0),F(0x3f800000),0x80,"");
    Store_Renderer_Float("r_fogSunPitch",F(0xc2340000),F(0xc3b40000),F(0x43b40000),0x80,"");
    Store_Renderer_Float("r_fogSunYaw",F(0x0),F(0xc3b40000),F(0x43b40000),0x80,"");
    Store_Renderer_Float("r_fogSunInner",F(0x0),F(0x0),F(0x43340000),0x80,"");
    Store_Renderer_Float("r_fogSunOuter",F(0x0),F(0x0),F(0x43340000),0x80,"");
    store_r_exposureTweak=InlineBool("r_exposureTweak",false,0x5000);
    store_r_exposureValue=Store_Renderer_Float("r_exposureValue",F(0x40400000),F(0xc0c00000),F(0x41800000),0x5000,"");
    Store_Renderer_Vec3("r_finalShiftX",F(0x0),F(0x0),F(0x0),F(0xc0a00000),F(0x40a00000),0x5000);
    Store_Renderer_Vec3("r_finalShiftY",F(0x0),F(0x0),F(0x0),F(0xc0a00000),F(0x40a00000),0x5000);
    store_r_primaryLightUseTweaks=InlineBool("r_primaryLightUseTweaks",false,0x80);
    store_r_primaryLightTweakDiffuseStrength=Store_Renderer_Float("r_primaryLightTweakDiffuseStrength",F(0x3f800000),F(0x0),F(0x41200000),0x80,"");
    store_r_primaryLightTweakSpecularStrength=Store_Renderer_Float("r_primaryLightTweakSpecularStrength",F(0x3f800000),F(0x0),F(0x41200000),0x80,"");
    store_r_lightGridEnableTweaks=InlineBool("r_lightGridEnableTweaks",false,0x1000);
    store_r_lightGridIntensity=Store_Renderer_Float("r_lightGridIntensity",F(0x3f800000),F(0x0),F(0x40000000),0x1000,"");
    store_r_lightGridContrast=Store_Renderer_Float("r_lightGridContrast",F(0x0),F(0xbf800000),F(0x3f800000),0x1000,"");
    store_r_heroLighting=InlineBool("r_heroLighting",true,0x0);
    store_r_heroLightScale=Store_Renderer_Vec3("r_heroLightScale",F(0x3f800000),F(0x3f800000),F(0x3f800000),F(0x0),F(0x40000000),Store_Renderer_ModeByte?0x5080:0x5000);
    store_r_num_viewports=InlineInt("r_num_viewports",0,0,4,0x0);
    store_r_outdoorAwayBias=Store_Renderer_Float("r_outdoorAwayBias",F(0x42000000),F(0xff7fffff),F(0x7f7fffff),0x1000,"");
    store_r_outdoorDownBias=Store_Renderer_Float("r_outdoorDownBias",F(0x0),F(0xff7fffff),F(0x7f7fffff),0x1000,"");
    store_r_outdoorFeather=Store_Renderer_Float("r_outdoorFeather",F(0x41000000),F(0xff7fffff),F(0x7f7fffff),0x1000,"");
    store_r_sun_from_dvars=InlineBool("r_sun_from_dvars",false,0x80);
    store_developer=InlineInt("developer",0,0,2,0x0);
    store_sv_cheats=InlineBool("sv_cheats",false,0x48);
    store_r_defaultMode=InlineRendererString("r_defaultMode","",0x0);
    store_r_monitor=InlineInt("r_monitor",0,0,8,0x21);
    store_r_currentMonitor=InlineInt("r_currentMonitor",0,0,8,0x0);
    store_r_monitorCount=InlineInt("r_monitorCount",0,0,8,0x0);
    store_r_aspectRatio=InlineRendererEnum("r_aspectRatio",0,Store_Renderer_AspectRatio,0x21);
    store_r_aspectRatioWindow=Store_Renderer_Float("r_aspectRatioWindow",F(0x3fe38e39),F(0x0),F(0x41200000),0x0,"");
    store_r_customMode=InlineRendererString("r_customMode","",0x21);
    InlineBool("r_open_automate",false,0x0);
    store_r_superflare_drawlist=InlineRendererEnum("r_superflare_drawlist",2,Store_Renderer_Superflare,0x0);
    store_r_grassEnable=InlineBool("r_grassEnable",true,0x0);
    store_r_skyTransition=Store_Renderer_Float("r_skyTransition",F(0x0),F(0x0),F(0x3f800000),0x1000,"");
    store_r_skyRotation=Store_Renderer_Float("r_skyRotation",F(0x0),F(0xc3b40000),F(0x43b40000),0x1000,"");
    store_r_shaderDebugA=InlineInt("r_shaderDebugA",0,0,1,0x1000);
    store_r_shaderDebugB=InlineInt("r_shaderDebugB",0,0,1,0x1000);
    store_r_shaderDebugC=Store_Renderer_Float("r_shaderDebugC",F(0x0),F(0x0),F(0x3f800000),0x1000,"");
    InlineBool("r_enableCubicUpsample",true,0x1000);
    Store_Renderer_Float("r_cubicUpsampleParam",F(0xbf000000),F(0xbf800000),F(0x3f800000),0x1000,"");
    store_r_skyColorTemp=Store_Renderer_Float("r_skyColorTemp",F(0x45cb2000),F(0x44ce4000),F(0x46c35000),0x1000,"");
    InlineInt("r_blinkLitQuasiOpaque",0,0,2,0x80);
    InlineInt("r_blinkTrans",0,0,2,0x80);
    InlineInt("r_blinkLayers",0,0,4,0x80);
    InlineInt("r_blinkCullNone",0,0,2,0x80);
    store_r_fullPrepass=InlineBool("r_fullPrepass",false,0x80);
    store_r_lightmapOnlyBspSurfs=InlineBool("r_lightmapOnlyBspSurfs",true,0x80);
    store_r_lightmapOnlySModelSurfs=InlineBool("r_lightmapOnlySModelSurfs",true,0x80);
    store_r_lightmapOnlyEntSurfs=InlineBool("r_lightmapOnlyEntSurfs",true,0x80);
    store_r_sortDrawSurfsBsp=InlineBool("r_sortDrawSurfsBsp",false,0x80);
    store_r_sortDrawSurfsStaticModel=InlineBool("r_sortDrawSurfsStaticModel",true,0x80);
    InlineBool("r_sortBackToFront",true,0x80);
    Store_Renderer_Float("r_swrk_override_characterCharredAmount",F(0x0),F(0x0),F(0x3f800000),0x80,"");
    store_r_swrk_override_characterDissolveColor=Store_Renderer_Vec4("r_swrk_override_characterDissolveColor",F(0x0),F(0x0),F(0x0),F(0x0),F(0x0),F(0x41200000),0x80);
    Store_Renderer_Float("r_swrk_override_wetness",F(0x0),F(0x0),F(0x3f800000),0x80,"");
    store_r_debugShowPrimaryLights=InlineBool("r_debugShowPrimaryLights",false,0x80);
    InlineBool("r_debugShowDynamicLights",false,0x80);
    InlineBool("r_debugShowCoronas",false,0x80);
    InlineBool("r_debugMetalStorm",false,0x80);
    InlineBool("r_disableGenericFilter",false,0x80);
    InlineBool("r_drawDebugFogParams",false,0x80);
    store_r_seethru_decal_enable=InlineBool("r_seethru_decal_enable",!Store_Renderer_ModeByte,0x1080);
    if(const auto *d=store_r_seethru_decal_enable; d && Read<const char *>(d,0)) {
        Value value{};
        const bool enabled=!Store_Renderer_ModeByte;
        if(Read<int>(d,0x18)==1) value.words[0]=enabled;
        else { const char *str=enabled?"1":"0"; std::memcpy(value.words,&str,sizeof(str)); }
        Store_Renderer_SetVariant(d,&value,0);
    }
    store_r_tension_enable=InlineBool("r_tension_enable",true,0x1080);
    InlineBool("r_ui3d_debug_display",false,0x80);
    InlineInt("r_ui3d_use_debug_values",-1,-1,8,0x80);
    Store_Renderer_Float("r_ui3d_x",F(0x0),F(0x0),F(0x3f800000),0x80,"");
    Store_Renderer_Float("r_ui3d_y",F(0x0),F(0x0),F(0x3f800000),0x80,"");
    Store_Renderer_Float("r_ui3d_w",F(0x3f800000),F(0x0),F(0x3f800000),0x80,"");
    Store_Renderer_Float("r_ui3d_h",F(0x3f800000),F(0x0),F(0x3f800000),0x80,"");
    InlineInt("r_multi_extracam_debug",-1,-1,10,0x80);
    store_r_missile_cam_debug_display=InlineInt("r_missile_cam_debug_display",0,0,10,0x80);
    store_r_extracam_custom_aspectratio=Store_Renderer_Float("r_extracam_custom_aspectratio",F(0xbf800000),F(0xbf800000),F(0x40a00000),0x1000,"");
    store_r_extracam_lod_scale=Store_Renderer_Float("r_extracam_lod_scale",F(0x40000000),F(0x3dcccccd),F(0x42c80000),0x1000,"");
    InlineInt("r_qrcode_debug_display",-1,-1,12,0x80);
    Store_Renderer_Float("r_qrcode_debug_display_size",F(0x43c80000),F(0x0),F(0x44fa0000),0x80,"");
    Store_Renderer_Float("r_shader_constant_set_debug_range",F(0xbf800000),F(0xc0000000),F(0x44fa0000),0x80,"");
    store_r_shader_constant_set_enable=InlineBool("r_shader_constant_set_enable",true,0x80);
    store_r_sky_intensity_showDebugDisplay=InlineBool("r_sky_intensity_showDebugDisplay",false,0x80);
    store_r_sky_intensity_angle0=Store_Renderer_Float("r_sky_intensity_angle0",F(0x42b40000),F(0xc2b40000),F(0x42b40000),0x1000,"");
    store_r_sky_intensity_angle1=Store_Renderer_Float("r_sky_intensity_angle1",F(0x42b40000),F(0xc2b40000),F(0x42b40000),0x1000,"");
    store_r_sky_intensity_factor0=Store_Renderer_Float("r_sky_intensity_factor0",F(0x3f800000),F(0x0),F(0x41200000),0x1000,"");
    store_r_sky_intensity_factor1=Store_Renderer_Float("r_sky_intensity_factor1",F(0x3f800000),F(0x0),F(0x41200000),0x1000,"");
    InlineBool("r_fog_disable",false,0x80);
    InlineBool("r_grassWindForceEnable",true,0x80);
    InlineBool("r_dpvs_useCellForceInvisibleBits",true,0x80);
    InlineBool("r_remotescreen_renderlastframe",true,0x80);
    store_r_vc_compile=InlineInt("r_vc_compile",0,0,2147483647,0x0);
}
void RegisterRendererExtendedPrefix() {
    RegisterRendererRecoveredPrefix();
    RegisterRendererVisibilityShadowFragment();
    RegisterRendererFilmColorFragment();
    RegisterRendererBloomColorMatrixFragment();
    RegisterRendererWaterDofFragment();
    RegisterRendererScreenEffectsFragment();
}

// Whole original body candidate, from the initial call to the final return.
void RegisterRendererComplete() {
    RegisterRendererExtendedPrefix();
    RegisterRendererFinalFragment();
}
} // namespace store_renderer_dvar
#endif
