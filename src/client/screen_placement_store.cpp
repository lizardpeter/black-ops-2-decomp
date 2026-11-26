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
}
#endif
