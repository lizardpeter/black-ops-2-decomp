// Original Store MP renderer resource initialization.
#define T6_SOURCE "gfx_d3d/r_resource_init.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_resource_init{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
static void Zero(B*p,unsigned n){for(unsigned i=0;i<n;++i)p[i]=0;}
extern "C"{
extern const B *Resource_Anisotropy,*Resource_Memory,*Resource_Override,*Resource_Quality[3];
extern U Resource_Tiers[3];
extern void* Resource_SamplerStates[256];
extern void* Resource_Device;
extern int Resource_DeviceRetry;
extern B Resource_Slots[40*0x38],Resource_SlotsReady;
void Resource_LockSlots();void Resource_UnlockSlots();
int Resource_Atoi(const char*);
int Resource_DDrawCreate(void**);
int Resource_DDrawMemory(void*,const void*,U*,U*);
void Resource_DDrawRelease(void*);
void Resource_DDrawEnumerate(unsigned,U*);
U Resource_DDrawFallback(unsigned);
void Resource_SetDvar(const B*,U,U);
extern const char* Resource_CodeNames[55];
extern void* Resource_CodeImages[55],*Resource_DefaultImages[8],*Resource_QRImages[12];
extern B Resource_CodeSampler[55];
void Resource_ResetCodeSources();
const char* Resource_FormatQR(const char*,U);
void* Resource_FindAsset(U,const char*,bool,U);
void* Resource_CreateImage(const char*,U,U);
void Resource_LockDevice();void Resource_UnlockDevice();
void Resource_ResetTextureBindings();
void* Resource_CreateTarget(U,U,U);
struct MaterialEntry{const char*name;void**out;};
extern MaterialEntry Resource_Materials[100];
void Resource_MissingMaterial(const char*);
extern void *Resource_DefaultTarget,*Resource_DynamicLight,*Resource_MaterialScratch;
extern Q Resource_ImageScratch;
extern B Resource_FrameRecords[4*0x1e4],Resource_ColorState[0x10c],Resource_Ready;
}
static U Int(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(U(t-6)<2)return R<U>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return U(_mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20))));
    return U(Resource_Atoi(R<const char*>(d,0x20)));
}
static bool Bool(const B*d){return d&&(R<int>(d,0x18)==1?d[0x20]!=0:Resource_Atoi(R<const char*>(d,0x20))!=0);}
// 14072AF10: original 36-entry D3D11 filter matrix, cached by sampler code.
void BindSampler(void*context,U slot,U code){
    if(!Resource_SamplerStates[code]){
        const U filters[36]={0,1,0,1,20,20,20,20,20,20,149,149,
                             0,1,0,1,20,21,85,85,85,85,149,149,
                             0,1,0,1,21,21,85,85,85,85,149,149};
        const U tier=Int(Resource_Anisotropy);
        U desc[13]={filters[(static_cast<int>(tier)*6+(code&7))*2+((code>>4)&1)],
                    1+2*((code>>5)&1),1+2*((code>>6)&1),1+2*((code>>7)&1),
                    0,1u<<(tier&31),5,0,0,0,0,0xff7fffff,0x7f7fffff};
        using Create=int(*)(void*,const U*,void**);
        do{auto**v=*static_cast<void***>(Resource_Device);reinterpret_cast<Create>(v[0xb8/8])(Resource_Device,desc,Resource_SamplerStates+code);}while(Resource_DeviceRetry);
    }
    using Bind=void(*)(void*,U,U,void**);
    auto**v=*static_cast<void***>(context);reinterpret_cast<Bind>(v[0x50/8])(context,slot,1,Resource_SamplerStates+code);
}
// 1407405B0: all 40 records, stride 0x38, identifier sentinel at +0x18.
void InitializeResourceSlots(){
    Resource_LockSlots();
    if(!Resource_SlotsReady){
        for(unsigned i=0;i<40;++i){B*p=Resource_Slots+i*0x38;Zero(p,0x38);W(p,0x18,0xffffffffu);}
        Resource_SlotsReady=1;
    }
    Resource_UnlockSlots();
}
// 14073C0F0: original DirectDraw enumeration fallback and quality tiers.
void SelectResourceTiers(){
    U bytes=0;void*dd=nullptr;
    if(Resource_DDrawCreate(&dd)>=0){
        const Q caps[2]={0x10000000,0};U freeBytes;
        int result=Resource_DDrawMemory(dd,caps,&bytes,&freeBytes);
        Resource_DDrawRelease(dd);if(result<0)bytes=0;
    }
    if(!bytes){Resource_DDrawEnumerate(0,&bytes);if(!bytes){bytes=Resource_DDrawFallback(0);if(!bytes)Resource_DDrawEnumerate(1,&bytes);}}
    U rounded=0;
    if(bytes){U mb=((bytes-1)>>20)+1,p=1;while(p<mb)p*=2;rounded=p-mb<33?p:p>>1;}
    U memory=Int(Resource_Memory);
    if(Bool(Resource_Override)){for(unsigned i=0;i<3;++i)Resource_Tiers[i]=Int(Resource_Quality[i]);return;}
    U a,b;
    if(memory<0xa00)a=b=3;
    else if(rounded<450)a=b=1;
    else if(rounded<700){a=0;b=1;}
    else a=b=0;
    Resource_Tiers[0]=a;Resource_Tiers[1]=b;Resource_Tiers[2]=b;
    for(unsigned i=0;i<3;++i)Resource_SetDvar(Resource_Quality[i],Resource_Tiers[i],0);
}
// 140737230: default code source table; untouched slots retain reset-callee values.
void InitializeCodeSources(){
    Resource_ResetCodeSources();
    struct Name{unsigned i;const char*n;};
    const Name names[]={{0,"TEXTURE_SRC_CODE_BLACK"},{1,"TEXTURE_SRC_CODE_WHITE"},{2,"TEXTURE_SRC_CODE_IDENTITY_NORMAL_MAP"},{3,"TEXTURE_SRC_CODE_MODEL_LIGHTING"},
        {7,"shadowmapSamplerSpot"},{8,"feedbackSampler"},{9,"TEXTURE_SRC_CODE_RESOLVED_POST_SUN"},{15,"attenuationSampler"},{16,"dlightAttenuationSampler"},{17,"TEXTURE_SRC_CODE_OUTDOOR"},
        {22,"cinematicY"},{23,"cinematicCr"},{24,"cinematicCb"},{25,"cinematicA"},{28,"TEXTURE_SRC_CODE_TEXTURE_0"},{29,"TEXTURE_SRC_CODE_TEXTURE_1"},{30,"TEXTURE_SRC_CODE_TEXTURE_2"},{31,"TEXTURE_SRC_CODE_TEXTURE_3"},
        {32,"impactMaskSampler"},{33,"ui3dSampler"},{34,"missileCamSampler"},{40,"heatMapSampler"},{41,"sonarColorSampler"},{42,"sonarDepthSampler"}};
    for(const Name&n:names)Resource_CodeNames[n.i]=n.n;
    Resource_CodeImages[0]=Resource_DefaultImages[1];Resource_CodeImages[1]=Resource_DefaultImages[0];Resource_CodeImages[2]=Resource_DefaultImages[5];
    const unsigned nulls[]={3,7,8,9,15,16,17,28,29,30,31};for(unsigned n:nulls)Resource_CodeImages[n]=nullptr;
    Resource_CodeImages[22]=Resource_DefaultImages[1];Resource_CodeImages[23]=Resource_DefaultImages[3];Resource_CodeImages[24]=Resource_DefaultImages[3];Resource_CodeImages[25]=Resource_DefaultImages[1];
    for(unsigned i=32;i<=34;++i)Resource_CodeImages[i]=Resource_DefaultImages[0];
    for(unsigned i=40;i<=42;++i)Resource_CodeImages[i]=Resource_DefaultImages[0];
    W(Resource_CodeSampler,0,0xe2010101u);W(Resource_CodeSampler,7,std::uint16_t(0xe265));Resource_CodeSampler[9]=0x62;
    W(Resource_CodeSampler,15,std::uint16_t(0));Resource_CodeSampler[17]=0x62;
    W(Resource_CodeSampler,22,0x62626262u);W(Resource_CodeSampler,28,0x62626262u);
    W(Resource_CodeSampler,32,std::uint16_t(0x7261));Resource_CodeSampler[34]=0x72;W(Resource_CodeSampler,40,std::uint16_t(0x7272));Resource_CodeSampler[42]=0x72;
    for(unsigned i=0;i<12;++i){const char*n=Resource_FormatQR("qrcode%dSampler",i);Resource_CodeImages[43+i]=Resource_QRImages[i];Resource_CodeSampler[43+i]=0x72;Resource_CodeNames[43+i]=n;}
}
// 140725760: light definition registration.
void* RegisterLightDefinition(const char*name){return Resource_FindAsset(18,name,true,0xffffffffu);}
// 14071E6E0: default resources and material registry initialization.
void InitializeDefaultResources(){
    SelectResourceTiers();
    const char*names[]={"$white","$black","$blank","$gray","$lineargray","$identitynormalmap","$whitetransparent","$blacktransparent"};
    for(unsigned i=0;i<8;++i)Resource_DefaultImages[i]=Resource_CreateImage(names[i],1,0);
    Resource_ImageScratch=0;InitializeCodeSources();
    Resource_LockDevice();Resource_ResetTextureBindings();Resource_UnlockDevice();
    Resource_DefaultTarget=Resource_CreateTarget(12,4,0);
    for(const MaterialEntry&e:Resource_Materials){*e.out=Resource_FindAsset(6,e.name,true,0xffffffffu);if(!*e.out)Resource_MissingMaterial(e.name);Resource_MaterialScratch=nullptr;}
    Resource_DynamicLight=RegisterLightDefinition("light_dynamic");
    for(unsigned i=0;i<4;++i)Zero(Resource_FrameRecords+i*0x1e4,0x1e4);
    Zero(Resource_ColorState,0x10c);W(Resource_ColorState,0,0x3f800000u);
    Resource_Ready=1;
}
}
#endif
