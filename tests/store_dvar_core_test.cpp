// Native source-body regression test; not an original PE differential test.
#define T6_STORE
#define T6_STORE_DVAR_CORE_REPLAY
#include "../src/universal/dvar_registration_store.cpp"
#include <cassert>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <limits>
#include <cstdio>
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
Record *Store_DvarCheats=nullptr;
U32 Store_DvarModifiedFlags=0;
unsigned char Store_DvarCallbacksActive=1;
static int releaseCount=0, setCount=0, latchCount=0;
static const char *released[128]{};
static Value seenSet{}, seenLatch{};
void Store_DvarSleep(unsigned ms) { assert(ms==0); std::abort(); }
int Store_DvarLowercase(int c) { return std::tolower(static_cast<unsigned char>(c)); }
U32 Store_DvarIntern(const char *s,unsigned user,unsigned len) {
    assert(user==0 && len==std::strlen(s)+1 && len<=24 && nextIndex<100);
    const unsigned index=nextIndex++;
    std::memcpy(storage+4+index*24,s,len);
    return index;
}
[[noreturn]] void Store_DvarCapacityError(int,const char *,const char *,int) { std::abort(); }
void Store_DvarRelease(const char *s) { assert(releaseCount<128); released[releaseCount++]=s; }
const char *Store_DvarFormat(Record *,const Value *) { return "0"; }
Value *Store_DvarParse(Value *out,int,const Limits *,const char *) { *out=Value{{17}}; return out; }
Value *Store_DvarConstrain(Value *out,int,const Value *parsed,const Value *,const Limits *) { *out=*parsed; return out; }
void Store_DvarSetVariant(Record *,const Value *v,int source) { assert(source==0); ++setCount; seenSet=*v; }
void Store_DvarSetLatched(Record *,const Value *v) { ++latchCount; seenLatch=*v; }
int Store_DvarFormatBuffer(char *s,unsigned n,const char *fmt,double v) { return std::snprintf(s,n,fmt,v); }
}
}
int main() {
    assert(FindByName(nullptr)==nullptr && FindByName("")==nullptr);
    auto *f=RegisterFloat("cg_fov_default",65,65,90,0x80000,"");
    assert(f==&Store_DvarPool[0] && f==Store_DvarList[0] && f==FindByName("CG_FOV_DEFAULT"));
    assert(f->type==2 && f->flags==0x80000 && f->domain.words[1]==Bits(90));
    assert(f->current.words[0]==Bits(65) && f->latched.words[0]==Bits(65) && f->reset.words[0]==Bits(65));
    assert(!Store_DvarSorted && Store_DvarCount==1 && !Store_DvarWriters && !Store_DvarReaders);
    auto *same=RegisterFloat("CG_FOV_DEFAULT",70,65,120,0x80,"");
    assert(same==f && Store_DvarCount==1 && f->type==2 && f->flags==0x80080);
    assert(f->reset.words[0]==Bits(65) && f->domain.words[1]==Bits(90));
    assert(setCount==0 && latchCount==0);
    Record cheats{};
    Store_DvarCheats=&cheats;
    f->latched.words[0]=Bits(75);
    RegisterFloat("cg_fov_default",80,65,150,0x20,nullptr);
    assert(setCount==2 && latchCount==1 && seenLatch.words[0]==Bits(65));
    assert(seenSet.words[0]==Bits(75)); // setters are mocked: original order matters
    Store_DvarCheats=nullptr;
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
    Value reset{}; reset.string="abc";
    auto original=s->reset.string;
    SetReset(s,&reset);
    assert(s->reset.string==original && releaseCount==0);
    Value copy{}; CopyCurrentString(s,&copy,"abc"); assert(copy.string==original);
    CopyResetString(s,&copy,"abc"); assert(copy.string==original);
    reset.string="other";
    SetReset(s,&reset);
    assert(std::strcmp(s->reset.string,"other")==0 && releaseCount==0);
    reset.string="abc"; SetReset(s,&reset);
    assert(s->reset.string==original && releaseCount==1);
    const char *ownedName=s->name;
    auto *converted=RegisterInt("external",9,0,20,0x40,"new description");
    assert(converted==s && s->type==6 && s->current.words[0]==9 && s->reset.words[0]==9);
    assert(s->domain.words[1]==20 && s->flags==0x40 && !std::strcmp(s->description,"new description"));
    assert(released[1]==ownedName && released[2]==original && Store_DvarModifiedFlags==0x40);
    auto *owned=RegisterString("shutdown_owned","shared",0x4000);
    const char *ownedValue=owned->current.string;
    const char *shutdownName=owned->name;
    Store_DvarCheats=&cheats;
    Shutdown();
    assert(Store_DvarCount==0 && !Store_DvarReaders && !Store_DvarWriters);
    assert(!Store_DvarCallbacksActive && !Store_DvarCheats && !Store_DvarModifiedFlags);
    assert(releaseCount==5 && released[3]==ownedValue && released[4]==shutdownName);
    assert(FindByName("shutdown_owned")==nullptr && owned->current.string==nullptr);
    assert(owned->latched.string==nullptr && owned->reset.string==nullptr);
    assert(RegisterBool("restart",true,0,"")==&Store_DvarPool[0]);
}
