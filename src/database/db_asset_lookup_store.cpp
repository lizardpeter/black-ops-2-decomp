// Original Store MP asset-entry lookup and string-table retrieval.
// The asset header is at +8, hash-chain link at +0x12, entry size 24.
#define T6_SOURCE "database/db_registry.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_asset_lookup{
using B=unsigned char;using U=std::uint32_t;using H=std::uint16_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
extern B* Asset_Entries;
extern H Asset_Buckets[0x9600];
extern const char* (*Asset_NameGetters[])(const void*);
extern unsigned (*Asset_Sizes[])();
extern void (*Asset_NameSetters[])(void*,const char*);
extern const char* Asset_DefaultNames[],*Asset_TypeNames[];
extern U Asset_DefaultCount;
extern const B* Asset_StringPool;
extern B* Table_CacheEntries;
extern B* Table_CacheOrder[16];
int Asset_Lower(int);
int Asset_Compare(const char*,const char*);
const void* Asset_Default(int);
B* Asset_Allocate(int,int);
void Asset_Unlock();
void Asset_Error(int,const char*,...);
unsigned Asset_Intern(const char*,int,int);
void* Asset_Find(int,const char*,int,int);
int Table_HashLowerBound(const B*,int);
}
static U Hash(U type,const char*name){
    U hash=type;
    for(;*name;++name){int c=Asset_Lower(static_cast<signed char>(*name));if(c=='\\')c='/';hash=hash*0x1003fu+U(c);}
    return hash%0x9600;
}
static int Len(const char*s){int n=0;while(s[n])++n;return n;}
static void Copy(char*d,const char*s,int n){
    if(!s){*d=0;return;}
    for(int i=0;i<n;++i){d[i]=s[i];if(!s[i])break;}d[n-1]=0;
}
// 140211B80
B* FindAssetEntry(U type,const char*name){
    H i=Asset_Buckets[Hash(type,name)];
    while(i){
        B*p=Asset_Entries+unsigned(i)*24;
        if(R<U>(p,0)==type){
            const char*n=Asset_NameGetters[type](p+8);
            if(!Asset_Compare(n,name))return p;
        }
        i=R<H>(p,0x12);
    }
    return nullptr;
}
// 140211D80
B* CloneDefaultAsset(int type,const char*name){
    const void*source=Asset_Default(type);
    if(!source){
        Asset_Unlock();
        if(U(type-11)<2)Asset_Error(1,"Couldn't find the bsp for this map.  Please build the fast file associated with %s and try again.",name);
        else Asset_Error(1,"Could not load default asset '%s' for asset type '%s'.\nTried to load asset '%s'.",Asset_DefaultNames[type],Asset_TypeNames[type],name);
    }
    ++Asset_DefaultCount;
    B*entry=Asset_Allocate(type,0);
    std::memcpy(R<void*>(entry,8),source,Asset_Sizes[type]());
    U bucket=Hash(U(type),name);W(entry,0x12,Asset_Buckets[bucket]);
    Asset_Buckets[bucket]=H((entry-Asset_Entries)/24);
    unsigned id=Asset_Intern(name,4,Len(name)+1);
    const char*text=id?reinterpret_cast<const char*>(Asset_StringPool+U(id*24)+4):nullptr;
    Asset_NameSetters[R<int>(entry,0)](entry+8,text);entry[0x11]=1;
    return entry;
}
// 140466470
int FindStringTableRow(const B*table,int column,const char*value){
    int columns=R<int>(table,8);
    if(!columns||column>=columns)return -1;
    U hash=0;
    if(value){
        hash=0x1505;
        for(const char*p=value;*p;++p)hash=hash*33u+U(Asset_Lower(static_cast<signed char>(*p)));
        if(*value)columns=R<int>(table,8);
    }
    int total=int(U(columns)*U(R<int>(table,12)));
    int index=Table_HashLowerBound(table,int(hash));
    if(index<0||index>=total)return -1;
    const B*values=R<const B*>(table,0x10);
    const short*order=R<const short*>(table,0x18);
    for(;index<total;++index){
        int cell=order[index];const B*p=values+static_cast<long long>(cell)*16;
        if(R<U>(p,8)!=hash)return -1;
        if(cell%columns==column&&R<const char*>(p,0)&&!Asset_Compare(value,R<const char*>(p,0)))return cell/columns;
    }
    return -1;
}
// 1404669C0. Original 16-entry move-to-front cache, not a new hash map.
void GetStringTable(const char*name,void**out){
    if(U(Len(name))>256){*out=Asset_Find(42,name,1,-1);return;}
    char normalized[256];Copy(normalized,name,256);
    U hash=0x1505;
    for(char*p=normalized;*p;++p){
        if(static_cast<unsigned char>(*p-'A')<26)*p=char(*p+32);
        hash=hash*33u+U(static_cast<signed char>(*p));
    }
    B*entry=nullptr;unsigned index=0;
    for(;index<16;++index){
        entry=Table_CacheOrder[index];
        if(!entry){entry=Table_CacheEntries+index*0x110;break;}
        const B*asset=R<const B*>(entry,0x108);
        if(R<U>(entry,0)==hash&&asset){
            const char*s=R<const char*>(asset,0);
            if(s){
                const char*a=normalized;while(*a==*s&&*a){++a;++s;}
                if(*a==*s){
                    for(unsigned i=index;i>0;--i)Table_CacheOrder[i]=Table_CacheOrder[i-1];
                    Table_CacheOrder[0]=entry;*out=const_cast<B*>(asset);return;
                }
            }
        }
    }
    if(!entry){*out=nullptr;return;}
    if(index>15)index=15;
    for(unsigned i=index;i>0;--i)Table_CacheOrder[i]=Table_CacheOrder[i-1];
    Table_CacheOrder[0]=entry;
    void*asset=Asset_Find(42,name,1,-1);
    Copy(reinterpret_cast<char*>(entry+4),normalized,256);W(entry,0,hash);W(entry,0x108,asset);
    *out=asset;
}
}
#endif
