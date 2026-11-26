// Store MP text effect helpers. Explicit original material dependencies.
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_font_geom {void StretchPicRotate(const unsigned char*,float,float,float,float,float,float,float,float,float,float,float,std::uint32_t);}
namespace store_font_icon {void GetFontIconEntryInfo(int,const char*,int,void**,float*,float*);}
namespace store_font_effect {
using Byte=unsigned char;using U32=std::uint32_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
extern "C" {
extern const Byte*Effect_DefaultMaterial;
extern const char*Effect_DefaultName;
const Byte*Effect_RegisterMaterial(const char*,int,bool,int);
}
using store_font_geom::StretchPicRotate;
// 1403E9910: count printable characters, excluding newline and numeric color codes.
int PrintLength(const Byte*p){
    if(!p)return 0;
    int n=0;
    while(*p){
        U32 ch=*p++;
        if((ch&0xe0)==0xc0){ch=((ch&31)<<6)|(*p++&63);}
        else if((ch&0xf0)==0xe0){ch=((ch&15)<<12)|((p[0]&63)<<6)|(p[1]&63);p+=2;}
        if(ch=='^'&&Byte(*p-'0')<=9)++p;
        else if(ch!=10&&ch!=13)++n;
    }
    return n;
}
// 1407323B0: one of sixteen atlas slices, signed remainder retained.
void DrawEffectSlice(const Byte*mat,int index,float x,float y,float z,float width,float height,float sn,float cs,U32 color){
    const float s=float(index%16)*0.0625f;
    StretchPicRotate(mat,x,y,z,width,height,s,0,s+0.0625f,1,sn,cs,color);
}
// 1407320E0: named font icon and its actual drawn advance.
float DrawNamedIcon(const char*tag,float x,float y,float sn,float cs,const Byte*font,float sx,float sy,U32 color,int*consumed){
    char name[64];int i=1;
    while(i<64&&tag[i]!='^'){name[i-1]=tag[i];++i;}
    name[i-1]=0;*consumed=i+1;
    void*material;float widthFactor,heightFactor;
    int pixels=R<int>(font,8);
    store_font_icon::GetFontIconEntryInfo(i+1,name,pixels,&material,&widthFactor,&heightFactor);
    float base=float(pixels),height=(base*heightFactor)*sy,width=(base*widthFactor)*sx;
    y=y-(base*sy+height)*0.5f;
    StretchPicRotate(static_cast<const Byte*>(material),x,y,1,width,height,0,0,1,1,sn,cs,color);
    return width;
}
// 140731E00: legacy ^H/^I inline texture, including malformed-tag fallback.
float DrawLegacyIcon(const char*tag,float x,float y,float sn,float cs,const Byte*font,float sx,float sy,U32 color){
    const float u0=tag[0]=='H'?0.0f:1.0f,u1=tag[0]=='H'?1.0f:0.0f;
    const int pixels=R<int>(font,8);
    const float base=float(pixels)*sy*0.5f;
    const Byte*mat;float w,h;
    if(tag[1]&&tag[2]&&tag[3]&&tag[4]){
        char name[400];const int count=int(static_cast<signed char>(tag[3]));
        std::memcpy(name,tag+4,count);name[count]=0;
        w=float(((int(static_cast<signed char>(tag[1]))-16)*pixels+16)/32)*sx;
        h=float(((int(static_cast<signed char>(tag[2]))-16)*pixels+16)/32)*sy;
        y=y+(h*(-0.5f)-base);
        mat=*name?Effect_RegisterMaterial(name,7,true,-1):Effect_DefaultMaterial;
    }else{
        y=y+(-16.0f-base);w=h=32;
        mat=*Effect_DefaultName?Effect_RegisterMaterial(Effect_DefaultName,7,true,-1):Effect_DefaultMaterial;
    }
    StretchPicRotate(mat,x,y,1,w,h,u0,0,u1,1,sn,cs,color);
    return w;
}
}
#endif
