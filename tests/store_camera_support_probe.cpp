#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_support_store.cpp"
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_support {
static Byte state[0x84a00],playback[0x1017a8],def[0x284],attachment[3][0x54],sentinel[0x54];
static const Byte *table[64];
static unsigned calls,active,lastClient;
extern "C" {
Byte *Support_State;
int Support_DemoMode;
Byte Support_PlaybackInited;
const Byte *Support_Playback,*Support_WeaponDefs[256],*Support_Sentinel;
int Support_MissileActive(int client){++calls;lastClient=client;return active;}
}
static int MS MActive(int c){return Support_MissileActive(c);}
extern "C" {
void *SupportMock(){return reinterpret_cast<void*>(MActive);}
void SupportRun(void*image,unsigned key,U*out) {
    calls=0;lastClient=0;active=key&1;int client=(key>>1)&1;
    Support_State=state;Support_Playback=playback;
    Write(state,0x481a8,U32((key&4)?0x40000:0));
    Support_DemoMode=(key&8)?2:0;Support_PlaybackInited=(key>>4)&1;
    Write(playback,0xd4c00,int((key&32)?2:0));
    Write(playback,0xd4c04,int((key&64)?1:0));
    Write(playback,0x63579,Byte((key>>7)&1));
    Write(playback,0x1017a4,int((key&256)?4:0));
    Write(state,0x848f4,int((key>>9)&1));
    Write(state,0x480b8,U32((key&1024)?0x800000:0));
    Write(state,0x64b00,Byte((key>>11)&1));
    if(image){
        Write(image,0xe53a20,Support_State);Write(image,0x2653900,Support_Playback);
        Write(image,0x25a3308,Support_DemoMode);Write(image,0x2653908,Support_PlaybackInited);
    }
    out[0]=image?reinterpret_cast<int(MS*)(int)>(static_cast<Byte*>(image)+0xc1240)(client):GetExtraCamMode(client);
    out[1]=calls;out[2]=lastClient;
    for(unsigned i=0;i<6;++i)Write(state,0x848fc+i*4,U32(key*0x9e3779b9u+i*0xdeadbeefu));
    if(image)reinterpret_cast<void(MS*)()>(static_cast<Byte*>(image)+0xc3390)();else CopySpikeView();
    for(unsigned i=0;i<3;++i){out[3+i]=Read<U32>(state,0x4d924+i*4);out[6+i]=Read<U32>(state,0x649d0+i*4);}
}
void SupportAdsRun(void*image,unsigned key,U32 a,U32 b,U32 c,U32 s,U32 fallback,U*out){
    Support_WeaponDefs[1]=def;Support_Sentinel=sentinel;
    Write(def,0x30,static_cast<const Byte*const*>(table));
    Write(def,0x27c,fallback);Write(def,0x280,fallback^0x80000000u);
    Write(sentinel,0x4c,s);Write(sentinel,0x50,s);
    const U32 bits[]={a,b,c};
    U32 weapon=1;
    for(unsigned i=0;i<3;++i){
        if(key&(1u<<i))weapon|=(i+1)<<(8+6*i);
        table[i+1]=(key&(8u<<i))?nullptr:attachment[i];
        Write(attachment[i],0x4c,bits[i]);Write(attachment[i],0x50,bits[2-i]);
    }
    if(image){Write(image,0x265d3b8,Support_WeaponDefs[1]);Write(image,0x265dbb0,Support_Sentinel);}
    const float x=image?reinterpret_cast<float(MS*)(U32)>(static_cast<Byte*>(image)+0x65b060)(weapon):AdsTransitionA(weapon);
    const float y=image?reinterpret_cast<float(MS*)(U32)>(static_cast<Byte*>(image)+0x65b120)(weapon):AdsTransitionB(weapon);
    U32 i,j;std::memcpy(&i,&x,4);std::memcpy(&j,&y,4);out[0]=i;out[1]=j;
}
}
}
