// Store MP CG_UpdateCameraMode, 1400C0E20..1400C1235.
// Exact mode dispatch and transition/sound boundaries from the Store body.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_camera_mode {
using Byte=unsigned char;
using U32=std::uint32_t;
extern "C" {
extern Byte *Mode_State; // 140E53A20
extern int Mode_Demo; // 1425A3308
extern Byte Mode_PlaybackInited; // 142653908
extern const Byte *Mode_Playback; // 142653900
extern Byte *Mode_EntityBase[]; // 140E4F8E0
extern const Byte *Mode_VehicleDefs[256]; // 14265D0B0
extern int Mode_ClientCount; // 140E5CA9C
extern Byte *Mode_ClientState; // 140E61BF8
extern const char *Mode_ErrorText; // 1408E2000
bool Mode_ErrorCheck(); // 1400A9960
void Mode_Error(int,const char*); // 14039AEB0
int Mode_Missile(int); // 14018E600
void Mode_Transition(int,int,int); // 1400BFCC0
void Mode_Sound(const char*,int); // 14056D110
void Mode_UpdateVehicle(int); // 1400C07D0
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float Neg(float v){U32 b;std::memcpy(&b,&v,4);b^=0x80000000u;std::memcpy(&v,&b,4);return v;}
static bool Vehicle(int m){return m==2||m==3||m==4;}
static unsigned EntityKind(int c,int n){
    if(Mode_ErrorCheck())Mode_Error(1,Mode_ErrorText);
    return R<Byte>(Mode_EntityBase[c]+n*0x3d8,0x286);
}
int UpdateCameraMode(int client){
    Byte *const cg=Mode_State;
    int mode=0;
    const int previous=R<int>(cg,0x64a30),previousKind=R<int>(cg,0x64a34);
    if(!(Mode_Demo==2 && Mode_PlaybackInited && R<int>(Mode_Playback,0xd4c00)==2)){
        const U32 flags=R<U32>(cg,0x481a4);
        if(flags&0x4000){
            W(cg,0x64a34,int(EntityKind(client,R<std::int16_t>(cg,0x4851c))));
            int seat=R<int>(cg,0x48520);
            if(U32(seat)-1u<4u)mode=4;
            else {
                const Byte *def=Mode_VehicleDefs[EntityKind(client,R<std::int16_t>(cg,0x4851c))];
                seat=R<int>(cg,0x48520);
                const unsigned type=R<unsigned short>(def,8);
                if((seat==0 && R<int>(def,0x30)!=0) ||
                   (R<int>(cg,0x480a0)==1 && !(type&0xfffa) && type!=5 &&
                    R<unsigned short>(def,0x28)==1))mode=3;
                else if(R<std::int16_t>(cg,0x4851c)!=0x3ff && !seat)mode=2;
            }
        }else if((flags&0x300) && R<std::int16_t>(cg,0x4851c)!=0x3ff){
            mode=5;W(cg,0x64b05,Byte((R<U32>(cg,0x481a8)>>28)&1));
        }else if(Mode_Missile(client) || (R<U32>(Mode_State,0x481a8)&0x40000))mode=6;
        else if(R<int>(cg,0x480ac)==1)mode=1;
    }
    const bool changed=mode!=previous;
    if(R<int>(cg,0x48248)!=R<int>(cg,0x64a08)){
        if(changed && !R<int>(cg,0x6b818) && !mode && Vehicle(previous) && !(R<Byte>(cg,0x480c0)&2)){
            Byte *cl=client<Mode_ClientCount?Mode_ClientState:nullptr;
            const float old=R<float>(cl,0x2ca4),value=Neg(R<float>(cg,0x48140));
            // Original preserves the smaller value via rounded differences, except
            // when value-old is nonnegative (then writes value directly).
            const float candidate=old-value>=0.0f?value:old;
            W(cl,0x2ca4,value-old>=0.0f?value:candidate);
        }
    }else if(R<int>(cg,0x6b818)==R<int>(cg,0x6b81c)){
        const Byte *next=R<const Byte*>(cg,0x30);
        if(!next || R<int>(next,0x1b0)==R<int>(R<const Byte*>(cg,0x28),0x1b0)){
            if(changed){
                if(!Vehicle(previous) && !Vehicle(mode))Mode_Transition(client,previous,mode);
            }else if(R<Byte>(cg,0x64b04) ||
                     ((R<U32>(cg,0x481a4)&0x4000) && R<int>(cg,0x64a38)!=R<int>(cg,0x48520)))
                Mode_Transition(client,previous,mode);
        }
    }
    if(changed || previousKind!=R<int>(cg,0x64a34)){
        W(Mode_State,0x6a3e0,int(Vehicle(mode)));
        const char *sound="no_gfutz";
        if(Vehicle(mode)||mode==6){
            const int n=R<std::int16_t>(Mode_State,0x4851c);
            if(n!=0x3ff){
                const Byte *def=Mode_VehicleDefs[EntityKind(client,n)];
                if(def){
                    const char *s=R<const char*>(def,0x2f0);
                    if(s&&*s)sound=s;
                }
            }
        }
        Mode_Sound(sound,1);
    }
    if(R<int>(cg,0x6a3e0))Mode_UpdateVehicle(client);
    W(cg,0x64a30,mode);W(cg,0x64a38,R<int>(cg,0x48520));
    W(cg,0x64b04,Byte(0));W(cg,0x64a08,R<int>(cg,0x48248));
    return mode;
}
}
#endif
