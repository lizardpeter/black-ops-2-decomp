// Original Store item text extents, cached placement (140426F30).
#define T6_SOURCE "ui/ui_main.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_ui_wrappers{void*ItemFont(int,const unsigned char*);}
namespace store_item_text{
using B=unsigned char;using U=std::uint32_t;
template<class T>static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const B*>(p)+off,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<B*>(p)+off,&v,sizeof(v));}
extern "C"{
int Item_Width(int,const char*,int,const B*);
int Item_BindingWidth(int,int,const B*,float);
const B*Item_FindDvar(const char*);
const char*Item_DvarText(const B*,const void*);
}
static const B* TextData(const B*item){
    switch(R<int>(item,0xb4)){
    case 1:case 3:case 4:case 5:case 7:case 8:case 9:case 10:case 11:case 12:case 13:case 14:case 15:case 16:case 18:case 20:case 22:return R<const B*>(item,0xe8);
    default:return nullptr;
    }
}
static float Round(float v){
    v+=0.5f;int n=_mm_cvttss_si32(_mm_set_ss(v));
    if(n!=(-2147483647-1)&&float(n)!=v)return float(n-int(R<U>(&v,0)>>31));
    return v;
}
static int Width(const char*t,const B*font,float scale){return _mm_cvttss_si32(_mm_set_ss(Round(float(Item_Width(0,t,0,font))*scale)));}
void CalculateTextExtents(void* /*unused*/,int client,B*item,const char*text){
    const B*d=TextData(item);int mode=R<int>(item,0xb0);U align=R<U>(d,0x24)&3;
    const char*dvar=R<const char*>(item,0xc0);
    bool dynamic=mode!=6&&dvar;
    B*out=R<B*>(item,0xe8)+static_cast<long long>(client)*0x18;
    if((!align||(mode!=6&&!dynamic))&&(R<float>(out,8)>0||R<float>(out,8)<0))return;
    const B*font=static_cast<const B*>(store_ui_wrappers::ItemFont(client,item));
    float scale=R<float>(d,0x30)*48.0f/float(R<int>(font,8));
    float x=R<float>(d,0x28),y=R<float>(d,0x2c),w=float(Width(text,font,scale));
    scale=R<float>(d,0x30)*48.0f/float(R<int>(font,8));
    float h=float(_mm_cvttss_si32(_mm_set_ss(Round(scale*float(R<int>(font,8))))));
    U ha=R<U>(item,0x18),va=R<U>(item,0x1c);
    if(align){
        float space=R<float>(item,0x10)-float(Width(text,font,scale));
        if(mode==6)space-=float(Item_BindingWidth(client,R<int>(item,0x4c),font,R<float>(d,0x30)));
        else if(dynamic&&(mode==5||mode==7||mode==13||mode==14||mode==30)){
            const char*t="";
            if(dvar&&*dvar)if(const B*v=Item_FindDvar(dvar)){B value[16];std::memcpy(value,v+0x20,16);t=Item_DvarText(v,value);}
            space-=float(Width(t,font,R<float>(d,0x30)*48.0f/float(R<int>(font,8))));
        }
        if(align==1)space*=0.5f;
        x+=space;
    }
    switch(R<U>(d,0x24)&12){
    case 4:y=h+y;break;
    case 8:y=(h+R<float>(item,0x14))*0.5f+y;break;
    case 12:y=R<float>(item,0x14)+y;break;
    default:break;
    }
    if(item[0x41]){x+=R<float>(item,0x54);y=R<float>(item,0x54)+y;}
    W(out,0,x+R<float>(item,8));W(out,4,y+R<float>(item,12));W(out,8,w);W(out,12,h);W(out,16,ha);W(out,20,va);
}
}
#endif
