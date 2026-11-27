// Original Store dvar value display and thread-local formatting helpers.
#define T6_SOURCE "qcommon/dvar.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_dvar_display{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
const char* Display_Format(const char*,...);
B* Display_TLS();
int Display_VFormat(Q,char*,Q,const char*,void*);
extern Q Display_StdioOptions;
void Display_FormatError(int,const char*);
int Display_Vsnprintf(char*,Q,const char*,void*);
}
// 14046A6A0. Dvar values are raw unions, not float arrays for all types.
const char* ValueToString(const B*d,const B*value){
    auto f=[&](unsigned i){return double(R<float>(value,i*4));};
    switch(R<U>(d,0x18)){
    case 1:return value[0]?"1":"0";
    case 2:return Display_Format("%g",f(0));
    case 3:return Display_Format("%g %g",f(0),f(1));
    case 4:case 11:case 12:return Display_Format("%g %g %g",f(0),f(1),f(2));
    case 5:return Display_Format("%g %g %g %g",f(0),f(1),f(2),f(3));
    case 6:return Display_Format("%i",R<int>(value,0));
    case 7:if(R<int>(d,0x50))return R<const char**>(d,0x58)[R<int>(value,0)];return "";
    case 8:return Display_Format("%s",R<const char*>(value,0));
    case 9:return Display_Format("%g %g %g %g",double(float(value[0])*0.003921569f),double(float(value[1])*0.003921569f),double(float(value[2])*0.003921569f),double(float(value[3])*0.003921569f));
    case 10:return Display_Format("%lli",R<long long>(value,0));
    default:return "";
    }
}
// 1403C2E30
Q TLSValue(int index){return R<Q>(R<const B*>(Display_TLS(),0xb0)+static_cast<long long>(index)*8,0);}
// 140472000. va_list adapter retains the original TLS rotation and CRT options.
// The public variadic ABI is expressed through a Microsoft-va-list boundary.
char* FormatThreadLocal(const char*format,void*args){
    B*ring=reinterpret_cast<B*>(TLSValue(1));
    int index=R<int>(ring,0x1000);
    W(ring,0x1000,int(U(index+1))%4);
    char*out=reinterpret_cast<char*>(ring+static_cast<long long>(index)*1024);
    int n=Display_VFormat(Display_StdioOptions|1,out,1024,format,args);
    out[1023]=0;
    if(n<0||n>1023)Display_FormatError(1,out);
    return out;
}
// 140471EC0. Preserve final terminator regardless of CRT return.
void FormatBounded(char*out,int size,const char*format,void*args){
    Display_Vsnprintf(out,static_cast<Q>(static_cast<long long>(size)),format,args);
    out[static_cast<long long>(size)-1]=0;
}
}
#endif
