// Original Store MP asynchronous zone completion (140213410).
#define T6_SOURCE "database/db_registry.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_zone_complete{
using B=unsigned char;using U=std::uint32_t;using H=std::uint16_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
extern "C"{
extern void* Complete_Event,*Complete_GumpEvent;
extern const B* Complete_LoadRecords;
extern int Complete_LoadCount;
extern B* Complete_ZoneRecords;
extern U Complete_PendingCount,Complete_Nesting,Complete_MainThread,Complete_LevelCount;
extern void* Complete_PendingIds[];
extern volatile U Complete_SyncDepth;
extern U Complete_GumpFlag,Complete_GumpStatus;
extern U Complete_Loading;
unsigned Complete_Wait(void*,unsigned);
void Complete_Signal(void*);
void Complete_FinishFiles(const char*,const char*,void*,void*);
void Complete_FinishLevel(U);
void Complete_Log(const char*);
void Complete_GumpLoaded();
unsigned* Complete_TLSId();
unsigned Complete_CurrentThread();
void Complete_EndSync();
void Complete_BeginSync();
void Complete_PrepareMain();
void Complete_Prepare();
void Complete_Lock();
void Complete_Unlock();
void Complete_FinalizeAsset(void*,int);
void Complete_Finish();
}
static bool IsMain(){
    U id=*Complete_TLSId();
    if(!id){id=Complete_CurrentThread();*Complete_TLSId()=id;}
    return id==Complete_MainThread;
}
static void SignalComplete(){
    if(Complete_GumpFlag){
        Complete_Log("Sys_GumpLoaded set gumpLoadedEvent\n");
        Complete_GumpFlag=0;Complete_GumpStatus=0;
        Complete_Signal(Complete_GumpEvent);Complete_GumpLoaded();
    }
    Complete_Loading=0;Complete_Signal(Complete_Event);
}
void CompleteZoneLoad(){
    if(Complete_Wait(Complete_Event,0)==0)return;
    B scratch[256];Complete_FinishFiles("","",nullptr,scratch);
    for(int i=0;i<Complete_LoadCount;++i){
        H index=R<H>(Complete_LoadRecords+i*0x88,0);
        B*p=Complete_ZoneRecords+unsigned(index)*0x4c;
        if(R<U>(p,0)==2){U value=3;std::memcpy(p,&value,4);}
    }
    if(!Complete_PendingCount){
        U end=Complete_LevelCount+1;
        if(end>1)
            for(U i=1;i<end;i=(i&~255u)|((i+1)&255u))Complete_FinishLevel(i);
        SignalComplete();return;
    }
    U depth=Complete_SyncDepth,restore=0;
    if(IsMain()){
        ++Complete_Nesting;
        while(Complete_SyncDepth)Complete_EndSync();
        --Complete_Nesting;Complete_SyncDepth=0;restore=depth;
        if(!Complete_PendingCount){
            Complete_PendingCount=0;
            while(depth--)Complete_BeginSync();
            return;
        }
    }
    if(IsMain())Complete_PrepareMain();
    Complete_Prepare();Complete_Lock();
    for(U i=0;i<Complete_PendingCount;++i)Complete_FinalizeAsset(Complete_PendingIds[i],1);
    Complete_PendingCount=0;Complete_Unlock();Complete_Finish();
    if(IsMain())while(restore--)Complete_BeginSync();
    SignalComplete();
}
}
#endif
