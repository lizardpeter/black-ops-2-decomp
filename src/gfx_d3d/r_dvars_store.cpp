// Store MP renderer registration: recovered entry prefix 14070E2B5..14070E516.
// This is NOT the complete 14070E230 function. Replay-only until all paths are recovered.
#define T6_SOURCE "gfx_d3d/r_dvars.cpp"
#if defined(T6_STORE) && defined(T6_STORE_RENDERER_DVAR_REPLAY)
#include <cstdint>
#include <cstring>

namespace store_renderer_dvar {
using U32 = std::uint32_t;
struct Dvar;
struct alignas(16) Value { U32 words[4]; };
struct alignas(16) Domain { U32 words[4]; };
extern "C" {
void Store_Renderer_Init(); // 140763730
const Dvar *Store_Renderer_Find(const char *);
const Dvar *Store_Renderer_New(const char *,int,U32,const Value *,const Domain *,const char *);
void Store_Renderer_Reregister(const Dvar *,const char *,int,U32,const Value *,const Domain *,const char *);
const Dvar *Store_Renderer_Bool(const char *,bool,U32,const char *);
const Dvar *Store_Renderer_Float(const char *,float,float,float,U32,const char *);
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
    // 14070E512; end before the next instruction (14070E517).
    Store_Renderer_Bool("r_ssaoDebug",false,0,"");
}
} // namespace store_renderer_dvar
#endif
