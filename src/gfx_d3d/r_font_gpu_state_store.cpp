// Original Store D3D11 command-state dependencies of RB_EndTessSurface.
// All GPU operations remain explicit calls to original device boundaries.
#define T6_SOURCE "gfx_d3d/r_state.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_font_gpu {
using Byte=unsigned char;using U16=std::uint16_t;using U32=std::uint32_t;using U64=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
extern "C" {
extern void*Gpu_UploadContext;
extern int Gpu_MapBusy;
extern void*Gpu_ConstantBuffers[4];
extern U32 Gpu_ConstantSizes[4];
extern const char*Gpu_CodeTextureNames[];
void Gpu_Lock();void Gpu_Unlock();
void* Gpu_Map(void*,void*,unsigned,unsigned);
void Gpu_Unmap(void*,void*);
void Gpu_IndexBind(void*,void*,unsigned);
void Gpu_DrawIndexed(void*,int,int,int);
void Gpu_InputLayout(void*,void*);
void Gpu_VertexShader(void*,void*);
void Gpu_PixelShader(void*,void*);
void Gpu_Texture(void*,unsigned,void*);
void Gpu_Sampler(void*,unsigned,unsigned);
void Gpu_ApplyMaterialState(Byte*,const Byte*);
void Gpu_ApplyPassArgs(Byte**,int,const Byte*);
void Gpu_MissingVertexType(int,const char*,const char*);
void Gpu_MissingCodeTexture(unsigned,const char*,const char*,const char*,Byte);
const Byte*Gpu_GetMatrix(const Byte*,unsigned,unsigned);
}
static Byte* Stream(Byte*s,unsigned offset){return R<Byte*>(R<Byte*>(s,0x5410),offset);}
static void* MapUntilReady(void*buffer,unsigned mode){
    void*out;do{out=Gpu_Map(Gpu_UploadContext,buffer,0,mode);}while(Gpu_MapBusy);
    return out;
}
// 1407743E0: original dynamic 16-bit index-buffer upload.
int UploadIndices(Byte*s,const void*indices,int count){
    Byte*before=Stream(s,0x4be9d0);
    Gpu_Lock();
    Byte*current=Stream(s,0x4be9d0);
    if(R<int>(current,4)<R<int>(current,0)+count)W(current,0,0);
    int result=R<int>(current,0),at=R<int>(before,0);
    void*buffer=R<void*>(before,8);
    auto*p=static_cast<Byte*>(MapUntilReady(buffer,at!=0?5:4));
    auto*dst=p+at*2;
    if(dst)std::memcpy(dst,indices,count*2);
    Gpu_Unmap(Gpu_UploadContext,buffer);
    if(R<void*>(s,0xd8)!=buffer){W(s,0xd8,buffer);Gpu_IndexBind(R<void*>(s,0xd0),buffer,0x39);}
    if(dst)W(before,0,R<int>(before,0)+count);
    Gpu_Unlock();return result;
}
// 140774700: original dynamic 32-byte vertex-buffer upload.
int UploadVertices(Byte*s,const void*vertices,int count){
    Gpu_Lock();Byte*stream=Stream(s,0x4be9d8);
    int at=R<int>(stream,0);void*buffer=R<void*>(stream,8);
    auto*p=static_cast<Byte*>(MapUntilReady(buffer,at!=0?5:4));auto*dst=p+at;
    if(dst)std::memcpy(dst,vertices,count*32);
    Gpu_Unmap(Gpu_UploadContext,buffer);
    int result=R<int>(stream,0);W(stream,0,result+count*32);
    Gpu_Unlock();return result;
}
// 1407664D0: upload dirty constant buffers and submit indexed triangles.
void DrawIndexed(Byte*deviceState,const int*args){
    void*context=R<void*>(deviceState,0);
    for(unsigned i=0;i<4;++i){
        if(!deviceState[0x10e0+i])continue;
        void*current=R<void*>(deviceState,0);
        void*dst=Gpu_Map(current,Gpu_ConstantBuffers[i],0,4);
        std::memcpy(dst,R<const void*>(deviceState,0x10c0+i*8),Gpu_ConstantSizes[i]);
        Gpu_Unmap(R<void*>(deviceState,0),Gpu_ConstantBuffers[i]);deviceState[0x10e0+i]=0;
    }
    Gpu_DrawIndexed(context,args[1]*3,args[2],0);
}
// 140774530: bind input layout and vertex shader only when cached values change.
void BindVertexShader(Byte*s){
    const Byte*pass=R<const Byte*>(s,0x11d8);
    const Byte*decl=R<const Byte*>(pass,0);
    const Byte*shader=R<const Byte*>(pass,8);
    void*layout=decl?R<void*>(decl,0x28+R<int>(s,0xe0)*8):nullptr;
    if(R<void*>(s,0x120)!=layout){Gpu_InputLayout(R<void*>(s,0xd0),layout);W(s,0x120,layout);decl=R<const Byte*>(pass,0);}
    if(!R<void*>(decl,0x28+R<int>(s,0xe0)*8))
        Gpu_MissingVertexType(R<int>(s,0xe0),R<const char*>(shader,0),R<const char*>(R<const Byte*>(s,0x11c0),0));
    if(R<const Byte*>(s,0x53d8)!=shader){
        Gpu_VertexShader(R<void*>(s,0xd0),shader?R<void*>(shader,8):nullptr);W(s,0x53d8,shader);
    }
}
// 140774620: material pass selection and pixel shader binding.
void SetPass(Byte**pair,U32 passIndex){
    Byte*s=pair[1];const Byte*pass=R<const Byte*>(s,0x11d0)+(passIndex*5+2)*8;
    W(s,0x11d8,pass);W(s,0x11e0,passIndex);
    const Byte*mat=R<const Byte*>(s,0x11c0);
    const Byte*state=R<const Byte*>(mat,0x78)+(passIndex+mat[0x30+s[0x11c8]])*32;
    Gpu_ApplyMaterialState(s,state);
    const Byte*shader=R<const Byte*>(pass,0x10);
    if(R<const Byte*>(s,0x53d0)!=shader){Gpu_PixelShader(R<void*>(s,0xd0),shader?R<void*>(shader,8):nullptr);W(s,0x53d0,shader);}
    if(pass[0x1a])Gpu_ApplyPassArgs(pair,int(static_cast<signed char>(pass[0x1a])),R<const Byte*>(pass,0x20)+(pass[0x18]+pass[0x19])*16);
}
// 140773B90: cache a code constant or matrix argument and update dirty bytes.
void SetCodeConstant(Byte**pair,const Byte*arg){
    Byte*source=pair[0],*state=pair[1];unsigned slot=R<U16>(arg,6),offset=R<U16>(arg,2),index=R<U16>(arg,8);U32 packed=R<U32>(arg,8);
    Byte*key=R<Byte*>(state,0x32b0+slot*8)+(offset>>2)*8;
    if(index<0xd3){
        U64 value=U64(R<U16>(source,0x18c0+index*2))|(U64(packed)<<32);
        if(R<U64>(key,0)!=value){
            W(key,0,value);std::memcpy(R<Byte*>(state,0x1190+slot*8)+offset,source+(index+0x80)*16,16);state[0x11b0+slot]=1;
        }
    }else{
        U64 value=U64(R<U16>(source,0x1aa6+((index-0xd3)>>2)*2))|(U64(packed)<<32);
        if(R<U64>(key,0)==value)return;
        W(key,0,value);unsigned count=packed>>24;
        for(unsigned i=1;i<count;++i)W(key,i*8,U64(-1));
        const Byte*matrix=Gpu_GetMatrix(source,index,(packed>>16)&255);
        std::memcpy(R<Byte*>(state,0x1190+slot*8)+offset,matrix,count*16);state[0x11b0+slot]=1;
    }
}
// 140773CF0: original stable code constant and texture argument walk.
void SetStableArgs(Byte**pair){
    Byte*source=pair[0],*state=pair[1];const Byte*pass=R<const Byte*>(state,0x11d8);
    unsigned count=pass[0x19];if(!count)return;
    const Byte*arg=R<const Byte*>(pass,0x20)+pass[0x18]*16;
    while(count&&R<U16>(arg,0)==3){SetCodeConstant(pair,arg);arg+=16;--count;}
    while(count&&R<U16>(arg,0)==4){
        U32 index=R<U32>(arg,8);void*texture=R<void*>(source,0x1530+index*8);Byte sampler=source[0x16e8+index];
        if(!texture)Gpu_MissingCodeTexture(index,Gpu_CodeTextureNames[index]?Gpu_CodeTextureNames[index]:"noname",
              R<const char*>(R<const Byte*>(state,0x11c0),0),R<const char*>(R<const Byte*>(state,0x11d0),0),state[0x11c8]);
        unsigned tex=arg[2],sam=arg[3];
        if(R<void*>(state,0x50+tex*8)!=texture){W(state,0x50+tex*8,texture);Gpu_Texture(R<void*>(state,0xd0),tex,texture);}
        if(state[sam]!=sampler){
            state[sam]=sampler;
            if(R<U32>(state,0x10+sam*4)!=sampler){Gpu_Sampler(R<void*>(state,0xd0),sam,sampler);W(state,0x10+sam*4,U32(sampler));}
        }
        arg+=16;--count;
    }
}
// 140773EA0: original material constants (types 5/6/7), invalidating cached ranges.
void SetMaterialArgs(Byte**pair,int count,const Byte*arg){
    const Byte*material=R<const Byte*>(pair[1],0x11c0);
    while(count&&R<U16>(arg,0)<5){arg+=16;--count;}
    while(count&&R<U16>(arg,0)==5){
        Byte*s=pair[1],*source=pair[0];U32 packed=R<U32>(arg,8);unsigned slot=R<U16>(arg,6),off=R<U16>(arg,2);
        Byte*cache=R<Byte*>(s,0x5390+slot*8)+(off>>2)*8;
        U64 key=U64(packed)|(U64(R<U16>(source,0x18c0+(packed&65535)*2))<<32);
        if(R<U64>(cache,0)!=key){W(cache,0,key);std::memcpy(R<Byte*>(s,0x1190+slot*8)+off,source+((packed&65535)+0x80)*16,(packed>>24)*16);s[0x11b0+slot]=1;}
        arg+=16;--count;
    }
    const Byte*constant=R<const Byte*>(material,0x70);
    while(count&&R<U16>(arg,0)==6){
        while(R<U32>(constant,0)!=R<U32>(arg,8))constant+=32;
        Byte*s=pair[1];unsigned slot=R<U16>(arg,6),off=R<U16>(arg,2),bytes=R<U16>(arg,4);
        W(R<Byte*>(s,0x5390+slot*8),(off>>2)*8,U64(-1));
        std::memcpy(R<Byte*>(s,0x1190+slot*8)+off,constant+16,bytes);s[0x11b0+slot]=1;
        arg+=16;--count;
    }
    while(count&&R<U16>(arg,0)==7){
        Byte*s=pair[1];unsigned slot=R<U16>(arg,6),off=R<U16>(arg,2),bytes=R<U16>(arg,4);
        W(R<Byte*>(s,0x5390+slot*8),(off>>2)*8,U64(-1));
        std::memcpy(R<Byte*>(s,0x1190+slot*8)+off,R<const void*>(arg,8),bytes);s[0x11b0+slot]=1;
        arg+=16;--count;
    }
}
}
#endif
