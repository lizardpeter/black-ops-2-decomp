// Store MP remote eye fallback, 1400C3650..1400C3935.
// Original entity/tag selection and input history are preserved.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_remote_fallback {
using Byte=unsigned char;using U32=std::uint32_t;
struct Dvar;
extern "C" {
extern Byte *Remote_State,*Remote_Entities[];
extern const std::int16_t *Remote_ModelMap; // 143FCD390
extern Byte *Remote_Models; // 143F85390
extern const int *Remote_StringOffsets; // 142D03D98
extern const char *Remote_StringBase; // 142D06C3C
extern unsigned short Remote_FallbackTag; // 141EF6B12
extern int Remote_ClientCount; // 140E5CA9C
extern const Byte *Remote_ClientState; // 140E61BF8
extern const Dvar *Remote_Cull; // 140D78610
extern const char *Remote_ErrorText,*Remote_CommandError;
bool Remote_ErrorCheck(); // 1400A9960
void Remote_Error(int,const char*); // 14039AEB0
void Remote_ErrorCommand(int,const char*,int,int); // same variadic boundary
unsigned Remote_FindString(const char*,unsigned); // 14050A9B0
const Byte *Remote_Tag(Byte*,Byte*,unsigned); // 1400E6370
void Remote_TagAxis(const Byte*,float*); // 1400DE970
void Remote_AxisToAngles(const float*,float*); // 140462D70
void Remote_UpdateInput(float*,const Byte*,int); // 140622B70
void Remote_Shake(Byte*,const Byte*); // 1400C33F0
int Remote_Atoi(const char*); // IAT 1408A7DC0
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static int Diff(int a,int b){U32 n=U32(a)-U32(b);int r;std::memcpy(&r,&n,4);return r;}
static Byte Cull(){
    const Dvar*d=Remote_Cull;
    if(!d)return 0;
    if(R<int>(d,0x18)==1)return R<Byte>(d,0x20);
    return Remote_Atoi(R<const char*>(d,0x20))!=0;
}
bool GetWeaponViewEyeValues(int client){
    Byte*cg=Remote_State;
    const int index=R<int>(cg,0x480e8);
    if(index==0x3ff)return false;
    if(Remote_ErrorCheck())Remote_Error(1,Remote_ErrorText);
    Byte*entity=Remote_Entities[client]+std::intptr_t(index)*0x3d8;
    const int model=Remote_ModelMap[client*0x701+R<int>(entity,0x1f4)];
    if(!model)return false;
    Byte*obj=Remote_Models+model*0x90;
    if(!obj)return false;
    const char*name=Remote_StringBase+Remote_StringOffsets[R<int>(cg,0x480ec)];
    if(!name)return false;
    unsigned len=1;while(name[len-1])++len;
    const unsigned id=Remote_FindString(name,len);
    if(!id)return false;
    const Byte*tag=Remote_Tag(entity,obj,id);
    if(!tag)tag=Remote_Tag(entity,obj,Remote_FallbackTag);
    if(!tag)return false;
    float axis[9];Remote_TagAxis(tag,axis);
    for(unsigned i=0;i<3;++i)W(cg,0x4d924+4*i,R<U32>(tag,0x10+4*i));
    float*stored=reinterpret_cast<float*>(cg+0x6a41c);
    if(!R<int>(cg,0x6a418)){Remote_AxisToAngles(axis,stored);W(cg,0x6a418,0);}
    const Byte*cl=client<Remote_ClientCount?Remote_ClientState:nullptr;
    const int current=R<int>(cl,0x44ac4);
    int latest=R<int>(cl,0x44ac4);
    if(current>latest){
        Remote_ErrorCommand(1,Remote_CommandError,current,latest);latest=R<int>(cl,0x44ac4);
    }
    if(current>Diff(latest,128)){
        Byte cmd[0x3c];
        std::memcpy(cmd,cl+0x42cc4+(U32(current)&127u)*0x3c,sizeof(cmd));
        Remote_UpdateInput(stored,cmd,R<int>(cg,0x48088));
    }
    for(unsigned i=0;i<3;++i)W(cg,0x649d0+4*i,R<U32>(cg,0x6a41c+4*i));
    Remote_Shake(cg,entity);
    W(cg,0x64888,Cull());
    return true;
}
}
#endif
