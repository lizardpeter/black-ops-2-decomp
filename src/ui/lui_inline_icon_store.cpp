// Store MP LUI inline icon quad and original Lua cache insertion (1406AF590).
#define T6_SOURCE "lui/lui_render.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_lui_text{
using Byte=unsigned char;
float FontHeight(const Byte*);
void TransformPoint(float*,float*,float*);
}
namespace store_lui_icon{
using B=unsigned char;using U=std::uint32_t;
template<class T>static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const B*>(p)+off,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<B*>(p)+off,&v,sizeof(v));}
extern "C"{
void Icon_Info(int,const char*,int,void**,float*,float*); // 140723450
void Icon_PackColor(const float*,B*); // 1400F2040
void Icon_Submit(const void*,short,const void*,const void*,const void*,int); // 14071DD30
void Icon_RawGet(void*,int,int); // 140663840
void* Icon_UserData(void*,int); // 140662630
void* Icon_Alloc(void*,int); // 14069EA10
void Icon_Unref(void*,int,int); // 14069BD00
int Icon_Ref(void*,int); // 14069BB00
extern int Icon_Layer,Icon_AllocCount,Icon_AllocBytes;
}
static B* NewIcon(void*lua){
    B*p=static_cast<B*>(Icon_Alloc(lua,0x7c));++Icon_AllocCount;Icon_AllocBytes+=0x7c;return p;
}
B* DrawInlineIcon(int /*unused1*/,void* /*unused2*/,void*lua,const char*tag,void*material,
                  const B*font,float*x,float y,float scale,const float*color,B*node){
    float width,height;
    float uv[8]={0,0,1,0,0,1,1,1};
    if(material){
        int a=int(store_lui_text::FontHeight(font)),b=int(store_lui_text::FontHeight(font));
        width=float(((int(static_cast<signed char>(tag[1]))-16)*a+16)/32)*0.040625f;
        height=float(((int(static_cast<signed char>(tag[2]))-16)*b+16)/32)*0.040625f;
        if(*tag=='I'){uv[0]=uv[4]=1;uv[2]=uv[6]=0;}
    }else Icon_Info(0,tag,R<int>(font,8),&material,&width,&height);
    float nominal=float(R<int>(font,8)),left=*x,right=nominal*width*scale+left,top=y-nominal*height*scale;
    float pos[12]={left,top,0,right,top,0,left,y,0,right,y,0};
    *x=right;
    for(unsigned i=0;i<12;i+=3)store_lui_text::TransformPoint(pos+i,pos+i+1,pos+i+2);
    float rgba[4]={1,1,1,color[3]};U packed;Icon_PackColor(rgba,reinterpret_cast<B*>(&packed));
    if(!node){Icon_Submit(material,1,pos,uv,&packed,Icon_Layer);return nullptr;}
    B*out=nullptr;bool allocated=false;
    int ref=R<int>(node,0x24);
    if(ref==-2){
        out=NewIcon(lua);allocated=true;W(out,0x24,-2);
    }else{
        Icon_RawGet(lua,-10000,ref);B*existing=static_cast<B*>(Icon_UserData(lua,-1));
        W(lua,0x48,R<B*>(lua,0x48)-16);
        if(existing&&R<short>(existing,0x20)>=1)out=existing;
        else{
            out=NewIcon(lua);allocated=true;
            if(existing){W(out,0x24,R<int>(existing,0x24));Icon_Unref(lua,-10000,R<int>(node,0x24));}
            else W(out,0x24,-2);
        }
    }
    // Reusing an existing positive-count node does not change registry ownership.
    if(allocated)W(node,0x24,Icon_Ref(lua,-10000));
    W(out,0x18,material);W(out,0,out+0x28);W(out,8,out+0x58);W(out,0x10,out+0x78);
    W(out,0x20,short(1));out[0x22]=0;
    std::memcpy(out+0x28,pos,48);std::memcpy(out+0x58,uv,32);W(out,0x78,packed);
    return out;
}
}
#endif
