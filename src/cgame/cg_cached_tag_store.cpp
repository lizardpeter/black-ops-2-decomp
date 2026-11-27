// Store MP cached attachment position and skeleton-generation synchronization.
#define T6_SOURCE "cgame/cg_cachedtag.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_cached_tag{
using B=unsigned char;using U=std::uint32_t;using H=std::uint16_t;using Q=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
extern U CachedTag_Mode;
extern H CachedTag_First,CachedTag_Second,CachedTag_Third;
extern const B* CachedTag_GameState;
extern const short* CachedTag_ObjectIndices;
extern const B* CachedTag_Objects;
U CachedTag_Fallback(B*,U,float*);
const B* CachedTag_GetTransform(B*,const B*,U);
void CachedTag_Error(int,const char*);
extern const B* CachedTag_ClientState;
extern int CachedTag_ActiveClient,CachedTag_LastFailureGeneration;
void CachedTag_Yield();
bool CachedTag_TryLock(B*); // atomic compare/exchange 0 -> 1, returns success
void* CachedTag_Allocate(U);
void CachedTag_BuildMask(B*,int,B*);
void CachedTag_Calculate(void*,B*,B*);
void CachedTag_Finalize(B*,B*);
}
// 140106C50
U GetCachedTagPosition(B*entity,U tag,float*out,int timeout,B force){
    if(R<H>(entity,0x2cc)!=1||!R<B*>(entity,0x330)||CachedTag_Mode==2)return CachedTag_Fallback(entity,tag,out);
    B*cache=R<B*>(entity,0x330);
    if(tag!=CachedTag_First){
        if(tag==CachedTag_Second)cache+=0x30;
        else if(tag==CachedTag_Third)cache+=0x60;
        else{
            CachedTag_Error(1,"CachedTag_GetTagPos: Called for a tag that it is not set up for\n");
            cache=nullptr;
            if(!R<B*>(entity,0x330))return CachedTag_Fallback(entity,tag,out);
        }
    }
    const B*state=CachedTag_GameState;
    if(force||int(R<U>(cache,0)+U(timeout))<=R<int>(state,0x4808c)){
        int i=int(U(entity[4])*0x701u+R<U>(entity,0x1f4));
        short index=CachedTag_ObjectIndices[i];
        if(!index)return 0;
        const B*object=CachedTag_Objects+static_cast<long long>(index)*0x90;
        if(!object)return 0;
        const B*transform=CachedTag_GetTransform(entity,object,tag);
        if(!transform)return 0;
        for(unsigned j=0;j<3;++j)W(cache,4+j*4,R<float>(transform,16+j*4)-R<float>(entity,0x40+j*4));
        W(cache,0,R<U>(state,0x4808c));
    }
    for(unsigned j=0;j<3;++j)out[j]=R<float>(cache,4+j*4)+R<float>(entity,0x40+j*4);
    return R<U>(cache,0);
}
// 1400E96B0: lock held over generation checks and skeleton build.
void EnsureTagGeneration(void*context,B*object,int part){
    for(;;){
        if(!R<U>(object,0x14)&&CachedTag_TryLock(object+0x14))break;
        CachedTag_Yield();
    }
    int generation=R<int>(CachedTag_ClientState+static_cast<long long>(CachedTag_ActiveClient)*0x8a400,0x2cb0);
    if(R<int>(object,0x54)!=generation){
        for(unsigned i=0x18;i<0x60;i+=8)W(object,i,Q(0));
    }else if(R<void*>(object,0x58)&&(R<U>(object+static_cast<long long>(part>>5)*4,0x40)&(0x80000000u>>(U(part)&31u)))){
        W(object,0x14,0u);return;
    }
    if(!R<void*>(object,0x58)){
        void*p=CachedTag_Allocate(U(object[0xe])<<5);
        if(!p){if(CachedTag_LastFailureGeneration!=generation)CachedTag_LastFailureGeneration=generation;W(object,0x14,0u);return;}
        W(object,0x58,p);W(object,0x54,generation);
    }
    B mask[32];
    CachedTag_BuildMask(object,part,mask);CachedTag_Calculate(context,object,mask);CachedTag_Finalize(object,mask);
    W(object,0x14,0u);
}
}
#endif
