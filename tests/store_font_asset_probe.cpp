#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/database/db_asset_lookup_store.cpp"
#include <cstdlib>
#include <cctype>
#define MS __attribute__((ms_abi))
namespace store_asset_lookup{
using Q=std::uint64_t;
static B entries[24*64],cache[16*0x110],table[32],cells[16*32],order[64],pool[24*64];
static B defaults[64],assetObjects[32][64],nativeAsset[64];
static char names[32][96],query[400];
static B* activeEntries;
static int findCalls,lowerCalls,compareCalls,internCalls,defaultCalls,allocateCalls;
static U internId;
extern "C"{
B* Asset_Entries=entries;
H Asset_Buckets[0x9600];
const char* (*Asset_NameGetters[64])(const void*);
unsigned (*Asset_Sizes[64])();
void (*Asset_NameSetters[64])(void*,const char*);
const char* Asset_DefaultNames[64],*Asset_TypeNames[64];
U Asset_DefaultCount;
const B* Asset_StringPool=pool;
B* Table_CacheEntries=cache;
B* Table_CacheOrder[16];
int Asset_Lower(int c){++lowerCalls;return std::tolower(static_cast<unsigned char>(c));}
int Asset_Compare(const char*a,const char*b){
    ++compareCalls;
    if(!a||!b)return a!=b;
    while(*a&&*b&&std::tolower(static_cast<unsigned char>(*a))==std::tolower(static_cast<unsigned char>(*b))){++a;++b;}
    return std::tolower(static_cast<unsigned char>(*a))-std::tolower(static_cast<unsigned char>(*b));
}
const void* Asset_Default(int){++defaultCalls;return defaults;}
B* Asset_Allocate(int type,int){++allocateCalls;B*p=activeEntries+63*24;W(p,0,type);W(p,8,static_cast<void*>(nativeAsset));return p;}
void Asset_Unlock(){}
void Asset_Error(int,const char*,...){std::abort();}
unsigned Asset_Intern(const char*,int,int){++internCalls;return internId;}
void* Asset_Find(int type,const char*name,int create,int flags){
    if(type!=42||create!=1||flags!=-1)std::abort();
    ++findCalls;
    for(unsigned i=0;i<32;++i)if(!Asset_Compare(name,names[i]))return assetObjects[i];
    return nullptr;
}
int Table_HashLowerBound(const B*t,int h){
    const short*indices=R<const short*>(t,0x18);const B*p=R<const B*>(t,0x10);
    int n=R<int>(t,8)*R<int>(t,12);
    for(int i=0;i<n;++i)if(R<int>(p+16*indices[i],8)>=h)return i;
    return -1;
}
}
static const char* Getter(const void*p){return R<const char*>(R<const B*>(p,0),0);}
static unsigned AssetSize(){return sizeof nativeAsset;}
static void Setter(void*p,const char*name){W(R<B*>(p,0),0,name);}
static const char* MS MGetter(const void*p){return Getter(p);}
static unsigned MS MSize(){return AssetSize();}
static void MS MSetter(void*p,const char*s){Setter(p,s);}
static int MS MLower(int c){return Asset_Lower(c);}
static int MS MCompare(const char*a,const char*b){return Asset_Compare(a,b);}
static const void*MS MDefault(int t){return Asset_Default(t);}
static B* MS MAllocate(int t,int n){return Asset_Allocate(t,n);}
static U MS MIntern(const char*s,int a,int n){return Asset_Intern(s,a,n);}
static void*MS MFind(int t,const char*s,bool a,int n){return Asset_Find(t,s,a,n);}
static int MS MLowerBound(const B*t,int h){return Table_HashLowerBound(t,h);}
static void*MS MCopy(void*d,const void*s,Q n){return std::memcpy(d,s,n);}
static void*MS MMove(void*d,const void*s,Q n){return std::memmove(d,s,n);}
static U HashName(U t,const char*s){for(;*s;++s){int c=std::tolower(static_cast<unsigned char>(*s));if(c=='\\')c='/';t=t*0x1003fu+U(c);}return t%0x9600;}
static U HashCell(const char*s){U h=0x1505;for(;*s;++s)h=h*33+U(std::tolower(static_cast<unsigned char>(*s)));return h;}
struct Capture{int result,find,compare,lower,intern,defaults,alloc;U defaultCount;H bucket,link;Q assetName;B data[64];int cacheOrder[16];char cacheNames[16][256];U hashes[16];int cacheAsset[16];};
extern "C"{
void*AssetMock(unsigned i){void*p[]={reinterpret_cast<void*>(MLower),reinterpret_cast<void*>(MCompare),reinterpret_cast<void*>(MGetter),reinterpret_cast<void*>(MSize),reinterpret_cast<void*>(MSetter),reinterpret_cast<void*>(MDefault),reinterpret_cast<void*>(MAllocate),reinterpret_cast<void*>(MIntern),reinterpret_cast<void*>(MFind),reinterpret_cast<void*>(MLowerBound),reinterpret_cast<void*>(MCopy),reinterpret_cast<void*>(MMove)};return p[i];}
unsigned AssetSizeCapture(){return sizeof(Capture);}
void AssetRun(void*image,unsigned op,unsigned seed,void*out){
    auto*b=static_cast<B*>(image);Capture c{};U s=seed;auto next=[&](){s=s*1664525u+1013904223u;return s;};
    std::memset(entries,0,sizeof entries);std::memset(Asset_Buckets,0,sizeof Asset_Buckets);std::memset(cache,0xa5,sizeof cache);std::memset(Table_CacheOrder,0,sizeof Table_CacheOrder);
    std::memset(nativeAsset,0xa5,sizeof nativeAsset);for(auto&v:defaults)v=B(next()>>24);
    for(unsigned i=0;i<32;++i){
        char*n=names[i];n[0]='a'+char(i%26);n[1]='0'+char(i/26);n[2]=0;
        W(assetObjects[i],0,static_cast<const char*>(n));
    }
    for(unsigned i=0;i<64;++i){Asset_NameGetters[i]=Getter;Asset_Sizes[i]=AssetSize;Asset_NameSetters[i]=Setter;}
    unsigned type=seed%4+1,chosen=next()%32;
    std::strcpy(query,names[chosen]);if(seed&1)query[0]=char(query[0]-32);
    if(seed&2)query[2]='x',query[3]=0;
    U bucket=HashName(type,query);Asset_Buckets[bucket]=1;
    unsigned match=seed%32+1;
    for(unsigned i=1;i<=32;++i){B*p=entries+i*24;W(p,0,U(i==match?type:(type+1)%64));W(p,8,static_cast<B*>(assetObjects[i==match?chosen:(i-1)]));W(p,0x12,H(i==32?0:i+1));}
    Asset_DefaultCount=0;internId=seed%32;findCalls=lowerCalls=compareCalls=internCalls=defaultCalls=allocateCalls=0;
    activeEntries=b?b+0x3c6e220:entries;
    if(b){
        std::memcpy(activeEntries,entries,sizeof entries);std::memcpy(b+0x3dd0580,Asset_Buckets,sizeof Asset_Buckets);
        for(unsigned i=0;i<60;++i){W(b,0x8e7840+i*8,reinterpret_cast<void*>(MGetter));W(b,0x8e7a20+i*8,reinterpret_cast<void*>(MSize));W(b,0x8e7c00+i*8,reinterpret_cast<void*>(MSetter));}
        W(b,0x3d50578,Asset_DefaultCount);W(b,0x2275610,Asset_StringPool);
        std::memcpy(b+0x412ecf0,cache,sizeof cache);std::memcpy(b+0x412fdf0,Table_CacheOrder,sizeof Table_CacheOrder);
    }
    if(op==0){
        B*p=b?reinterpret_cast<B*(MS*)(U,const char*)>(b+0x211b80)(type,query):FindAssetEntry(type,query);
        c.result=p?int((p-activeEntries)/24):-1;
    }else if(op==1){
        B*p=b?reinterpret_cast<B*(MS*)(int,const char*)>(b+0x211d80)(int(type),query):CloneDefaultAsset(int(type),query);
        c.result=int((p-activeEntries)/24);c.defaultCount=b?R<U>(b,0x3d50578):Asset_DefaultCount;
        c.bucket=b?R<H>(b+0x3dd0580,bucket*2):Asset_Buckets[bucket];c.link=R<H>(p,0x12);
        c.assetName=R<Q>(nativeAsset,0);std::memcpy(c.data,nativeAsset,sizeof nativeAsset);
    }else if(op==2){
        unsigned columns=1+next()%4,rows=1+next()%8,count=columns*rows;
        W(table,8,int(columns));W(table,12,int(rows));W(table,16,static_cast<B*>(cells));W(table,24,static_cast<B*>(order));
        for(unsigned i=0;i<count;++i){const char*n=names[next()%32];W(cells,i*16,n);W(cells,i*16+8,HashCell(n));W(order,i*2,H(i));}
        for(unsigned i=1;i<count;++i){
            H key=R<H>(order,i*2);unsigned j=i;
            while(j&&R<int>(cells,R<H>(order,(j-1)*2)*16+8)>R<int>(cells,key*16+8)){W(order,j*2,R<H>(order,(j-1)*2));--j;}
            W(order,j*2,key);
        }
        int column=int(next()%(columns+2));
        const char*value=(seed&4)?R<const char*>(cells,(next()%count)*16):query;
        c.result=b?reinterpret_cast<int(MS*)(const B*,int,const char*)>(b+0x466470)(table,column,value):FindStringTableRow(table,column,value);
    }else{
        unsigned iterations=seed%31+1;
        void*asset=nullptr;
        for(unsigned i=0;i<iterations;++i){
            unsigned n=next()%32;
            const char*name=names[n];
            if(b)reinterpret_cast<void(MS*)(const char*,void**)>(b+0x4669c0)(name,&asset);else GetStringTable(name,&asset);
        }
        c.result=asset?int((static_cast<B*>(asset)-assetObjects[0])/64):-1;
        B*cacheBase=b?b+0x412ecf0:cache;
        for(unsigned i=0;i<16;++i){
            B*p=b?R<B*>(b+0x412fdf0,i*8):Table_CacheOrder[i];
            c.cacheOrder[i]=p?int((p-cacheBase)/0x110):-1;
            const B*entry=cacheBase+i*0x110;
            c.hashes[i]=R<U>(entry,0);std::memcpy(c.cacheNames[i],entry+4,256);
            const B*ap=R<const B*>(entry,0x108);
            c.cacheAsset[i]=ap>=assetObjects[0]&&ap<assetObjects[0]+sizeof assetObjects?int((ap-assetObjects[0])/64):-1;
        }
    }
    c.find=findCalls;c.compare=compareCalls;c.lower=lowerCalls;c.intern=internCalls;c.defaults=defaultCalls;c.alloc=allocateCalls;
    std::memcpy(out,&c,sizeof c);
}
}
}
