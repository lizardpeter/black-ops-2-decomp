// Store MP CG_CalcTurretViewValues, 14016BFB0..14016C374.
#define T6_SOURCE "cgame_mp/cg_view_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_turret_view {
using Byte=unsigned char;using U32=std::uint32_t;
extern "C" {
extern Byte *TurretView_State; // 140E53A20
extern Byte *TurretView_Entities[]; // 140E4F8E0
extern const std::int16_t *TurretView_ModelMap; // array 143FCD390
extern Byte *TurretView_Models; // array 143F85390
extern const Byte *TurretView_Weapons[256]; // 14265D3B0
extern unsigned short TurretView_PlayerTag; // 141EF6B18
extern int *TurretView_LastTime; // array 1444A059C
extern float *TurretView_Offset; // array 1444A0580
extern float *TurretView_Aux; // array 1444A0590
extern float *TurretView_PreviousAngles; // array 1444A05A0
extern const char *TurretView_ErrorText,*TurretView_TagErrorText;
bool TurretView_ErrorCheck(); // 1400A9960
void TurretView_Error(int,const char*); // 14039AEB0
const Byte *TurretView_Tag(Byte*,Byte*,unsigned); // 1400E6370
int TurretView_Random(); // IAT 1408A8330
void TurretView_ComputeOffset(const float*,U32,float*,float*,float*,float,int); // 1400C23C0
void TurretView_AnglesToAxis(const float*,float*); // 140464D00
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static int Diff(int a,int b){U32 v=U32(a)-U32(b);int n;std::memcpy(&n,&v,4);return n;}
static float Neg(float f){U32 v;std::memcpy(&v,&f,4);v^=0x80000000u;std::memcpy(&f,&v,4);return f;}
void CalcTurretViewValues(int client){
    Byte *cg=TurretView_State;
    if(!(R<U32>(cg,0x481a4)&0x300))return;
    const int entityIndex=R<std::int16_t>(cg,0x4851c);
    if(TurretView_ErrorCheck())TurretView_Error(1,TurretView_ErrorText);
    Byte *entity=TurretView_Entities[client]+entityIndex*0x3d8;
    const int model=TurretView_ModelMap[client*0x701+R<int>(entity,0x1f4)];
    if(!model)return;
    Byte *obj=TurretView_Models+model*0x90;
    if(!obj)return;
    const Byte *tag=TurretView_Tag(entity,obj,TurretView_PlayerTag);
    if(tag){
        for(unsigned i=0;i<3;++i)W(cg,0x4d924+4*i,R<U32>(tag,0x10+4*i));
    }else TurretView_Error(1,TurretView_TagErrorText);
    const Byte *props=R<const Byte*>(TurretView_Weapons[R<Byte>(entity,0x2d8)],0x10);
    const int third=R<int>(cg,0x480a0);
    if(!third){
        for(unsigned i=0;i<3;++i)W(cg,0x4d924+4*i,R<float>(props,0x4a8+4*i)+R<float>(cg,0x4d924+4*i));
    }
    if(R<int>(cg,0x48518)==2&&!third){
        const float pitch=(float(TurretView_Random())*(1.0f/16384.0f)-1.0f)*R<float>(props,0xb2c);
        W(cg,0x649d0,pitch+R<float>(cg,0x649d0));
        const float yaw=(float(TurretView_Random())*(1.0f/16384.0f)-1.0f)*R<float>(props,0xb28);
        W(cg,0x649d4,yaw+R<float>(cg,0x649d4));
    }
    int previous=TurretView_LastTime[client],now=R<int>(cg,0x4808c);
    float *history=TurretView_PreviousAngles+client*3;
    const float *angles=reinterpret_cast<const float*>(cg+0x482a0);
    if(previous>now||previous<Diff(now,300)){
        std::memcpy(history,angles,12);
        previous=now=R<int>(cg,0x4808c);
    }
    float *offset=TurretView_Offset+client*3;
    TurretView_ComputeOffset(angles,R<U32>(entity,0x2d8),history,offset,TurretView_Aux+client*3,2.0f,Diff(now,previous));
    TurretView_LastTime[client]=R<int>(cg,0x4808c);
    float axis[9];
    TurretView_AnglesToAxis(reinterpret_cast<const float*>(cg+0x649d0),axis);
    for(unsigned row=0;row<3;++row){
        const float scale=row==2?Neg(offset[row]):offset[row];
        for(unsigned col=0;col<3;++col)
            W(cg,0x4d924+4*col,axis[row*3+col]*scale+R<float>(cg,0x4d924+4*col));
    }
}
}
#endif
