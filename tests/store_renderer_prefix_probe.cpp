// Original-PE prefix replay. No renderer or game is launched.
#define T6_STORE
#define T6_STORE_RENDERER_DVAR_REPLAY
#include "../src/gfx_d3d/r_dvars_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_renderer_dvar {
struct Dvar { alignas(8) unsigned char bytes[104]; U tag; };
extern "C" {
const Dvar *store_r_aaSamples=nullptr;
const Dvar *store_r_aaSamplesMax=nullptr;
const Dvar *store_r_vsync=nullptr;
const Dvar *store_r_clearColor=nullptr;
const Dvar *store_r_clearColor2=nullptr;
const Dvar *store_r_viewportBackingColor=nullptr;
const Dvar *store_r_dlightLimit=nullptr;
const Dvar *store_r_dobjLimit=nullptr;
const Dvar *store_r_modelLimit=nullptr;
const Dvar *store_r_brushLimit=nullptr;
const Dvar *store_r_flashLightFovInnerFraction=nullptr;
const Dvar *store_r_flashLightStartRadius=nullptr;
const Dvar *store_r_flashLightEndRadius=nullptr;
const Dvar *store_r_flashLightBrightness=nullptr;
const Dvar *store_r_flashLightOffset=nullptr;
const Dvar *store_r_flashLightRange=nullptr;
const Dvar *store_r_flashLightColor=nullptr;
const Dvar *store_r_flashLightBobAmount=nullptr;
const Dvar *store_r_flashLightBobRate=nullptr;
const Dvar *store_r_flashLightFlickerAmount=nullptr;
const Dvar *store_r_flashLightFlickerRate=nullptr;
const Dvar *store_r_lockPvs=nullptr;
const Dvar *store_r_lockPvsInCode=nullptr;
const Dvar *store_r_skipPvs=nullptr;
const Dvar *store_r_singleCell=nullptr;
const Dvar *store_r_portalWalkLimit=nullptr;
const Dvar *store_r_portalMinRecurseDepth=nullptr;
const Dvar *store_r_enableOccluders=nullptr;
const Dvar *store_r_highLodDist=nullptr;
const Dvar *store_r_mediumLodDist=nullptr;
const Dvar *store_r_lowLodDist=nullptr;
const Dvar *store_r_lowestLodDist=nullptr;
const Dvar *store_r_forceLod=nullptr;
const Dvar *store_r_modelVertColor=nullptr;
const Dvar *store_sm_sunEnable=nullptr;
const Dvar *store_sm_sunQuality=nullptr;
const Dvar *store_sm_spotQuality=nullptr;
const Dvar *store_sm_spotEnable=nullptr;
const Dvar *store_sm_maxLights=nullptr;
const Dvar *store_sm_spotShadowFadeTime=nullptr;
const Dvar *store_sm_sunAlwaysCastsShadow=nullptr;
const Dvar *store_sm_polygonOffsetBias=nullptr;
const Dvar *store_sm_sunSampleSizeNear=nullptr;
const Dvar *store_sm_sunShadowCenter=nullptr;
const Dvar *store_sm_sunShadowScale=nullptr;
const Dvar *store_sm_sunShadowSmall=nullptr;
const Dvar *store_sm_sunShadowSmallEnable=nullptr;
const Dvar *store_r_flameScaler=nullptr;
const Dvar *store_r_zombieNameAllowFriendsList=nullptr;
const Dvar *store_r_zombieNameAllowDevList=nullptr;
const Dvar *store_sm_strictCull=nullptr;
const Dvar *store_sm_fastSunShadow=nullptr;
const Dvar *store_r_noipak=nullptr;
const Dvar *store_r_dualPlayEnable=nullptr;
const Dvar *store_r_dualPlayActive=nullptr;
const Dvar *store_r_blur=nullptr;
const Dvar *store_r_flame_allowed=nullptr;
const Dvar *store_r_enablePlayerShadow=nullptr;
const Dvar *store_r_enableFlashlight=nullptr;
const Dvar *store_r_brightness=nullptr;
const Dvar *store_r_filmUseTweaks=nullptr;
const Dvar *store_r_filmTweakEnable=nullptr;
const Dvar *store_exp_softclip=nullptr;
const Dvar *store_vc_LIB=nullptr;
const Dvar *store_vc_LIG=nullptr;
const Dvar *store_vc_LIW=nullptr;
const Dvar *store_vc_LOB=nullptr;
const Dvar *store_vc_LOW=nullptr;
const Dvar *store_vc_RGBH=nullptr;
const Dvar *store_vc_RGBL=nullptr;
const Dvar *store_vc_YH=nullptr;
const Dvar *store_vc_YL=nullptr;
const Dvar *store_r_bloomTweaks=nullptr;
const Dvar *store_r_bloomHiQuality=nullptr;
const Dvar *store_r_dofHDR=nullptr;
const Dvar *store_r_zombieDarkness=nullptr;
const Dvar *store_vc_RS=nullptr;
const Dvar *store_vc_RE=nullptr;
const Dvar *store_vc_SMR=nullptr;
const Dvar *store_vc_SMG=nullptr;
const Dvar *store_vc_SMB=nullptr;
const Dvar *store_vc_MMR=nullptr;
const Dvar *store_vc_MMG=nullptr;
const Dvar *store_vc_MMB=nullptr;
const Dvar *store_vc_HMR=nullptr;
const Dvar *store_vc_HMG=nullptr;
const Dvar *store_vc_HMB=nullptr;
const Dvar *store_vc_FGM=nullptr;
const Dvar *store_vc_FSM=nullptr;
const Dvar *store_vc_FBM=nullptr;
const Dvar *store_vc_LUT=nullptr;
const Dvar *store_r_superFlareDraw=nullptr;
const Dvar *store_r_waterWaveAngle=nullptr;
const Dvar *store_r_waterWaveWavelength=nullptr;
const Dvar *store_r_waterWaveAmplitude=nullptr;
const Dvar *store_r_waterWavePhase=nullptr;
const Dvar *store_r_waterWaveSteepness=nullptr;
const Dvar *store_r_waterWaveSpeed=nullptr;
const Dvar *store_r_waterWaveBase=nullptr;
const Dvar *store_r_waterWaveNormalScale=nullptr;
const Dvar *store_r_waterWaveScriptShader0=nullptr;
const Dvar *store_r_waterWaveScriptShader1=nullptr;
const Dvar *store_r_dof_enable=nullptr;
const Dvar *store_r_dof_tweak=nullptr;
const Dvar *store_r_dof_nearBlur=nullptr;
const Dvar *store_r_dof_farBlur=nullptr;
const Dvar *store_r_dof_viewModelStart=nullptr;
const Dvar *store_r_dof_viewModelEnd=nullptr;
const Dvar *store_r_dof_nearStart=nullptr;
const Dvar *store_r_dof_nearEnd=nullptr;
const Dvar *store_r_dof_farStart=nullptr;
const Dvar *store_r_dof_farEnd=nullptr;
const Dvar *store_r_dof_bias=nullptr;
const Dvar *store_r_dof_showdebug=nullptr;
unsigned char Store_Renderer_ModeByte=0;
const Dvar *store_r_texFilterMipBias=nullptr;
const Dvar *store_r_fullbright=nullptr;
const Dvar *store_r_debugShader=nullptr;
const Dvar *store_r_debugPerformance=nullptr;
const Dvar *store_r_loadingScreen=nullptr;
const Dvar *store_r_multiGpu=nullptr;
const Dvar *store_r_texFilterQuality=nullptr;
const Dvar *store_r_lodScaleRigid=nullptr;
const Dvar *store_r_lodBiasRigid=nullptr;
const Dvar *store_r_lodScaleSkinned=nullptr;
const Dvar *store_r_lodBiasSkinned=nullptr;
const Dvar *store_r_skinnedCacheOverflowLodMaxScale=nullptr;
const Dvar *store_r_znear=nullptr;
const Dvar *store_r_zfar=nullptr;
const Dvar *store_r_picmip_manual=nullptr;
const Dvar *store_r_picmip=nullptr;
const Dvar *store_r_picmip_bump=nullptr;
const Dvar *store_r_picmip_spec=nullptr;
const Dvar *store_r_colorMap=nullptr;
const Dvar *store_r_normalMap=nullptr;
const Dvar *store_r_alphaMap=nullptr;
const Dvar *store_r_specularMap=nullptr;
const Dvar *store_r_glossMap=nullptr;
const Dvar *store_r_occlusionMap=nullptr;
const Dvar *store_r_warmShaders=nullptr;
const Dvar *store_r_loadForRenderer=nullptr;
const Dvar *store_r_shaderWarmingPerFrame=nullptr;
const Dvar *store_r_showMissingLightGrid=nullptr;
const Dvar *store_r_lightTweakAmbient=nullptr;
const Dvar *store_r_lightTweakSunLight=nullptr;
const Dvar *store_r_lightTweakAmbientColor=nullptr;
const Dvar *store_r_lightTweakSunColor=nullptr;
const Dvar *store_r_lightTweakSunDiffuseColor=nullptr;
const Dvar *store_r_lightTweakSunDirection=nullptr;
const Dvar *store_r_norefresh=nullptr;
const Dvar *store_r_scaleViewport=nullptr;
U32 Store_Renderer_HardwareCount=0;
const Dvar *store_r_smp_worker_threads=nullptr;
const Dvar *store_r_smp_worker_thread[8]{};
const Dvar *store_r_txaa=nullptr;
const Dvar *store_r_txaaSupported=nullptr;
const Dvar *store_r_txaaDepth1=nullptr;
const Dvar *store_r_txaaDepth2=nullptr;
const Dvar *store_r_txaaMotion1=nullptr;
const Dvar *store_r_txaaMotion2=nullptr;
const Dvar *store_vid_xpos=nullptr;
const Dvar *store_vid_ypos=nullptr;
const Dvar *store_r_fullscreen=nullptr;
const Dvar *store_r_previousFullscreen=nullptr;
const Dvar *store_r_convergence=nullptr;
const Dvar *store_r_ignorehwgamma=nullptr;
const Dvar *store_r_texFilterAnisoMax=nullptr;
const Dvar *store_r_texFilterAnisoMin=nullptr;
const Dvar *store_r_texFilterMipMode=nullptr;
const Dvar *store_r_fontResolution=nullptr;
const Dvar *store_r_resolution=nullptr;
const Dvar *store_r_ssao=nullptr;
const Dvar *store_r_ssaoScale=nullptr;
const Dvar *store_r_ssaoRadius=nullptr;
const Dvar *store_r_ssaoBias=nullptr;
const Dvar *store_r_ssaoIntensity=nullptr;
}
namespace {
struct Event { U kind,name,type,flags,value,min,max,y,z,w; };
struct Trace { U count; Event events[384]; };
Trace trace{};
unsigned mask;
void *mapped_image;
bool worker_mode=false;
bool post_worker_mode=false;
bool integrated_mode=false;
unsigned integrated_worker_exists,integrated_post_mask;
bool extended_mode=false, visibility_mode=false;
U visibility_mask;
unsigned worker_type;
int worker_value;
Dvar slots[384]{};
unsigned used;
U Hash(const char *s) {
    U h=14695981039346656037ull;
    while (*s) { h^=static_cast<unsigned char>(*s++); h*=1099511628211ull; }
    return h;
}
void Push(U k,U n=0,U t=0,U f=0,U v=0,U lo=0,U hi=0,U y=0,U z=0,U w=0) {
    assert(trace.count<384);
    trace.events[trace.count++]={k,n,t,f,v,lo,hi,y,z,w};
}
Dvar *Slot(const char *n) {
    U h=Hash(n);
    for (unsigned i=0;i<used;++i) if(slots[i].tag==h) return &slots[i];
    assert(used<384); slots[used].tag=h; return &slots[used++];
}
template<class T> void Write(Dvar *d,unsigned off,T v) { std::memcpy(d->bytes+off,&v,sizeof(v)); }
Dvar *WorkerSlot(const char *n) {
    auto *d=Slot(n);
    if (!std::strcmp(n,"r_smp_worker_threads")) {
        Write(d,0x18,int(worker_type));
        switch(worker_type) {
        case 1: Write(d,0x20,static_cast<unsigned char>(worker_value!=0)); break;
        case 2: Write(d,0x20,float(worker_value)+0.25f); break;
        case 6: case 7: Write(d,0x20,worker_value); break;
        default: {
            static const char *const nums[]={"0","1","2","3","4","5","6","7","8"};
            Write(d,0x20,worker_value<0?"-1":nums[worker_value]); break;
        }
        }
    }
    return d;
}
U32 Bits(float f) { U32 v; std::memcpy(&v,&f,4); return v; }
void Empty(const char *s) { assert(s && !*s); }
}
extern "C" {
void Store_Renderer_Init() { Push(0); }
const Dvar *Store_Renderer_Find(const char *n) {
    if(extended_mode && !std::strcmp(n,"r_showForcedInvisibleCells")) {
        visibility_mode=true; worker_mode=false; post_worker_mode=false;
    }
    if(visibility_mode) {
        const char *names[]={"r_showForcedInvisibleCells","r_portalWalkLimit","r_portalMinRecurseDepth","r_enableOccluders","r_forceLod","r_modelVertColor","sm_sunEnable","sm_sunQuality","sm_spotQuality","sm_spotEnable","sm_maxLights","sm_sunAlwaysCastsShadow","sm_polygonOffsetBias","sm_sunShadowSmall","sm_sunShadowSmallEnable","r_zombieDisableSlideEffect","r_zombieDisableEarthEffect","r_zombieNameAllowFriendsList","r_zombieNameAllowDevList","sm_strictCull","sm_fastSunShadow","r_noipak","r_stereo3DAvailable","r_stereo3DOn","r_stereo3DMode","r_dualPlayEnable","r_dualPlayActive","r_dualPlayTestCount","r_flame_allowed","r_filmTweakLut","r_forceCameraNode","r_forceCameraHdg","r_forceCamerarecord","r_enablePlayerShadow","r_enableFlashlight","r_waterFogTest","r_filmTweakInvert","r_filmUseTweaks","r_filmTweakEnable","r_bloomTweaks","r_bloomHiQuality","r_dofHDR","r_zombieDarkness","vc_LUT","vc_SNAP","r_superFlareDraw","r_dof_enable","r_dof_tweak","r_dof_showdebug"};
        unsigned i=0; while(i<sizeof(names)/sizeof(names[0]) && std::strcmp(n,names[i])) ++i;
        assert(i<sizeof(names)/sizeof(names[0]));
        bool exists=(visibility_mask&(U(1)<<i))!=0;
        Push(1,Hash(n),0,0,exists); return exists?Slot(n):nullptr;
    }
    if(integrated_mode) {
        if(!std::strcmp(n,"r_smp_worker_threads")) {
            worker_mode=true; post_worker_mode=false; mask=integrated_worker_exists;
        } else if(!std::strcmp(n,"r_aaSamples")) {
            worker_mode=false; post_worker_mode=true; mask=integrated_post_mask;
        }
    }
    if(post_worker_mode) {
        const char *names[]={"r_aaSamples","r_aaSamplesMax","r_vsync","r_vsyncThreshold","r_dlightLimit","r_dobjLimit","r_modelLimit","r_brushLimit","r_spotLightShadows","r_spotLightSModelShadows","r_spotLightEntityShadows","r_flashLightShadows","r_drawWater","r_lockPvs","r_lockPvsInCode","r_skipPvs","r_singleCell"};
        unsigned i=0; while(i<sizeof(names)/sizeof(names[0]) && std::strcmp(n,names[i])) ++i;
        assert(i<sizeof(names)/sizeof(names[0]));
        const bool exists=(mask&(1u<<i))!=0;
        Push(1,Hash(n),0,0,exists); return exists?Slot(n):nullptr;
    }
    if(worker_mode) {
        assert(!std::strcmp(n,"r_smp_worker_threads"));
        const bool exists=(mask&1)!=0;
        Push(1,Hash(n),0,0,exists);
        return exists?WorkerSlot(n):nullptr;
    }
    const char *names[]={"r_fontResolution","r_resolution","r_txaaMode","r_clipFPS","vid_xpos","vid_ypos","r_fullscreen","r_previousFullscreen","r_texFilterAnisoMax","r_texFilterAnisoMin","r_zebrastripe","r_texFilterQuality","r_picmip","r_picmip_bump","r_picmip_spec","r_shaderWarmingPerFrame","r_lightTweakSunColor","r_materialXYZ"};
    unsigned index=0; while(index<sizeof(names)/sizeof(names[0]) && std::strcmp(n,names[index])) ++index;
    assert(index<sizeof(names)/sizeof(names[0])); const unsigned bit=1u<<index;
    const bool exists=(mask&bit)!=0;
    Push(1,Hash(n),0,0,exists);
    return exists?Slot(n):nullptr;
}
static void PushInline(U k,const char *n,int t,U32 f,const Value *v,const Domain *d) {
    if(t==7) {
        assert(!std::strcmp(n,"r_forceLod"));
        assert(d->words[0]==5);
        const char *const *ptr; std::memcpy(&ptr,&d->words[2],sizeof(ptr));
        if(mapped_image) {
            auto *base=static_cast<unsigned char *>(mapped_image);
            assert(reinterpret_cast<const void *>(ptr)==base+0xd3a0e0);
            for(unsigned i=0;i<5;++i) {
                U va; std::memcpy(&va,base+0xd3a0e0+8*i,8);
                assert(va>=0x140000000ull && va<0x141000000ull);
                assert(!std::strcmp(reinterpret_cast<const char *>(base+va-0x140000000ull),Store_Renderer_ForceLod[i]));
            }
            U terminator; std::memcpy(&terminator,base+0xd3a0e0+40,8); assert(!terminator);
        } else assert(ptr==Store_Renderer_ForceLod);
        Push(k,Hash(n),t,f,v->words[0],d->words[0],0xd3a0e0);
    } else Push(k,Hash(n),t,f,v->words[0],d->words[0],d->words[1],t==11||t==12?v->words[1]:0,t==11||t==12?v->words[2]:0);
}
const Dvar *Store_Renderer_New(const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) {
    Empty(s); PushInline(2,n,t,f,v,d); return worker_mode?WorkerSlot(n):Slot(n);
}
void Store_Renderer_Reregister(const Dvar *old,const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) {
    Empty(s); assert(old==Slot(n)); PushInline(3,n,t,f,v,d);
}
const Dvar *Store_Renderer_Bool(const char *n,bool v,U32 f,const char *s) {
    Empty(s); Push(4,Hash(n),1,f,v); return Slot(n);
}
const Dvar *Store_Renderer_Float(const char *n,float v,float lo,float hi,U32 f,const char *s) {
    Empty(s); Push(5,Hash(n),2,f,Bits(v),Bits(lo),Bits(hi)); return Slot(n);
}
const Dvar *Store_Renderer_Enum(const char *n,const char *const *o,int v,U32 f) {
    const char *names[]={"r_clipCodec","r_clipSize","r_texFilterMipMode","r_debugShader","r_debugPerformance","r_lightMapSecondary","r_colorMap","r_alphaMap","r_specularMap","r_glossMap","r_occlusionMap","r_showPenetration","r_showReflectionProbeSelection"};
    const char *const *tables[]={Store_Renderer_ClipCodec,Store_Renderer_ClipSize,Store_Renderer_MipMode,Store_Renderer_DebugShader,Store_Renderer_DebugPerformance,Store_Renderer_LightMapSecondary,Store_Renderer_ColorMap,Store_Renderer_AlphaMap,Store_Renderer_SpecularMap,Store_Renderer_GlossMap,Store_Renderer_OcclusionMap,Store_Renderer_Penetration,Store_Renderer_ReflectionProbe};
    const unsigned rvas[]={0xd39f30,0xd3a1e0,0xd3a138,0xd3a1a8,0xd3a230,0xd39f50,0xd3a088,0xd3a0b8,0xd3a250,0xd3a270,0xd3a208,0xd3a160,0xd3a110};
    const unsigned counts[]={2,4,4,6,3,33,5,4,3,7,4,3,4};
    unsigned i=0; while(i<13 && std::strcmp(n,names[i])) ++i;
    assert(i<13);
    if(mapped_image) {
        auto *base=static_cast<unsigned char *>(mapped_image);
        assert(reinterpret_cast<const void *>(o)==base+rvas[i]);
        if(counts[i]) {
            for(unsigned k=0;k<counts[i];++k) {
                U ptr; std::memcpy(&ptr,base+rvas[i]+8*k,8);
                assert(ptr>=0x140000000ull && ptr<0x141000000ull);
                assert(!std::strcmp(reinterpret_cast<const char *>(base+ptr-0x140000000ull),tables[i][k]));
            }
            U end; std::memcpy(&end,base+rvas[i]+8*counts[i],8); assert(!end);
        }
    } else assert(o==tables[i]);
    Push(6,Hash(n),7,f,U32(v),rvas[i]); return Slot(n);
}
const Dvar *Store_Renderer_Color(const char *n,float a,float b,float c,float d,U32 f) {
    Push(7,Hash(n),9,f,Bits(a),0,0,Bits(b),Bits(c),Bits(d)); return Slot(n);
}
const Dvar *Store_Renderer_Vec3(const char *n,float a,float b,float c,float lo,float hi,U32 f) {
    Push(8,Hash(n),4,f,Bits(a),Bits(lo),Bits(hi),Bits(b),Bits(c)); return Slot(n);
}
const Dvar *Store_Renderer_Vec4(const char *n,float a,float b,float c,float d,float lo,float hi,U32 f) {
    Push(9,Hash(n),5,f,Bits(a),Bits(lo),Bits(hi),Bits(b),Bits(c),Bits(d)); return Slot(n);
}
}
static const Dvar *MS MVec4(const char *n,float a,float b,float c,float d,float lo,float hi,U32 f) { return Store_Renderer_Vec4(n,a,b,c,d,lo,hi,f); }
static const Dvar *MS MColor(const char *n,float a,float b,float c,float d,U32 f) { return Store_Renderer_Color(n,a,b,c,d,f); }
static const Dvar *MS MVec3(const char *n,float a,float b,float c,float lo,float hi,U32 f) { return Store_Renderer_Vec3(n,a,b,c,lo,hi,f); }
static const Dvar *MS MEnum(const char *n,const char *const *o,int v,U32 f) { return Store_Renderer_Enum(n,o,v,f); }
static int MS MAtoi(const char *n) { return std::atoi(n); }
static void MS MInit() { Store_Renderer_Init(); }
static const Dvar *MS MFind(const char *n) { return Store_Renderer_Find(n); }
static const Dvar *MS MNew(const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) { return Store_Renderer_New(n,t,f,v,d,s); }
static void MS MReg(const Dvar *p,const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) { Store_Renderer_Reregister(p,n,t,f,v,d,s); }
static const Dvar *MS MBool(const char *n,bool v,U32 f,const char *s) { return Store_Renderer_Bool(n,v,f,s); }
static const Dvar *MS MFloat(const char *n,float v,float lo,float hi,U32 f,const char *s) { return Store_Renderer_Float(n,v,lo,hi,f,s); }
extern "C" {
void *RendererPrefixMock(unsigned i) {
    void *m[]={reinterpret_cast<void *>(MInit),reinterpret_cast<void *>(MFind),reinterpret_cast<void *>(MNew),
               reinterpret_cast<void *>(MReg),reinterpret_cast<void *>(MBool),reinterpret_cast<void *>(MFloat),reinterpret_cast<void *>(MEnum),reinterpret_cast<void *>(MAtoi),reinterpret_cast<void *>(MColor),reinterpret_cast<void *>(MVec3),reinterpret_cast<void *>(MVec4)};
    assert(i<11); return m[i];
}
unsigned RendererPrefixSize() { return sizeof(Trace); }
void RendererPrefixRun(void *image,unsigned state,void *out) {
    trace={}; used=0; extended_mode=false; visibility_mode=false; mask=state; mapped_image=image; worker_mode=false; post_worker_mode=false; integrated_mode=false; Store_Renderer_ModeByte=(state>>18)&1;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererEntryPrefix();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererWorkerRun(void *image,unsigned exists,unsigned hardware,unsigned type,int value,void *out) {
    trace={}; used=0; extended_mode=false; visibility_mode=false; mask=exists; mapped_image=image; worker_mode=true; post_worker_mode=false; integrated_mode=false;
    worker_type=type; worker_value=value; Store_Renderer_HardwareCount=hardware;
    store_r_smp_worker_threads=nullptr;
    for(auto &d:store_r_smp_worker_thread) d=nullptr;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererWorkerFragment();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererWorkerGlobals(U *out) {
    out[0]=reinterpret_cast<U>(store_r_smp_worker_threads);
    for(unsigned i=0;i<8;++i) out[i+1]=reinterpret_cast<U>(store_r_smp_worker_thread[i]);
}
void RendererPostWorkerRun(void *image,unsigned state,unsigned mode,void *out) {
    trace={}; used=0; extended_mode=false; visibility_mode=false; mask=state; mapped_image=image; worker_mode=false; post_worker_mode=true; integrated_mode=false;
    Store_Renderer_ModeByte=mode;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererPostWorkerFragment();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererPostWorkerGlobals(U *out) {
    const Dvar *values[]={store_r_aaSamples,store_r_aaSamplesMax,store_r_vsync,store_r_clearColor,store_r_clearColor2,store_r_viewportBackingColor,store_r_dlightLimit,store_r_dobjLimit,store_r_modelLimit,store_r_brushLimit,store_r_flashLightFovInnerFraction,store_r_flashLightStartRadius,store_r_flashLightEndRadius,store_r_flashLightBrightness,store_r_flashLightOffset,store_r_flashLightRange,store_r_flashLightColor,store_r_flashLightBobAmount,store_r_flashLightBobRate,store_r_flashLightFlickerAmount,store_r_flashLightFlickerRate,store_r_lockPvs,store_r_lockPvsInCode,store_r_skipPvs,store_r_singleCell};
    for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);++i) out[i]=reinterpret_cast<U>(values[i]);
}
void RendererIntegratedRun(void *image,unsigned early,unsigned later,unsigned exists,unsigned hardware,unsigned type,int value,unsigned mode,void *out) {
    trace={}; used=0; extended_mode=false; visibility_mode=false; mask=early; mapped_image=image; worker_mode=false; post_worker_mode=false; integrated_mode=true;
    integrated_worker_exists=exists; integrated_post_mask=later; worker_type=type; worker_value=value;
    Store_Renderer_HardwareCount=hardware; Store_Renderer_ModeByte=mode;
    store_r_smp_worker_threads=nullptr;
    for(auto &d:store_r_smp_worker_thread) d=nullptr;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererRecoveredPrefix();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererExtendedRun(void *image,unsigned early,unsigned later,unsigned exists,unsigned hardware,unsigned type,int value,unsigned mode,U vis,void *out) {
    trace={}; used=0; mask=early; mapped_image=image; worker_mode=false; post_worker_mode=false; integrated_mode=true;
    extended_mode=true; visibility_mode=false; visibility_mask=vis;
    integrated_worker_exists=exists; integrated_post_mask=later; worker_type=type; worker_value=value;
    Store_Renderer_HardwareCount=hardware; Store_Renderer_ModeByte=mode;
    store_r_smp_worker_threads=nullptr;
    for(auto &d:store_r_smp_worker_thread) d=nullptr;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererExtendedPrefix();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererVisibilityGlobals(U *out) {
    const Dvar *values[]={store_r_portalWalkLimit,store_r_portalMinRecurseDepth,store_r_enableOccluders,store_r_highLodDist,store_r_mediumLodDist,store_r_lowLodDist,store_r_lowestLodDist,store_r_forceLod,store_r_modelVertColor,store_sm_sunEnable,store_sm_sunQuality,store_sm_spotQuality,store_sm_spotEnable,store_sm_maxLights,store_sm_spotShadowFadeTime,store_sm_sunAlwaysCastsShadow,store_sm_polygonOffsetBias,store_sm_sunSampleSizeNear,store_sm_sunShadowCenter,store_sm_sunShadowScale,store_sm_sunShadowSmall,store_sm_sunShadowSmallEnable,store_r_flameScaler,store_r_zombieNameAllowFriendsList,store_r_zombieNameAllowDevList,store_sm_strictCull,store_sm_fastSunShadow,store_r_noipak,store_r_dualPlayEnable,store_r_dualPlayActive,store_r_blur,store_r_flame_allowed,store_r_enablePlayerShadow,store_r_enableFlashlight,store_r_brightness,store_r_filmUseTweaks,store_r_filmTweakEnable,store_exp_softclip,store_vc_LIB,store_vc_LIG,store_vc_LIW,store_vc_LOB,store_vc_LOW,store_vc_RGBH,store_vc_RGBL,store_vc_YH,store_vc_YL,store_r_bloomTweaks,store_r_bloomHiQuality,store_r_dofHDR,store_r_zombieDarkness,store_vc_RS,store_vc_RE,store_vc_SMR,store_vc_SMG,store_vc_SMB,store_vc_MMR,store_vc_MMG,store_vc_MMB,store_vc_HMR,store_vc_HMG,store_vc_HMB,store_vc_FGM,store_vc_FSM,store_vc_FBM,store_vc_LUT,store_r_superFlareDraw,store_r_waterWaveAngle,store_r_waterWaveWavelength,store_r_waterWaveAmplitude,store_r_waterWavePhase,store_r_waterWaveSteepness,store_r_waterWaveSpeed,store_r_waterWaveBase,store_r_waterWaveNormalScale,store_r_waterWaveScriptShader0,store_r_waterWaveScriptShader1,store_r_dof_enable,store_r_dof_tweak,store_r_dof_nearBlur,store_r_dof_farBlur,store_r_dof_viewModelStart,store_r_dof_viewModelEnd,store_r_dof_nearStart,store_r_dof_nearEnd,store_r_dof_farStart,store_r_dof_farEnd,store_r_dof_bias,store_r_dof_showdebug};
    for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);++i) out[i]=reinterpret_cast<U>(values[i]);
}
void RendererPrefixGlobals(U *out) {
    const Dvar *slots_[]={store_r_fontResolution,store_r_resolution,store_r_ssao,store_r_ssaoScale,
                          store_r_ssaoRadius,store_r_ssaoBias,store_r_ssaoIntensity,
                          store_r_txaa,store_r_txaaSupported,store_r_txaaDepth1,store_r_txaaDepth2,store_r_txaaMotion1,store_r_txaaMotion2,store_vid_xpos,store_vid_ypos,store_r_fullscreen,store_r_previousFullscreen,store_r_convergence,store_r_ignorehwgamma,store_r_texFilterAnisoMax,store_r_texFilterAnisoMin,store_r_texFilterMipMode,store_r_texFilterMipBias,store_r_fullbright,store_r_debugShader,store_r_debugPerformance,store_r_loadingScreen,store_r_multiGpu,store_r_texFilterQuality,store_r_lodScaleRigid,store_r_lodBiasRigid,store_r_lodScaleSkinned,store_r_lodBiasSkinned,store_r_skinnedCacheOverflowLodMaxScale,store_r_znear,store_r_zfar,store_r_picmip_manual,store_r_picmip,store_r_picmip_bump,store_r_picmip_spec,store_r_colorMap,store_r_normalMap,store_r_alphaMap,store_r_specularMap,store_r_glossMap,store_r_occlusionMap,store_r_warmShaders,store_r_loadForRenderer,store_r_shaderWarmingPerFrame,store_r_showMissingLightGrid,store_r_lightTweakAmbient,store_r_lightTweakSunLight,store_r_lightTweakAmbientColor,store_r_lightTweakSunColor,store_r_lightTweakSunDiffuseColor,store_r_lightTweakSunDirection,store_r_norefresh,store_r_scaleViewport};
    for(unsigned i=0;i<58;++i) out[i]=reinterpret_cast<U>(slots_[i]);
}
}
} // namespace store_renderer_dvar
