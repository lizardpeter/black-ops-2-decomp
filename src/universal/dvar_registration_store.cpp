// universal/dvar.cpp -- Store MP registration core and typed wrappers.
// Source-body candidates, isolated until original-PE replay and native bindings are verified.
#define T6_SOURCE "universal/dvar.cpp"
#if defined(T6_STORE) && defined(T6_STORE_DVAR_CORE_REPLAY)
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace store_dvar {
using U32 = std::uint32_t;
union Value {
    U32 words[4];
    const char *string;
};
union Limits {
    U32 words[4];
    struct { int count; const char *const *strings; } enumeration;
};
struct Record {
    const char *name;
    const char *description;
    U32 hash;
    U32 flags;
    int type;
    unsigned char modified;
    Value current, latched, reset;
    Limits domain;
    Record *hashNext;
};
static_assert(sizeof(Value)==16 && sizeof(Limits)==16);
static_assert(sizeof(Record)==0x68 && offsetof(Record,hash)==0x10 &&
              offsetof(Record,flags)==0x14 && offsetof(Record,type)==0x18 &&
              offsetof(Record,modified)==0x1c && offsetof(Record,current)==0x20 &&
              offsetof(Record,latched)==0x30 && offsetof(Record,reset)==0x40 &&
              offsetof(Record,domain)==0x50 && offsetof(Record,hashNext)==0x60);

extern "C" {
extern int Store_DvarReaders;                  // 14413FEE0
extern int Store_DvarWriters;                  // 14413FEE4
extern int Store_DvarCount;                    // 142161680
extern unsigned char Store_DvarSorted;        // 144129C3F
extern Record Store_DvarPool[4096];            // 144147EF0
extern Record *Store_DvarList[4096];           // 14413FEF0
extern Record *Store_DvarBuckets[1024];        // 1441B04F0
extern unsigned char *Store_DvarStringPool;    // 142275610
void Store_DvarSleep(unsigned);                // Sleep import
int Store_DvarLowercase(int);                  // tolower import
U32 Store_DvarIntern(const char *, unsigned, unsigned); // 14050ABE0
// Original capacity error path releases the writer first. The engine error
// must not return for this out-of-pool condition; the replay mock traps it.
[[noreturn]] void Store_DvarCapacityError(int,const char *,const char *,int); // 14039AEB0
void Store_DvarReregister(Record *,const char *,int,U32,const Value *,const Limits *,const char *); // 14046BF10
}

static U32 NameHash(const char *name) {
    if (!name) return 0;
    U32 hash=0x1505;
    while (*name)
        hash=hash*33u+static_cast<U32>(Store_DvarLowercase(static_cast<signed char>(*name++)));
    return hash;
}
static int Load(const int *v) { return __atomic_load_n(v,__ATOMIC_SEQ_CST); }
static int Add(int *v,int delta) { return __atomic_add_fetch(v,delta,__ATOMIC_SEQ_CST); }

// 14046B830 -- deliberately compares the full hash, not the original name.
Record *FindByHash(U32 hash) {
    Add(&Store_DvarReaders,1);
    while (Load(&Store_DvarWriters)!=0) Store_DvarSleep(0);
    Record *d=Store_DvarBuckets[hash&0x3ff];
    while (d && d->hash!=hash) d=d->hashNext;
    Add(&Store_DvarReaders,-1);
    return d;
}
// 14046B8B0 -- null name hashes to zero; empty string hashes to 0x1505.
Record *FindByName(const char *name) { return FindByHash(NameHash(name)); }

static const char *Intern(const char *s) {
    U32 index=Store_DvarIntern(s,0,static_cast<U32>(std::strlen(s)+1));
    return index ? reinterpret_cast<const char *>(Store_DvarStringPool+4+static_cast<U32>(index*24u)) : nullptr;
}

// 14046C1A0 -- original Store allocation, publication and lock protocol.
// Reregister and string ownership helpers remain external original boundaries.
Record *RegisterNew(const char *name,int type,U32 flags,const Value *value,
                    const Limits *limits,const char *description) {
    for (;;) {
        if (!Load(&Store_DvarReaders)) {
            if (Add(&Store_DvarWriters,1)==1 && !Load(&Store_DvarReaders)) break;
            Add(&Store_DvarWriters,-1);
        }
        Store_DvarSleep(0);
    }
    if (Store_DvarCount>=4096) {
        Add(&Store_DvarWriters,-1);
        Store_DvarCapacityError(0,"Can't create dvar '%s': %i dvars already exist",name,4096);
    }
    const int i=Store_DvarCount++;
    Store_DvarSorted=0;
    Record *d=&Store_DvarPool[i];
    Store_DvarList[i]=d;
    d->type=type;
    d->name=(flags&0x4000) ? Intern(name) : name;
    if (type==8) {
        const char *s=Intern(value->string);
        d->current.string=s; d->latched.string=s; d->reset.string=s;
    } else {
        // The original preserves the unused vector lanes in the pool.
        unsigned size=16;
        if (type==3) size=8;
        if (type==4 || type==11 || type==12) size=12;
        std::memcpy(&d->current,value,size);
        std::memcpy(&d->latched,value,size);
        std::memcpy(&d->reset,value,size);
    }
    d->modified=0;
    d->flags=flags;
    d->description=description;
    std::memcpy(&d->domain,limits,16);
    d->hash=NameHash(name);
    auto &bucket=Store_DvarBuckets[d->hash&0x3ff];
    d->hashNext=bucket;
    bucket=d;
    Add(&Store_DvarWriters,-1);
    return d;
}

static Record *Register(const char *name,int type,U32 flags,const Value &v,
                        const Limits &domain,const char *description) {
    // The Store passes two 16-byte temporary objects by pointer.
    alignas(16) Value value=v;
    alignas(16) Limits limits=domain;
    if (Record *d=FindByName(name)) {
        Store_DvarReregister(d,name,type,flags,&value,&limits,description);
        return d;
    }
    return RegisterNew(name,type,flags,&value,&limits,description);
}
static U32 Bits(float x) { U32 u; std::memcpy(&u,&x,4); return u; }
static Limits Domain(float min,float max,float z=0,float w=0) {
    return Limits{{Bits(min),Bits(max),Bits(z),Bits(w)}};
}

// 14046C4C0
Record *RegisterBool(const char *name,bool value,U32 flags,const char *desc) {
    return Register(name,1,flags,Value{{value?1u:0u}},Limits{},desc);
}
// 14046C580
Record *RegisterInt(const char *name,int value,int min,int max,U32 flags,const char *desc) {
    return Register(name,6,flags,Value{{U32(value)}},Limits{{U32(min),U32(max)}},desc);
}
// 14046C650
Record *RegisterFloat(const char *name,float value,float min,float max,U32 flags,const char *desc) {
    return Register(name,2,flags,Value{{Bits(value)}},Domain(min,max),desc);
}
// 14046C760
Record *RegisterVec2(const char *name,float x,float y,float min,float max,U32 flags) {
    return Register(name,3,flags,Value{{Bits(x),Bits(y)}},Domain(min,max),"");
}
// 14046C840 -- the original also copies z into the third domain word.
Record *RegisterVec3(const char *name,float x,float y,float z,float min,float max,U32 flags) {
    return Register(name,4,flags,Value{{Bits(x),Bits(y),Bits(z)}},Domain(min,max,z),"");
}
// 14046C930 -- both trailing domain words are copied from the default vector.
Record *RegisterVec4(const char *name,float x,float y,float z,float w,float min,float max,U32 flags) {
    return Register(name,5,flags,Value{{Bits(x),Bits(y),Bits(z),Bits(w)}},Domain(min,max,z,w),"");
}
// 14046CA30
Record *RegisterString(const char *name,const char *value,U32 flags) {
    Value v{}; v.string=value;
    return Register(name,8,flags,v,Limits{},"");
}
// 14046CAF0
Record *RegisterEnum(const char *name,const char *const *strings,int value,U32 flags) {
    Limits domain{}; domain.enumeration.strings=strings;
    int n=0; while (strings[n]) ++n;
    domain.enumeration.count=n;
    return Register(name,7,flags,Value{{U32(value)}},domain,"");
}
static unsigned Byte(float x) {
    // MINSS returns the second operand on NaN; the following COMISS/JA
    // clamps non-positive inputs. Keep MULSS and ADDSS separate.
    if (!(x<1.0f)) x=1.0f;
    if (!(x>0.0f)) x=0.0f;
    volatile float product=x*255.0f;
    return static_cast<unsigned>(product+0.501f);
}
// 14046CBD0
Record *RegisterColor(const char *name,float r,float g,float b,float a,U32 flags) {
    U32 packed=Byte(r)|(Byte(g)<<8)|(Byte(b)<<16)|(Byte(a)<<24);
    return Register(name,9,flags,Value{{packed}},Limits{},"");
}
} // namespace store_dvar
#endif
