// Store MP CG_OffsetThirdPersonView 14016A940..14016B04B.
// Original camera collision and model tag boundaries are retained.
#define T6_SOURCE "cgame_mp/cg_view_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_third_camera {
using Byte=unsigned char; using U32=std::uint32_t;
struct Dvar;
extern "C" {
extern Byte *Third_State;
extern Byte *Third_Entities[];
extern const std::int16_t *Third_ModelMap; // 143FCD390
extern Byte *Third_Models; // 143F85390, stride 0x90
extern unsigned short Third_PlayerTag,Third_DeathTag; // 141EF6B8A,141EF6B84
extern int Third_DemoMode;
extern Byte Third_PlaybackInited;
extern const Byte *Third_Playback;
extern const Dvar *Third_Mode,*Third_Angle,*Third_Range,*Third_AboveWater;
extern const char *Third_ErrorText;
Byte *Third_Tag(Byte*,Byte*,unsigned); // 1400E6370
void Third_TagAxis(Byte*,float*); // 1400DE970
bool Third_ErrorCheck(); // 1400A9960
void Third_Error(int,const char*); // 14039AEB0
void Third_Chase(int); // 14016A5D0
int Third_Atoi(const char*);
double Third_Atof(const char*);
__m128 Third_SinCos(float,float); // 140890DA3
void Third_Trace(Byte*,float*,const float*,int,float*,Byte,Byte); // 1400C69F0
float Third_WaterHeight(float*,float,float); // 14038BAD0
float Third_Atan2(float,float); // 140870BBC
void Third_AnglesToAxis(const float*,float*); // 140464D00
}
template<class T> static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T> static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 b){float v;std::memcpy(&v,&b,4);return v;}
static float Neg(float v){U32 b;std::memcpy(&b,&v,4);return F(b^0x80000000u);}
static float Sqrt(float x){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));}
static float GetFloat(const Dvar*d){
    if(!d)return 0;
    const int type=R<int>(d,0x18);
    if(type==2)return R<float>(d,0x20);
    if(type==6)return float(R<int>(d,0x20));
    return float(Third_Atof(R<const char*>(d,0x20)));
}
static int GetInt(const Dvar*d){
    if(!d)return 0;
    const int type=R<int>(d,0x18);
    if(type==6||type==7)return R<int>(d,0x20);
    if(type==1)return R<Byte>(d,0x20)!=0;
    if(type==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Third_Atoi(R<const char*>(d,0x20));
}
static Byte *Model(int client,int index){
    const int n=Third_ModelMap[client*0x701+index];
    if(!n)return nullptr;
    return Third_Models+n*0x90;
}
static void CopyOrigin(Byte*cg,const Byte*tag){
    for(unsigned i=0;i<3;++i)W(cg,0x4d924+4*i,R<U32>(tag,0x10+4*i));
}
void OffsetThirdPersonView(int client){
    Byte *cg=Third_State;
    if(R<Byte>(cg,0x4852c)&4)return;
    if(!(R<U32>(cg,0x481a4)&0x300)){
        Byte *model=Model(client,R<int>(cg,0x48248));
        if(!model)return;
        Byte *tag=Third_Tag(cg+0x4aab8,model,Third_PlayerTag);
        if(!tag)return;
        float axis[9];Third_TagAxis(tag,axis);
        CopyOrigin(cg,tag);
    }
    if(R<int>(cg,0x480ac)>=10 && U32(R<int>(cg,0x4819c))<=3){
        if(Third_ErrorCheck())Third_Error(1,Third_ErrorText);
        Byte *entity=Third_Entities[client]+(R<int>(cg,0x4819c)+18)*0x3d8;
        if(R<Byte>(entity,0x3cc)&2){
            Byte *model=Model(client,R<int>(entity,0x1f4));
            if(model){
                Byte *tag=Third_Tag(entity,model,Third_DeathTag);
                if(tag)CopyOrigin(cg,tag);
                if(R<unsigned short>(entity,0x2ce)!=0x3ff){
                    const float px=R<float>(cg,0x480d0),py=R<float>(cg,0x480d4);
                    const float x=R<float>(cg,0x4d924)-px,y=R<float>(cg,0x4d928)-py;
                    if(y*y+x*x>225.0f){
                        float length=Sqrt(y*y+x*x);
                        if(!(length>0))length=1;
                        const float inv=1.0f/length;
                        W(cg,0x4d924,(inv*x)*15.0f+px);
                        W(cg,0x4d928,(inv*y)*15.0f+py);
                    }
                    const float pz=R<float>(cg,0x480d8);
                    float z=R<float>(cg,0x4d92c);
                    if(!(z>=pz)){z=pz;W(cg,0x4d92c,z);}
                    if(z>pz+60.0f)W(cg,0x4d92c,pz+60.0f);
                }
            }
        }
    }
    if((R<Byte>(cg,0x480c0)&2) ||
       (Third_DemoMode==2 && Third_PlaybackInited && R<int>(Third_Playback,0xd4c00)==1)){
        Third_Chase(client);return;
    }
    float pitch=R<float>(cg,0x649d0),yaw=R<float>(cg,0x649d4),roll=R<float>(cg,0x649d8);
    if(GetInt(Third_Mode)==2)pitch=0.0f;
    if(R<int>(cg,0x480ac)>=10)yaw=float(R<int>(cg,0x482e4));
    float p=pitch;
    if(!(p<=45.0f))p=45.0f;
    alignas(16) float t[4];
    _mm_store_ps(t,Third_SinCos(yaw*F(0x3c8efa35),p*F(0x3c8efa35)));
    float *origin=reinterpret_cast<float*>(cg+0x4d924);
    const float x=origin[0],y=origin[1],z=origin[2];
    const float focus[3]={(t[1]*t[3])*512.0f+x,(t[0]*t[3])*512.0f+y,z-t[2]*512.0f};
    const float angle=yaw-GetFloat(Third_Angle);
    _mm_store_ps(t,Third_SinCos(angle*F(0x3c8efa35),pitch*F(0x3c0efa35)));
    const float distance=Neg(GetFloat(Third_Range));
    const float target[3]={distance*(t[1]*t[3])+x,y+distance*(t[0]*t[3]),(distance*Neg(t[2])+z)+8.0f};
    Third_Trace(cg,origin,target,0x810811,origin,1,1);
    if(R<int>(cg,0x480ac)>=10 && GetFloat(Third_AboveWater)>0){
        const float above=GetFloat(Third_AboveWater);
        const float water=Third_WaterHeight(origin,200.0f,-200.0f);
        if(water> -131072.0f && water+above>origin[2])origin[2]=water+above;
    }
    const float dx=focus[0]-origin[0],dy=focus[1]-origin[1],dz=focus[2]-origin[2];
    float horizontal=Sqrt(dy*dy+dx*dx);
    // MAXSS selects the second operand on unordered input.
    if(!(horizontal>1.0f))horizontal=1.0f;
    const float angles[3]={Third_Atan2(dz,horizontal)*F(0xc2652ee0),angle,roll};
    Third_AnglesToAxis(angles,reinterpret_cast<float*>(cg+0x4d934));
}
}
#endif
