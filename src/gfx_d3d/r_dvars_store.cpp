// Store MP renderer registration: recovered entry prefix 14070E2B5..14070ECCF.
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
extern const char *const Store_Renderer_ClipCodec[]; // 140D39F30
extern const char *const Store_Renderer_ClipSize[]; // 140D3A1E0
extern const char *const Store_Renderer_MipMode[]; // 140D3A138
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
} // namespace store_renderer_dvar
#endif
