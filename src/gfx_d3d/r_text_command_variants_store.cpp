// Store MP AddBaseDrawTextCmd, 14071C880. Original command id 0x11.
#define T6_SOURCE "gfx_d3d/r_rendercmds.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_text_cmd {
using Byte=unsigned char;
struct CommandList {Byte*cmds;int usedTotal,usedCritical;Byte*lastCmd;int byteSize;};
extern "C" {
extern CommandList*Text_CommandList;
extern int Text_UI3DDepth;
extern int Text_UI3DStack[];
extern Byte Text_UI3DStats[];
extern const Byte*Text_DualPlay;
int Text_Atoi(const char*);
void Text_Color(const float*,Byte*);
extern const void*Text_DefaultMaterial;
}
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
Byte* AddBaseDrawTextCmd(const char*,int,const void*,float,float,float,float,float,float,const float*,int,int,Byte,float);
// Shared allocator semantics inlined at each original command producer.
static Byte* AllocateCommand(int bytes,Byte id,bool critical,bool draw){
    auto*c=Text_CommandList;
    int available=c->byteSize-c->usedTotal;
    if(!critical)available+=c->usedCritical-0x2000;
    if(bytes>available){c->lastCmd=nullptr;return nullptr;}
    Byte*out=c->cmds+c->usedTotal;
    c->usedTotal+=bytes;if(critical)c->usedCritical+=bytes;c->lastCmd=out;
    W(out,0,std::uint16_t(bytes));out[2]=id;out[3]=0;
    if(Text_UI3DDepth){
        int i=Text_UI3DStack[Text_UI3DDepth];
        if(i>=0)out[3]=Byte(i|0x80);
        if(static_cast<unsigned>(i)<6){
            Byte*p=Text_UI3DStats+i*0xa4;
            W(p,0,R<unsigned>(p,0)+1);
            if(draw)W(p,4,R<unsigned>(p,4)+1);
        }
    }
    return out;
}
// 14071BF10
void AddCmdEndOfList(){AllocateCommand(4,0,true,false);}
// 14071DFB0
void AddCmdUI3DMarker(){if(auto*p=AllocateCommand(8,0x1e,false,false))W(p,4,0);}
// 14071CAA0, 14071CC10, 14071CD70: three distinct effect command entrypoints.
static void ApplyGlow(Byte*p,const float*color,const float*glow){
    if(glow[3]>0.0f||glow[3]<0.0f){
        W(p,0x38,R<unsigned>(p,0x38)|0x30u);
        float c[]={glow[0]*0.1f,glow[1]*0.1f,glow[2]*0.1f,color[3]*glow[3]};
        Text_Color(c,p+0x44);
    }
}
static void ApplyEffect(Byte*p,unsigned flag,const void*material,const void*glowMaterial,int birth,int letter,int decayStart,int duration){
    W(p,0x38,R<unsigned>(p,0x38)|flag);W(p,0x60,material);W(p,0x68,glowMaterial);
    W(p,0x48,birth);W(p,0x4c,letter);W(p,0x50,decayStart);W(p,0x54,duration);W(p,0x70,0.0f);
}
void AddCmdDrawTextEffects(const char*text,int maxChars,const void*font,float x,float y,float w,float sx,float sy,float rotation,const float*color,int style,const float*glow,const void*material,const void*glowMaterial,int birth,int letter,int decayStart,int duration){
    Byte*p=AddBaseDrawTextCmd(text,maxChars,font,x,y,w,sx,sy,rotation,color,style,-1,0,0);
    if(!p)return;ApplyGlow(p,color,glow);
    if(material&&glowMaterial&&birth)ApplyEffect(p,0x880u,material,glowMaterial,birth,letter,decayStart,duration);
}
void AddCmdDrawTextDecode(const char*text,int maxChars,const void*font,float x,float y,float sx,float sy,const void*unused,const float*color,int style,const float*glow,const void*material,const void*glowMaterial,int birth,int letter,int decayStart,int duration){
    (void)unused;
    Byte*p=AddBaseDrawTextCmd(text,maxChars,font,x,y,1,sx,sy,0,color,style,-1,0,0);
    if(!p)return;ApplyGlow(p,color,glow);
    if(birth)ApplyEffect(p,0x1000u,material,glowMaterial,birth,letter,decayStart,duration);
}
void AddCmdDrawTextRedact(const char*text,int unusedLength,const void*font,float x,float y,float sx,float sy,const void*unused,const float*color,int style,const float*glow,const void*material,const void*glowMaterial,int birth,int letter,int decayStart,int duration,int redactStart,int redactDuration){
    (void)unusedLength;(void)unused;
    Byte*p=AddBaseDrawTextCmd(text,0x7fffffff,font,x,y,1,sx,sy,0,color,style,-1,0,0);
    if(!p)return;ApplyGlow(p,color,glow);
    if(birth){ApplyEffect(p,0x4000u,material,glowMaterial,birth,letter,decayStart,duration);W(p,0x58,redactStart);W(p,0x5c,redactDuration);}
}
// 14071C5F0: original material fallback predicate and stretch command.
void AddCmdStretchPic(float x,float y,float w,float h,float s0,float t0,float s1,float t1,float z,const float*color,const void*material){
    const void*m=material?material:Text_DefaultMaterial;
    const void*tech=R<const void*>(m,0x60);
    const void*def=Text_DefaultMaterial;
    bool keep=false;
    if(R<const void*>(tech,0x30)||R<const void*>(tech,0x28)){
        auto*a=R<const int*>(m,0x68);auto*b=R<const int*>(def,0x68);
        keep=a!=b||(*a!=*b&&(R<const void*>(m,0x70)!=R<const void*>(def,0x70)||tech!=R<const void*>(def,0x60)));
    }
    if(!keep&&(R<Byte>(m,0x57)&0x10))m=def;
    if(auto*p=AllocateCommand(0x38,0xb,false,true)){
        W(p,8,m);W(p,0x10,x);W(p,0x14,y);W(p,0x18,w);W(p,0x1c,h);
        W(p,0x20,s0);W(p,0x24,t0);W(p,0x28,s1);W(p,0x2c,t1);W(p,0x30,z);Text_Color(color,p+0x34);
    }
}
// 14071C740: rotated stretch command; angle encoding remains a separate original callee.
extern "C" unsigned Text_EncodeAngle(float);
void AddCmdStretchPicRotate(float x,float y,float unused,float h,float s0,float t0,float s1,float t1,float z,float angle,const float*color,const void*material){
    (void)unused;
    if(auto*p=AllocateCommand(0x40,0xd,false,true)){
        W(p,8,material?material:Text_DefaultMaterial);W(p,0x10,x);W(p,0x14,y);W(p,0x18,1.0f);W(p,0x1c,h);
        W(p,0x20,s0);W(p,0x24,t0);W(p,0x28,s1);W(p,0x2c,t1);W(p,0x30,z);Text_Color(color,p+0x34);W(p,0x38,Text_EncodeAngle(angle));
    }
}
// 14071DD30: LUI variable-size quad array.
void AddCmdUIQuadArray(const void*material,short count,const void*vertices,const void*uv,const void*colors,int layer){
    if(Text_UI3DDepth&&Text_UI3DStack[Text_UI3DDepth]>=0&&layer<0)return;
    const int n=count,uvOffset=n*0x30+0x18,colorOffset=n*0x20+uvOffset,bytes=n*4+colorOffset;
    if(auto*p=AllocateCommand(bytes,0x17,false,true)){
        W(p,8,material?material:Text_DefaultMaterial);W(p,0x10,count);
        unsigned magnitude=layer<0?0u-unsigned(layer):unsigned(layer);
        W(p,0x12,Byte(magnitude>255?255:magnitude));
        p[0x13]=Byte((p[0x13]&0xfc)|(unsigned(layer)>>31));
        std::memcpy(p+0x18,vertices,n*0x30);std::memcpy(p+uvOffset,uv,n*0x20);std::memcpy(p+colorOffset,colors,n*4);
    }
}
}
#endif
