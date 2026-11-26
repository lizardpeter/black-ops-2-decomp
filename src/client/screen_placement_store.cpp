// Store MP 1400A9AB0, 1400A9B20, 1400D7110, 1400DDB40.
#define T6_SOURCE "client/screen_placement.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstddef>
#include <cstring>
namespace store_font {
using Byte=unsigned char;
extern "C" {
extern int Font_Capture; // 1427F1B60
extern float Font_CaptureX,Font_CaptureY,Font_HudScale; // 1427F1B64,1427F1B68,140E6287C
extern const Byte *Font_ClientTable; // 141EF91E4
extern const Byte *Font_Placements; // 140E63390
extern const Byte *Font_ViewSizes; // 142C64A70
}
template<class T> static T At(const void*p,std::ptrdiff_t off){T v;std::memcpy(&v,static_cast<const Byte*>(p)+off,sizeof(v));return v;}
static float ToView(int client,float value,unsigned axis){
    const auto row=std::ptrdiff_t(client)*24;
    const int index=At<int>(Font_ClientTable,row)==client?At<int>(Font_ClientTable,row+8):-1;
    const float scaled=value*Font_HudScale;
    const float capture=Font_Capture?(axis?Font_CaptureY:Font_CaptureX):1.0f;
    return ((capture*scaled)*At<float>(Font_Placements,std::ptrdiff_t(index)*0x7c+axis*4))/
        At<float>(Font_ViewSizes,std::ptrdiff_t(client)*0x12d0+axis*4);
}
float HudViewX(int client,float value){return ToView(client,value,0);}
float HudViewY(int client,float value){return ToView(client,value,1);}
float VirtualFromRealWidth(const Byte*placement,float value){
    const float scaled=value/Font_HudScale;
    return (scaled/(Font_Capture?Font_CaptureX:1.0f))*At<float>(placement,0x10);
}
float VirtualFromRealHeight(const Byte*placement,float value){
    const float scaled=value/Font_HudScale;
    return (scaled/(Font_Capture?Font_CaptureY:1.0f))*At<float>(placement,0x14);
}
extern "C" {
extern unsigned Font_CaptureWidth,Font_CaptureHeight;
extern float Font_CaptureOffsetX,Font_CaptureOffsetY;
}
// Store MP ScrPlace_ApplyRect 1401F65E0; align 4 retains the original
// (viewportBase + fullScale) multiplication, not an affine translation.
static void ApplyAxis(const Byte*p,float&v,float&size,int align,unsigned axis){
    const unsigned o=axis*4;const auto f=[&](unsigned n){return At<float>(p,n+o);};
    const float s=f(0);
    switch(align){
    case 1:v=s*v+f(0x50);break;
    case 2:v=s*v+f(0x40)*0.5f+f(0x38);break;
    case 3:v=s*v+f(0x58);break;
    case 4:v=(f(0x38)+f(8))*v;size=f(8)*size;return;
    case 5:return;
    case 6:v=v*f(0x10);size=size*f(0x10);return;
    case 7:v=(f(0x58)+f(0x50))*0.5f+s*v;break;
    case 8:v=s*v+f(0x60);break;
    case 9:v=(f(0x68)+f(0x60))*0.5f+s*v;break;
    case 10:v=s*v+f(0x68);break;
    default:v=s*v+f(0x70);break;
    }
    size=s*size;
}
void ApplyRect(const Byte*p,float*x,float*y,float*w,float*h,int ha,int va){
    const float scale=At<float>(p,0x78);
    *x=scale**x;*y=scale**y;*w=scale**w;*h=scale**h;
    ApplyAxis(p,*x,*w,ha,0);ApplyAxis(p,*y,*h,va,1);
    if(Font_Capture){
        *x=Font_CaptureX**x;*y=Font_CaptureY**y;
        *w=Font_CaptureX**w;*h=Font_CaptureY**h;
        *x=*x-(float(Font_CaptureWidth)*Font_CaptureOffsetX)*Font_CaptureX;
        *y=*y-(float(Font_CaptureHeight)*Font_CaptureOffsetY)*Font_CaptureY;
    }
}
}
#endif
