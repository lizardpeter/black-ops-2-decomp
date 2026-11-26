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
} // namespace store_renderer_dvar
#endif
