// Store MP 1403FBAF0, UI asset cache, 402 instructions.
// The original path uses prebuilt fonts and also registers two UI materials
// absent from the older reference. All dependencies remain explicit.
#define T6_SOURCE "ui/ui_main.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_font {
using Byte=unsigned char;
template<class T> static T Read(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const Byte*>(p)+off,sizeof(v));return v;}
extern "C" {
extern const Byte *Font_Language; // 142121178
extern const Byte *Font_Resolution; // 1427E71D0
extern void *Font_Assets[21]; // 142126570 through 142126610
int Font_Atoi(const char*); // IAT 1408A7DC0
const char *Font_Format(const char*,int,const char*); // 140472000
void *Font_Register(const char*,int); // 14071A780
void *Font_Material(const char*,int,bool,int); // 14071A4E0
}
static int Int(const Byte*d){
    if(!d)return 0;
    switch(Read<int>(d,0x18)){
    case 6:case 7:return Read<int>(d,0x20);
    case 1:return Read<Byte>(d,0x20)!=0;
    case 2:return _mm_cvttss_si32(_mm_set_ss(Read<float>(d,0x20)));
    default:return Font_Atoi(Read<const char*>(d,0x20));
    }
}
static void Material(unsigned offset,const char*name,int track=3){
    Font_Assets[offset/8]=Font_Material(name,track,true,-1);
}
static void *RegisterTier(const char*name){
    return Font_Register(Font_Format("fonts/%d/%s",Int(Font_Resolution),name),0);
}
void AssetCache(){
    Material(0x30,"white");
    Material(0x10,"menu_mp_lobby_scrollbar_main");
    Material(0x00,"ui_scrollbar_arrow_left");
    Material(0x08,"ui_scrollbar_arrow_right");
    Material(0x18,"menu_mp_lobby_scrollbar_block");
    Material(0x28,"ui_arrow_right");
    Material(0x40,"ui_arrow_left");
    Material(0x48,"ui_arrow_right");
    Material(0x38,"ui_cursor",0);
    const Byte*lang=Font_Language;
    bool single=false;
    if(lang){
        int type=Read<int>(lang,0x18);
        int language=Int(lang);
        if(type!=1&&type!=2&&type!=6&&type!=7)lang=Font_Language;
        single=language==11||(lang&&Int(lang)==13);
    }
    void*extraSmall;
    if(single){
        Font_Assets[0x78/8]=RegisterTier("consoleFont");
        extraSmall=RegisterTier("normalfont");
        Font_Assets[0x88/8]=extraSmall;
        Font_Assets[0x90/8]=extraSmall;
        Font_Assets[0x80/8]=extraSmall;
        Font_Assets[0x70/8]=extraSmall;
        Font_Assets[0x68/8]=extraSmall;
    }else{
        Font_Assets[0x68/8]=RegisterTier("bigfont");
        Font_Assets[0x70/8]=RegisterTier("smallfont");
        Font_Assets[0x78/8]=RegisterTier("consoleFont");
        Font_Assets[0x80/8]=RegisterTier("boldfont");
        Font_Assets[0x88/8]=RegisterTier("normalfont");
        Font_Assets[0x90/8]=RegisterTier("extrabigfont");
        extraSmall=RegisterTier("extrasmallfont");
    }
    Font_Assets[0x98/8]=extraSmall;
    Font_Assets[0xa0/8]=Font_Register("fonts/distfont",0);
    Material(0x58,"ui_blur");
    Material(0x60,"ui_line_graph");
}
// 1400D0050: original resolution scale helper, not dynamic font rasterization.
float ResolutionScale(){return Font_Resolution?720.0f/float(Int(Font_Resolution)):1.0f;}
}
#endif
