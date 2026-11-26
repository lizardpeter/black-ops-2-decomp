// Store MP LUI text/geometry utility bodies, from the original renderer closure.
#define T6_SOURCE "lui/lui_render.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_lui_text {
using Byte=unsigned char;using U32=std::uint32_t;using U64=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
extern "C"{
extern float Lui_ColorTable[8][4],Lui_Color8[4],Lui_Color9[4];
extern int Lui_TransformDepth;
extern float Lui_TransformStack[][16];
extern const Byte*Lui_Language;
int Lui_Atoi(const char*);
extern float Lui_AlignFactors[4];
extern int Lui_Layer;
void Lui_PushRegistry(void*,int);
void*Lui_ToUserData(void*,int);
void Lui_Unref(void*,int,int);
void Lui_AddQuadArray(const void*,short,const void*,const void*,const void*,int);
}
// 1403E9880: public UTF-8 decoder with original two/three-byte rules.
U32 DecodeCharacter(const Byte**cursor){
    const Byte*p=*cursor;U32 c=p[0];unsigned len=1;
    if((c&0xe0)==0xc0){c=((c&31)<<6)|(p[1]&63);len=2;}
    else if((c&0xf0)==0xe0){c=((c&15)<<12)|((p[1]&63)<<6)|(p[2]&63);len=3;}
    *cursor=p+len;return c;
}
// 14019E3B0: unpacked color escape palette.
void ColorForCode(Byte c,float*out){
    unsigned i=Byte(c-'0');
    if(i==8)std::memcpy(out,Lui_Color8,16);
    else if(i==9)std::memcpy(out,Lui_Color9,16);
    else std::memcpy(out,Lui_ColorTable[i<8?i:7],16);
}
// 1406F24D0: affine transform with original SSE operation ordering.
void TransformPoint(float*x,float*y,float*z){
    if(!Lui_TransformDepth)return;
    const float*m=Lui_TransformStack[Lui_TransformDepth];
    const float a=*x,b=*y,c=*z;
    *x=(b*m[1]+a*m[0])+(c*m[2]+m[3]);
    *y=(b*m[5]+a*m[4])+(c*m[6]+m[7]);
    *z=(b*m[9]+a*m[8])+(c*m[10]+m[11]);
}
// 1406AEFD0: generate a transformed four-point LUI rectangle.
void RectVertices(float x0,float y0,float x1,float y1,float*out){
    const float v[12]={x0,y0,0,x1,y0,0,x0,y1,0,x1,y1,0};
    std::memcpy(out,v,sizeof(v));
    for(unsigned i=0;i<12;i+=3)TransformPoint(out+i,out+i+1,out+i+2);
}
static U32 IntDvar(const Byte*d){
    if(!d)return 0;
    int type=R<int>(d,0x18);
    if(unsigned(type-6)<2)return R<U32>(d,0x20);
    if(type==1)return R<Byte>(d,0x20)!=0;
    if(type==2)return U32(int(R<float>(d,0x20)));
    return U32(Lui_Atoi(R<const char*>(d,0x20)));
}
// 1406AE330: original language-specific nominal font height.
float FontHeight(const Byte*font){
    const int h=R<int>(font,8);
    if(Lui_Language&&(IntDvar(Lui_Language)==11||IntDvar(Lui_Language)==13))return 17.0f;
    return float(h);
}
// 1400F3370: HUD vertical alignment interpolation, including early/late timing.
float AnimatedY(const Byte*elem,const Byte*state,float height){
    const short duration=R<short>(elem,0x62);
    float y=R<float>(state,4);
    const float dest=y-height*Lui_AlignFactors[elem[0x72]&3];
    if(duration>0){
        int elapsed=R<int>(state,0x240)-R<int>(elem,0x30);
        if(elapsed<duration){
            float frac=elapsed<1?0.0f:float(elapsed)/float(duration);
            if(frac!=1.0f){float start=y-height*Lui_AlignFactors[elem[0x76]&3];return (start-dest)*frac+start;}
        }
    }
    return dest;
}
// 1406AEF40: walk original Lua registry chain and submit each positive quad count.
void SubmitQuadChain(void*lua,Byte*node){
    const int layer=Lui_Layer;
    while(node){
        short n=R<short>(node,0x20);
        if(n>0)Lui_AddQuadArray(R<const void*>(node,0x18),n,R<const void*>(node,0),R<const void*>(node,8),R<const void*>(node,0x10),layer);
        if(R<int>(node,0x24)==-2)break;
        Lui_PushRegistry(lua,-10000);node=static_cast<Byte*>(Lui_ToUserData(lua,-1));
        W(lua,0x48,R<Byte*>(lua,0x48)-16);
    }
}
// 1406AA560: release registry references in the original linked cache.
void ReleaseQuadChain(void*lua,Byte*node){
    if(!node||R<int>(node,0x24)==-2)return;
    Lui_PushRegistry(lua,-10000);
    Byte*next=static_cast<Byte*>(Lui_ToUserData(lua,-1));W(lua,0x48,R<Byte*>(lua,0x48)-16);
    while(next){
        Lui_Unref(lua,-10000,R<int>(node,0x24));W(node,0x24,-2);
        if(R<int>(next,0x24)==-2)return;
        Lui_PushRegistry(lua,-10000);node=next;next=static_cast<Byte*>(Lui_ToUserData(lua,-1));W(lua,0x48,R<Byte*>(lua,0x48)-16);
    }
}
}
#endif
