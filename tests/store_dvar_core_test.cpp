// Native source-body regression test; not an original PE differential test.
#define T6_STORE
#define T6_STORE_DVAR_CORE_REPLAY
#include "../src/universal/dvar_registration_store.cpp"
#include <cassert>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <limits>
using namespace store_dvar;
namespace store_dvar {
extern "C" {
int Store_DvarReaders=0,Store_DvarWriters=0,Store_DvarCount=0;
unsigned char Store_DvarSorted=1;
Record Store_DvarPool[4096]{};
Record *Store_DvarList[4096]{},*Store_DvarBuckets[1024]{};
static unsigned char storage[4096]{};
unsigned char *Store_DvarStringPool=storage;
static unsigned nextIndex=1;
static int reregisterCount=0;
static Value seenValue{};
static Limits seenDomain{};
static U32 seenFlags=0;
static int seenType=0;
void Store_DvarSleep(unsigned ms) { assert(ms==0); std::abort(); }
int Store_DvarLowercase(int c) { return std::tolower(static_cast<unsigned char>(c)); }
U32 Store_DvarIntern(const char *s,unsigned user,unsigned len) {
    assert(user==0 && len==std::strlen(s)+1 && len<=24 && nextIndex<100);
    const unsigned index=nextIndex++;
    std::memcpy(storage+4+index*24,s,len);
    return index;
}
[[noreturn]] void Store_DvarCapacityError(int,const char *,const char *,int) { std::abort(); }
void Store_DvarReregister(Record *,const char *,int type,U32 flags,const Value *v,const Limits *l,const char *) {
    ++reregisterCount; seenType=type; seenFlags=flags; seenValue=*v; seenDomain=*l;
}
}
}
int main() {
    assert(FindByName(nullptr)==nullptr && FindByName("")==nullptr);
    auto *f=RegisterFloat("cg_fov_default",65,65,90,0x84000,"");
    assert(f==&Store_DvarPool[0] && f==Store_DvarList[0] && f==FindByName("CG_FOV_DEFAULT"));
    assert(f->type==2 && f->flags==0x84000 && f->domain.words[1]==Bits(90));
    assert(f->current.words[0]==Bits(65) && f->latched.words[0]==Bits(65) && f->reset.words[0]==Bits(65));
    assert(!Store_DvarSorted && Store_DvarCount==1 && !Store_DvarWriters && !Store_DvarReaders);
    auto *same=RegisterFloat("CG_FOV_DEFAULT",70,65,120,0x80,"");
    assert(same==f && reregisterCount==1 && Store_DvarCount==1 && seenType==2 && seenFlags==0x80);
    assert(seenValue.words[0]==Bits(70) && seenDomain.words[1]==Bits(120));
    auto *b=RegisterBool("bool",true,1,"desc");
    assert(b->type==1 && b->current.words[0]==1 && !std::strcmp(b->description,"desc"));
    auto *n=RegisterInt("integer",-7,-100,100,0,"");
    assert(n->type==6 && n->current.words[0]==U32(-7) && n->domain.words[0]==U32(-100));
    Store_DvarPool[3].current.words[2]=0xfeed;
    auto *v2=RegisterVec2("v2",2,3,0,5,0);
    assert(v2->type==3 && v2->current.words[1]==Bits(3) && v2->current.words[2]==0xfeed);
    auto *v3=RegisterVec3("v3",1,2,3,-5,5,0);
    assert(v3->type==4 && v3->domain.words[2]==Bits(3));
    auto *v4=RegisterVec4("v4",1,2,3,4,-5,5,0);
    assert(v4->type==5 && v4->reset.words[3]==Bits(4) && v4->domain.words[3]==Bits(4));
    auto *s=RegisterString("external","abc",0x4000);
    assert(s->type==8 && !std::strcmp(s->name,"external") && !std::strcmp(s->current.string,"abc"));
    assert(s->current.string==s->latched.string && s->current.string==s->reset.string);
    const char *options[]={"off","on",nullptr};
    auto *e=RegisterEnum("enum",options,1,0);
    assert(e->type==7 && e->domain.enumeration.count==2 && e->domain.enumeration.strings==options);
    auto *c=RegisterColor("color",-1,0.5f,1,std::numeric_limits<float>::quiet_NaN(),0);
    assert(c->type==9 && c->current.words[0]==0xffff8000u);
    // Store matches on the full hash rather than name, and uses chained buckets.
    Record fake{}; fake.hash=0x1234; fake.hashNext=c;
    auto &bucket=Store_DvarBuckets[0x1234&0x3ff];
    auto *old=bucket; bucket=&fake;
    assert(FindByHash(0x1234)==&fake && FindByHash(0x5234)==nullptr);
    bucket=old;
    assert(!Store_DvarReaders && !Store_DvarWriters && Store_DvarCount==9);
}
