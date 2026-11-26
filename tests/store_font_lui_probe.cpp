// Differential replay of original Store LUI utility routines.
#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/ui/lui_text_util_store.cpp"
#include <cstdlib>
#define MS __attribute__((ms_abi))
namespace store_lui_text{
extern "C"{
float Lui_ColorTable[8][4],Lui_Color8[4],Lui_Color9[4];
int Lui_TransformDepth;float Lui_TransformStack[4][16];const Byte*Lui_Language;
float Lui_AlignFactors[4];int Lui_Layer;
int Lui_Atoi(const char*s){return std::atoi(s);}
void Lui_PushRegistry(void*,int){}
void*Lui_ToUserData(void*,int){return nullptr;}
void Lui_Unref(void*,int,int){}
void Lui_AddQuadArray(const void*,short,const void*,const void*,const void*,int){}
}
static int MS MAtoi(const char*s){return Lui_Atoi(s);}
struct Capture{U32 ch,advance;float values[12];};
static Byte elem[0x80],state[0x250],font[64],dvar[48];
extern "C"{
void*LuiMock(){return reinterpret_cast<void*>(MAtoi);}
unsigned LuiSize(){return sizeof(Capture);}
void LuiRun(void*image,unsigned op,unsigned seed,void*out){
    auto*b=static_cast<Byte*>(image);Capture cap{};
    unsigned st=seed+1;auto next=[&](){st=st*1664525u+1013904223u;return st;};
    auto f=[&](){return float(int(next()%2048)-1024)*0.125f;};
    if(op==0){
        Byte raw[4]={Byte(next()>>24),Byte(next()>>24),Byte(next()>>24),0};
        const Byte*p=raw;
        if(b)cap.ch=reinterpret_cast<U32(MS*)(const Byte**)>(b+0x3e9880)(&p);
        else cap.ch=DecodeCharacter(&p);
        cap.advance=unsigned(p-raw);
    }else if(op==1){
        for(auto&c:Lui_ColorTable)for(auto&v:c)v=f();
        for(auto&v:Lui_Color8)v=f();
        for(auto&v:Lui_Color9)v=f();
        Byte c=Byte(next()>>24);
        if(b){std::memcpy(b+0x8e6870,Lui_ColorTable,sizeof(Lui_ColorTable));std::memcpy(b+0x3c11c00,Lui_Color8,16);std::memcpy(b+0x3c11c10,Lui_Color9,16);reinterpret_cast<void(MS*)(Byte,float*)>(b+0x19e3b0)(c,cap.values);}
        else ColorForCode(c,cap.values);
    }else if(op==2||op==3){
        Lui_TransformDepth=int(next()%4);
        for(auto&m:Lui_TransformStack)for(auto&v:m)v=f();
        float x=f(),y=f(),z=f(),w=f();
        if(b){W(b,0x43c2bd4,Lui_TransformDepth);std::memcpy(b+0x43c4cf0,Lui_TransformStack,sizeof(Lui_TransformStack));}
        if(op==2){
            if(b)reinterpret_cast<void(MS*)(float*,float*,float*)>(b+0x6f24d0)(&x,&y,&z);
            else TransformPoint(&x,&y,&z);
            cap.values[0]=x;cap.values[1]=y;cap.values[2]=z;
        }else{
            if(b)reinterpret_cast<void(MS*)(float,float,float,float,float*)>(b+0x6aefd0)(x,y,z,w,cap.values);
            else RectVertices(x,y,z,w,cap.values);
        }
    }else if(op==4){
        W(font,8,int(next()%96)-20);std::memset(dvar,0,sizeof(dvar));Lui_Language=dvar;
        const unsigned v=next()%17;switch(seed%6){
        case 0:Lui_Language=nullptr;break;
        case 1:W(dvar,0x18,1);W(dvar,0x20,Byte(v&1));break;
        case 2:W(dvar,0x18,2);W(dvar,0x20,float(v)+0.5f);break;
        case 3:W(dvar,0x18,6);W(dvar,0x20,v);break;
        case 4:W(dvar,0x18,7);W(dvar,0x20,v);break;
        default:W(dvar,0x18,8);W(dvar,0x20,v&1?"11":"13");break;
        }
        if(b){W(b,0x2121178,Lui_Language);cap.values[0]=reinterpret_cast<float(MS*)(const Byte*)>(b+0x6ae330)(font);}
        else cap.values[0]=FontHeight(font);
    }else{
        W(elem,0x62,short(int(next()%300)-50));W(elem,0x30,int(next()%400));
        W(state,0x240,int(next()%600)-50);W(state,4,f());elem[0x72]=Byte(next());elem[0x76]=Byte(next());
        for(auto&v:Lui_AlignFactors)v=f();
        float height=f();
        if(b){std::memcpy(b+0x8e3498,Lui_AlignFactors,16);cap.values[0]=reinterpret_cast<float(MS*)(const Byte*,const Byte*,float)>(b+0xf3370)(elem,state,height);}
        else cap.values[0]=AnimatedY(elem,state,height);
    }
    std::memcpy(out,&cap,sizeof(cap));
}
}
}
