// Store MP 14016CE80..14016D269, complete extra-camera FOV/view body.
// The symbol-bearing reference identifies CG_CalcFov_ExtraCam, with
// surrounding view logic inlined by LTCG in this Store revision.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstring>
#include <cstdint>
namespace store_extra_camera {
using Byte=unsigned char;
using U32=std::uint32_t;
struct Dvar;
extern "C" {
extern Byte *Extra_State; // 140E53A20
extern const Dvar *Extra_Fov; // 140D78638
extern const Dvar *Extra_Aspect; // 1427E6E58
extern U32 Extra_ViewportWidth,Extra_ViewportHeight; // 143043C38, 143043C3C
extern U32 Extra_AltWidth,Extra_AltHeight; // 143043C40, 143043C44
extern int Extra_DemoMode; // 1425A3308
extern Byte Extra_DemoFlags; // 1425A330C
extern Byte Extra_PlaybackInited; // 142653908
extern const Byte *Extra_Playback; // 142653900
int Extra_GetMode(int); // 1400C1240
void Extra_Update(int,Byte); // 140173ED0
void Extra_Special(Byte*); // 14016BDA0
void Extra_Linked(int,int); // 140100FC0
void Extra_Mode6A(int); // 1400C3AE0
void Extra_Mode6B(int); // 1400C3E10
void Extra_Mode7(); // 1400C3390; argument lanes not prepared at this site
void Extra_AnglesToAxis(const float*,float*); // 140464D00
float Extra_Tan(float); // 140892D02
double Extra_Atof(const char*); // IAT 1408A7DD8
}
template<class T>static T Read(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void Write(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float GetFloat(const Dvar*d){
    if(!d)return 0;
    const int t=Read<int>(d,0x18);
    if(t==2)return Read<float>(d,0x20);
    if(t==6)return float(Read<int>(d,0x20));
    return float(Extra_Atof(Read<const char*>(d,0x20)));
}
static void CopyVec(Byte*dst,unsigned off,const Byte*src,unsigned from){
    for(unsigned i=0;i<3;++i)Write(dst,off+4*i,Read<U32>(src,from+4*i));
}
static void CopyPlayerView(Byte*cg){
    CopyVec(cg,0x4d924,cg,0x480d0);
    CopyVec(cg,0x649d0,cg,0x482a0);
}
void CalcExtraView(int client){
    Byte *const initial=Extra_State;
    Write(initial,0x648e0,1.0f);Write(initial,0x4e82c,1.0f);
    const int mode=Extra_GetMode(client);
    Byte *const output=Extra_State;
    float fov=GetFloat(Extra_Fov);
    const Byte *current=Extra_State;
    if(Read<Byte>(current,0x64b00))
        fov=Read<float>(current,0x64aec+4*Read<int>(current,0x64afc));
    float horizontal=Extra_Tan(fov*0.008726646192371845245361328125f);
    float vertical=horizontal;
    float aspect=GetFloat(Extra_Aspect);
    if(aspect>0.0f && aspect<1.0f)vertical=horizontal/aspect;
    else {
        if(!(aspect>0.0f))aspect=1.0f;
        horizontal=aspect*horizontal;
    }
    Write(output,0x4d910,horizontal);Write(output,0x4d914,vertical);
    Write(output,0x4d920,fov);Write(output,0x4d91c,vertical);
    Extra_Update(client,1);
    if(Read<int>(initial,0xc)){Extra_Special(initial);return;}
    const bool alternate=mode==8 && Read<Byte>(initial,0x64b00);
    Byte *view=Extra_State;
    const U32 w=alternate?Extra_AltWidth:Extra_ViewportWidth;
    const U32 h=alternate?Extra_AltHeight:Extra_ViewportHeight;
    Write(view,0x4d8f0,std::uint64_t(0));Write(view,0x4d8f8,w);Write(view,0x4d8fc,h);
    Write(view,0x646e0,Byte(0));Write(view,0x4d900,std::uint64_t(0));
    Write(view,0x4d908,float(w));Write(view,0x4d90c,float(h));
    if(Read<int>(initial,0x480ac)==6)CopyPlayerView(initial);
    else {
        view=Extra_State;
        const bool linked=Read<int>(view,0x6b818)!=0 ||
            (Extra_DemoMode==2 && Extra_PlaybackInited && (Extra_DemoFlags&2) &&
             Read<Byte>(Extra_Playback,0x26a998));
        if(linked && Read<int>(view,0x6b82c)){
            Extra_Linked(Read<int>(initial,0x6b82c),client);
            Extra_Update(client,1);
            return;
        }
        CopyPlayerView(initial);
        switch(mode){
        case 6:Extra_Mode6A(client);Extra_Mode6B(client);break;
        case 7:Extra_Mode7();break;
        case 8:{
            view=Extra_State;
            CopyVec(view,0x4d924,view,0x64a7c+12*Read<int>(view,0x64afc));
            CopyVec(view,0x649d0,view,0x64a7c+12*(Read<int>(view,0x64afc)+4));
            break;
        }
        default:Write(initial,0x4d92c,Read<float>(initial,0x482b0)+Read<float>(initial,0x4d92c));break;
        }
    }
    Extra_AnglesToAxis(reinterpret_cast<const float*>(initial+0x649d0),reinterpret_cast<float*>(initial+0x4d934));
}
}
#endif
