// Store MP CG_UpdateCameraTransition 1400BFCC0..1400C0040.
// Switch targets are corroborated by the named reference build and
// checked against the Store RVA jump table in the replay.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_camera_transition {
using Byte=unsigned char;using U32=std::uint32_t;
struct Dvar;
struct alignas(16) Value {U32 words[4];};
extern "C" {
extern Byte *Transition_State; // 140E53A20
extern Byte *Transition_ClientState; // 140E61BF8
extern int Transition_ClientCount; // 140E5CA9C
extern const Dvar *Transition_ExitTime; // 140D78630
extern const Dvar *Transition_Highlight; // 140D78620
extern const Byte *Transition_VehicleDefs[256]; // 14265D0B0
double Transition_Atof(const char*); // IAT 1408A7DD8
int Transition_Missile(int); // 14018E600
void Transition_SetFloat(const Dvar*,float,int); // 14046CEF0
void Transition_SetVariant(const Dvar*,const Value*,int); // 14046B300
void Transition_StartTween(float); // 1400BF890
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float GetFloat(const Dvar*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(t==2)return R<float>(d,0x20);
    if(t==6)return float(R<int>(d,0x20));
    return float(Transition_Atof(R<const char*>(d,0x20)));
}
static bool Vehicle(int m){return U32(m)-2u<=2;}
static void Copy(Byte*cg,unsigned to,unsigned from){
    for(unsigned i=0;i<3;++i)W(cg,to+4*i,R<U32>(cg,from+4*i));
}
static void ClientAngles(Byte*cg,int client){
    Byte*cl=client<Transition_ClientCount?Transition_ClientState:nullptr;
    for(unsigned i=0;i<3;++i)W(cl,0x2ca4+4*i,R<float>(cg,0x482a0+4*i)-R<float>(cg,0x48140+4*i));
}
void UpdateCameraTransition(int client,int oldMode,int newMode){
    const float exitTime=GetFloat(Transition_ExitTime);
    Byte*cg=Transition_State;
    float tween=0;
    switch(newMode){
    case 0:
        if(Vehicle(oldMode)&&R<int>(Transition_VehicleDefs[R<int>(cg,0x64a34)],0xc))break;
        if(oldMode==5){
            if(R<Byte>(cg,0x64b05))W(cg,0x64b05,Byte(0));
            else tween=0.4f;
        }else if(Vehicle(oldMode))tween=exitTime;
        break;
    case 1:
        if(R<U32>(cg,0x481a4)&0x4000){
            if(R<int>(cg,0x48520)!=R<int>(cg,0x64a38))tween=0.3f;
        }else if(Vehicle(oldMode))tween=exitTime;
        break;
    case 2:if(!(R<Byte>(cg,0x480c0)&2))tween=exitTime;break;
    case 3:tween=exitTime;break;
    case 4:
        if(U32(oldMode)<=2)tween=0.3f;
        if((oldMode==2||oldMode==4)&&R<int>(cg,0x48520)!=R<int>(cg,0x64a38))tween=0.5f;
        break;
    case 6:
        W(cg,0x64a48,Byte(R<int>(cg,0x6b818)==1));
        if(Transition_Missile(client)){
            W(cg,0x6a418,1);Transition_SetFloat(Transition_Highlight,3000.0f,0);
        }
        if(!R<int>(cg,0x6b818))Copy(cg,0x64a3c,0x482a0);
        break;
    }
    if(R<Byte>(cg,0x64b04)&&!(tween<0.0f||tween>0.0f)){
        W(cg,0x482a0,R<U32>(cg,0x649e8));W(cg,0x482a4,R<U32>(cg,0x649ec));
        W(cg,0x482a8,U32(0));ClientAngles(cg,client);
    }else if(tween>0)Transition_StartTween(tween);
    if(oldMode==6){
        if(!R<Byte>(cg,0x64a48)){Copy(cg,0x482a0,0x64a3c);ClientAngles(cg,client);}
        Value value;std::memcpy(&value,reinterpret_cast<const Byte*>(Transition_Highlight)+0x40,16);
        Transition_SetVariant(Transition_Highlight,&value,0);
    }else if(oldMode==4&&!(R<Byte>(cg,0x480c0)&2)){
        W(cg,0x482a8,U32(0));ClientAngles(cg,client);
    }
}
}
#endif
