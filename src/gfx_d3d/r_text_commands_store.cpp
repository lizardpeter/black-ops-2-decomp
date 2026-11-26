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
Byte* AddBaseDrawTextCmd(const char*text,int maxChars,const void*font,float x,float y,float w,
                         float sx,float sy,float rotation,const float*color,int style,int cursor,
                         Byte cursorLetter,float padding){
    if(!*text&&cursor<0)return nullptr;
    int len=0;while(text[len])++len;
    const int bytes=(len+0x78)&~3;
    auto*c=Text_CommandList;
    if(bytes>c->byteSize+c->usedCritical-c->usedTotal-0x2000){c->lastCmd=nullptr;return nullptr;}
    Byte*out=c->cmds+c->usedTotal;
    c->usedTotal+=bytes;c->lastCmd=out;
    out[2]=0x11;out[3]=0;
    if(Text_UI3DDepth){
        const int i=Text_UI3DStack[Text_UI3DDepth];
        if(i>=0)out[3]=static_cast<Byte>(i|0x80);
        if(static_cast<unsigned>(i)<=5){
            auto*p=Text_UI3DStats+i*0xa4;
            W(p,0,R<unsigned>(p,0)+1);W(p,4,R<unsigned>(p,4)+1);
        }
    }
    W(out,0,static_cast<std::uint16_t>(bytes));W(out,4,0);
    W(out,8,x);W(out,0xc,y);W(out,0x10,w);W(out,0x18,rotation);
    W(out,0x20,font);W(out,0x28,sx);W(out,0x2c,sy);
    if(Text_DualPlay){
        const bool active=R<int>(Text_DualPlay,0x18)==1?R<Byte>(Text_DualPlay,0x20)!=0:
            Text_Atoi(R<const char*>(Text_DualPlay,0x20))!=0;
        if(active)W(out,0x2c,sy*0.5f);
    }
    Text_Color(color,out+0x30);
    W(out,0x34,maxChars);
    unsigned flags=style==3?4u:style==6?12u:style==0x80?1u:0u;
    W(out,0x38,flags);
    if(padding>0.0f){flags|=0x80;W(out,0x70,padding);W(out,0x38,flags);}
    if(cursor>-1){flags|=2;W(out,0x38,flags);W(out,0x40,cursorLetter);W(out,0x3c,cursor);}
    std::memcpy(out+0x74,text,static_cast<unsigned>(len));out[0x74+len]=0;
    return out;
}
// 14071D0D0: original 0x0f, 56-byte untextured quad command.
void AddCmdDrawQuad(const float*points,float z,const float*color,const void*material){
    auto*c=Text_CommandList;
    if(c->byteSize+c->usedCritical-c->usedTotal-0x2000<0x38){c->lastCmd=nullptr;return;}
    Byte*out=c->cmds+c->usedTotal;c->usedTotal+=0x38;c->lastCmd=out;
    out[2]=0xf;out[3]=0;
    if(Text_UI3DDepth){
        const int i=Text_UI3DStack[Text_UI3DDepth];
        if(i>=0)out[3]=Byte(i|0x80);
        if(static_cast<unsigned>(i)<6){auto*p=Text_UI3DStats+i*0xa4;W(p,0,R<unsigned>(p,0)+1);W(p,4,R<unsigned>(p,4)+1);}
    }
    W(out,0,std::uint16_t(0x38));W(out,8,material?material:Text_DefaultMaterial);
    std::memcpy(out+16,points,32);W(out,0x30,z);Text_Color(color,out+0x34);
}
}
#endif
