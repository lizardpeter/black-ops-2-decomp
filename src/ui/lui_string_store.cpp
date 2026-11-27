// Original Store string hashing and VM stack insertion: 140126780, 140697A70.
#define T6_SOURCE "ui/lui_hks.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_lui_string{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
struct Value{U tag,aux;Q payload;};
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C" Q String_InternInternal(B*,B*,U,const B*,Q,U);
Q InternString(B*pool,B*vm,const B*text,Q length,U hash){
    return String_InternInternal(pool,vm,0,text,length,hash);
}
static U Rot(U x,unsigned n){return (x<<n)|(x>>(32-n));}
static U Partial(const B*p,unsigned n){
    if(n==4)return R<U>(p,0);
    U x=0;for(unsigned i=0;i<n;++i)x|=U(p[i])<<(24-i*8);
    return x;
}
void PushString(B*vm,const B*text,Q length){
    U a=U(length)+0x6b6f7265u,b=a,c=a;
    unsigned n=length<31?unsigned(length):31;
    const B*p=text;
    while(n>12){
        a+=R<U>(p,0);b+=R<U>(p,4);c+=R<U>(p,8);
        a=(a-c)^Rot(c,4);c+=b;
        b=(b-a)^Rot(a,6);a+=c;
        c=(c-b)^Rot(b,8);b+=a;
        a=(a-c)^Rot(c,16);c+=b;
        b=(b-a)^Rot(a,19);a+=c;
        c=(c-b)^Rot(b,4);b+=a;
        p+=12;n-=12;
    }
    if(n){
        unsigned i=n>4?4:n;a+=Partial(p,i);
        if(n>4){i=n-4>4?4:n-4;b+=Partial(p+4,i);}
        if(n>8)c+=Partial(p+8,n-8);
        c=(c^b)-Rot(b,14);a=(a^c)-Rot(c,11);
        b=(b^a)-Rot(a,25);c=(c^b)-Rot(b,16);
        a=(a^c)-Rot(c,4);b=(b^a)-Rot(a,14);
        c=(c^b)-Rot(b,24);
    }
    Q handle=InternString(R<B*>(vm,0x10)+0x1c0,vm,text,length,c);
    B*top=R<B*>(vm,0x48);
    W(top,0,4u);W(top,8,handle);W(vm,0x48,top+16);
}
}
#endif
