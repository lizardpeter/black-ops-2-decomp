// Store MP common engine registration (14039E2B0..1403A002B).
// All branches, ordered registration operations and result stores; replay only.
#define T6_SOURCE "qcommon/common.cpp"
#if defined(T6_STORE) && defined(T6_STORE_COMMON_DVAR_REPLAY)
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <xmmintrin.h>
namespace store_common_dvar {
struct Dvar;
using U32=std::uint32_t;
struct alignas(16) Value { U32 words[4]; };
struct alignas(16) Domain { U32 words[4]; };
extern "C" {
const Dvar *Store_Common_Find(const char *);
const Dvar *Store_Common_New(const char *,int,U32,const Value *,const Domain *,const char *);
void Store_Common_Reregister(const Dvar *,const char *,int,U32,const Value *,const Domain *,const char *);
const Dvar *Store_Common_Bool(const char *,bool,U32,const char *);
const Dvar *Store_Common_Float(const char *,float,float,float,U32,const char *);
const Dvar *Store_Common_String(const char *,const char *,U32);
const Dvar *Store_Common_Enum(const char *,const char *const *,int,U32);
void Store_Common_SetIntFromSource(const Dvar *,int,int); // 14046CDF0
void Store_Common_Sub_1402983A0();
bool Store_Common_ExitAfterConnectPaths(); // 140297EB0
extern const char *const Store_Common_DedicatedOptions[]; // 140A53D90
extern const Dvar *Store_Common_War[5]; // 141F04898
extern const Dvar *store_comDvar_141f04878; // 141F04878
extern const Dvar *store_comDvar_141f04908; // 141F04908
extern const Dvar *store_comDvar_141f03820; // 141F03820
extern const Dvar *store_comDvar_141f04910; // 141F04910
extern const Dvar *store_comDvar_141f04998; // 141F04998
extern const Dvar *store_comDvar_141f04850; // 141F04850
extern const Dvar *store_comDvar_141f049d8; // 141F049D8
extern const Dvar *store_comDvar_141f037a8; // 141F037A8
extern const Dvar *store_comDvar_141f037c8; // 141F037C8
extern const Dvar *store_comDvar_141f04858; // 141F04858
extern const Dvar *store_comDvar_141f04848; // 141F04848
extern const Dvar *store_comDvar_141f02f60; // 141F02F60
extern const Dvar *store_comDvar_141f04870; // 141F04870
extern const Dvar *store_comDvar_141f02f70; // 141F02F70
extern const Dvar *store_comDvar_141f04840; // 141F04840
extern const Dvar *store_comDvar_141f02f50; // 141F02F50
extern const Dvar *store_comDvar_141f037c0; // 141F037C0
extern const Dvar *store_comDvar_141f04950; // 141F04950
extern const Dvar *store_comDvar_141f037a0; // 141F037A0
extern const Dvar *store_comDvar_141f03798; // 141F03798
extern const Dvar *store_comDvar_141f04920; // 141F04920
extern const Dvar *store_comDvar_141f037e8; // 141F037E8
extern const Dvar *store_comDvar_141f037d0; // 141F037D0
extern const Dvar *store_comDvar_141f02f68; // 141F02F68
extern const Dvar *store_comDvar_141f04868; // 141F04868
extern const Dvar *store_comDvar_141f048e8; // 141F048E8
extern const Dvar *store_comDvar_141f04930; // 141F04930
extern const Dvar *store_comDvar_141f04888; // 141F04888
extern const Dvar *store_comDvar_141f049e8; // 141F049E8
extern const Dvar *store_comDvar_141f04990; // 141F04990
extern const Dvar *store_comDvar_141f048f0; // 141F048F0
extern const Dvar *store_comDvar_141f04938; // 141F04938
extern const Dvar *store_comDvar_141f04948; // 141F04948
extern const Dvar *store_comDvar_141f04958; // 141F04958
extern const Dvar *store_comDvar_141f02f48; // 141F02F48
extern const Dvar *store_comDvar_141f037d8; // 141F037D8
extern const Dvar *store_comDvar_142126558; // 142126558
extern const Dvar *store_comDvar_142155d00; // 142155D00
extern const Dvar *store_comDvar_141f037e0; // 141F037E0
extern const Dvar *store_comDvar_141f02ef0; // 141F02EF0
extern const Dvar *store_comDvar_141f037b8; // 141F037B8
extern const Dvar *store_comDvar_141f03830; // 141F03830
extern const Dvar *store_comDvar_141f03818; // 141F03818
extern const Dvar *store_comDvar_141f04900; // 141F04900
extern const Dvar *store_comDvar_141f02f58; // 141F02F58
extern const Dvar *store_comDvar_141f03808; // 141F03808
extern const Dvar *store_comDvar_141f049a0; // 141F049A0
extern const Dvar *store_comDvar_141f02ef8; // 141F02EF8
extern const Dvar *store_comDvar_141f048e0; // 141F048E0
extern const Dvar *store_comDvar_141f048d8; // 141F048D8
extern const Dvar *store_comDvar_141f048c8; // 141F048C8
extern const Dvar *store_comDvar_141f048c0; // 141F048C0
extern const Dvar *store_comDvar_141f04880; // 141F04880
extern const Dvar *store_comDvar_141f049b8; // 141F049B8
extern const Dvar *store_comDvar_141f03810; // 141F03810
extern const Dvar *store_comDvar_141f03838; // 141F03838
extern const Dvar *store_comDvar_141f037f0; // 141F037F0
extern const Dvar *store_comDvar_141f02f00; // 141F02F00
extern const Dvar *store_comDvar_141f037b0; // 141F037B0
extern const Dvar *store_comDvar_141f037f8; // 141F037F8
extern const Dvar *store_comDvar_141f02f08; // 141F02F08
extern const Dvar *store_comDvar_141f049a8; // 141F049A8
extern const Dvar *store_comDvar_141f049b0; // 141F049B0
extern const Dvar *store_comDvar_141f049d0; // 141F049D0
extern const Dvar *store_comDvar_141f049c0; // 141F049C0
extern const Dvar *store_comDvar_141f049c8; // 141F049C8
extern const Dvar *store_comDvar_141f048d0; // 141F048D0
extern const Dvar *store_comDvar_141f049f0; // 141F049F0
extern const Dvar *store_comDvar_141de1438; // 141DE1438
extern const Dvar *store_comDvar_141f04988; // 141F04988
}
static float F(U32 bits) { float f; std::memcpy(&f,&bits,4); return f; }
template<class T> static T Read(const Dvar *d,unsigned offset) {
    T v; std::memcpy(&v,reinterpret_cast<const unsigned char *>(d)+offset,sizeof(T)); return v;
}
static int GetInt(const Dvar *d) {
    if (!d) return 0;
    const int type=Read<int>(d,0x18);
    if (type==6 || type==7) return Read<int>(d,0x20);
    if (type==1) return Read<unsigned char>(d,0x20)!=0;
    if (type==2) return _mm_cvttss_si32(_mm_set_ss(Read<float>(d,0x20)));
    return std::atoi(Read<const char *>(d,0x20));
}
static const Dvar *Inline(const char *n,int type,int v,int min,int max,U32 flags) {
    // Unused temporary words normalized; meaningful fields match the Store ABI.
    Value value{{U32(v),0,0,0}};
    Domain domain{{U32(min),U32(max),0,0}};
    if (const auto *d=Store_Common_Find(n)) {
        Store_Common_Reregister(d,n,type,flags,&value,&domain,"");
        return d;
    }
    return Store_Common_New(n,type,flags,&value,&domain,"");
}
void RegisterCommonDvars() {
    store_comDvar_141f04878 = Inline("com_maxclients",6,18,1,18,4);
    Store_Common_Float("com_freemoveScale",F(0x3f800000),F(0),F(0x40a00000),0x80,"");
    Store_Common_String("disconnected_ctrls","",0);
    store_comDvar_141f04908 = Store_Common_Enum("dedicated",Store_Common_DedicatedOptions,0,0x20);
    if (GetInt(store_comDvar_141f04908)!=0)
        store_comDvar_141f04908 = Store_Common_Enum("dedicated",Store_Common_DedicatedOptions,0,0x40);
    store_comDvar_141f03820 = Inline("com_maxfps",6,0,0,1000,1);
    store_comDvar_141f04910 = Store_Common_Bool("grenade_indicators_enabled",1, 0x4000,"");
    store_comDvar_141f04998 = Store_Common_Bool("throwback_enabled",1, 0x4000,"");
    Store_Common_Bool("disable_vcs",0, 0x20000,"");
    Store_Common_Bool("disable_vcs_viewmodel",0, 0x20000,"");
    Inline("vcs_timelimit",6,120,0,2147483647,0x20000);
    store_comDvar_141f04850 = Store_Common_Bool("com_report_syserrors",1, 0x20000,"");
    store_comDvar_141f049d8 = Store_Common_Bool("gts_validation_enabled",1, 0x20000,"");
    store_comDvar_141f037a8 = Store_Common_Bool("lui_checksum_enabled",1, 0x20000,"");
    store_comDvar_141f037c8 = Store_Common_Bool("lui_error_report",1, 0x20000,"");
    store_comDvar_141f04858 = Inline("lui_error_report_delay",6,1000,0,2147483647,0x20000);
    store_comDvar_141f04848 = Store_Common_Float("lui_timescale",F(0x3f800000),F(0x3a83126f),F(0x41200000),0x80,"");
    store_comDvar_141f02f60 = Store_Common_Bool("lui_disable_blur",0, 0,"");
    store_comDvar_141f04870 = Store_Common_Bool("tu7_restoreBlur",1, 0x20000,"");
    store_comDvar_141f02f70 = Store_Common_Bool("tu11_luiCompleteAnimationFix",1, 0x20000,"");
    store_comDvar_141f04840 = Store_Common_Float("zombiemode_path_minz_bias",F(0x42480000),F(0),F(0x43480000),0x1000,"");
    Store_Common_String("zombie_devgui","",0);
    store_comDvar_141f02f50 = Store_Common_Bool("zombie_useOldPathFallback",0, 0x5000,"");
    store_comDvar_141f037c0 = Store_Common_Bool("zombie_double_wide_checks",0, 0x5000,"");
    Store_Common_Bool("spmode",0, 0x100,"");
    Store_Common_Bool("onlinegame",0, 0,"");
    Store_Common_Bool("xblive_rankedmatch",0, 0,"");
    Store_Common_Bool("xblive_privatematch",0, 0,"");
    Inline("sv_fakeServerLoad",6,0,0,500,0);
    Inline("sv_fakeServerLoadRand",6,0,0,500,0);
    Store_Common_Bool("sv_forceunranked",0, 0,"");
    store_comDvar_141f04950 = Inline("developer",6,0,0,2,0);
    store_comDvar_141f037a0 = Store_Common_Bool("developer_script",0, 0,"");
    store_comDvar_141f03798 = Inline("logfile",6,0,0,2,0);
    store_comDvar_141f04920 = Store_Common_Bool("com_statmon",0, 0,"");
    store_comDvar_141f037e8 = Store_Common_Float("com_timescale",F(0x3f800000),F(0x3a83126f),F(0x41200000),0x11c0,"");
    store_comDvar_141f037d0 = Store_Common_Float("timescale",F(0x3f800000),F(0x3a83126f),F(0x41200000),0x180,"");
    store_comDvar_141f02f68 = Inline("fixedtime",6,0,0,1000,0x80);
    store_comDvar_141f04868 = Store_Common_Float("fixedtime_float",F(0),F(0),F(0x447a0000),0x80,"");
    store_comDvar_141f048e8 = Store_Common_Bool("long_blocking_call",0, 0,"");
    store_comDvar_141f04930 = Inline("sv_paused",6,0,0,2,0x40);
    store_comDvar_141f04888 = Inline("cl_paused",6,0,0,2,0x40);
    store_comDvar_141f049e8 = Store_Common_Bool("useMapPreloading",1, 1,"");
    store_comDvar_141f04990 = Store_Common_Bool("useSvMapPreloading",1, 1,"");
    store_comDvar_141f048f0 = Store_Common_Bool("sv_running",0, 0x40,"");
    store_comDvar_141f04938 = Inline("com_voip_resume_time",6,0,0,2147483647,0);
    store_comDvar_141f04948 = Store_Common_Bool("com_introPlayed",0, 0,"");
    store_comDvar_141f04958 = Inline("com_desiredMenu",6,0,0,2147483647,0);
    store_comDvar_141f02f48 = Store_Common_Bool("hiDef",1, 0x40,"");
    store_comDvar_141f037d8 = Store_Common_Bool("wideScreen",1, 0x40,"");
    Store_Common_Bool("doublesided_raycasts",0, 0x80,"");
    Store_Common_Bool("log_append",0, 1,"");
    store_comDvar_142126558 = Store_Common_String("ui_mapname","",0x4000);
    store_comDvar_142155d00 = Store_Common_String("ui_gametype","",0);
    Inline("ui_levelEra",6,2020,0,2020,0);
    Inline("ui_combatCurrScrollBarPos",6,0,-2147483648,2147483647,0);
    Store_Common_String("ui_deadquote","",0x4000);
    store_comDvar_141f037e0 = Inline("ui_mapCount",6,0,0,2147483647,0);
    store_comDvar_141f02ef0 = Store_Common_String("ui_errorTitle","",0x40);
    store_comDvar_141f037b8 = Store_Common_String("ui_errorMessage","",0x40);
    store_comDvar_141f03830 = Store_Common_String("ui_errorMessageDebug","",0x40);
    Store_Common_Bool("ui_autoContinue",0, 0,"");
    Store_Common_Float("creditsScrollScale",F(0x3f800000),F(0),F(0x42c80000),0,"");
    Inline("ui_drawSpinnerAfterMovie",6,0,0,1,0);
    Inline("all_players_are_connected",6,0,0,2147483647,0x4000);
    Store_Common_Bool("scr_hostmigrationtest",0, 0,"");
    store_comDvar_141f03818 = Store_Common_Bool("quit_on_error",0, 0,"");
    store_comDvar_141f04900 = Inline("debug_show_viewpos",6,0,0,1,0);
    store_comDvar_141f02f58 = Inline("scr_wagerTier",6,0,0,2147483647,0);
    store_comDvar_141f03808 = Inline("scr_killcam",6,0,0,2147483647,0);
    store_comDvar_141f049a0 = Inline("scr_numLives",6,0,0,2147483647,0);
    Store_Common_String("debug_protocol","",0);
    Store_Common_String("nextarena","",0);
    store_comDvar_141f02ef8 = Store_Common_Bool("ClickToContinue",0, 0,"");
    Store_Common_String("demoname","",0);
    store_comDvar_141f048e0 = Store_Common_String("createfx","",0);
    Store_Common_String("saved_gameskill","",0x4000);
    store_comDvar_141f048d8 = Store_Common_String("war_sb","",0);
    store_comDvar_141f048c8 = Store_Common_String("scr_allies","",0);
    store_comDvar_141f048c0 = Store_Common_String("scr_axis","",0);
    store_comDvar_141f04880 = Inline("scr_wagerBet",6,0,0,2147483647,0);
    store_comDvar_141f049b8 = Store_Common_Bool("miniscoreboardhide",0, 0,"");
    const char *war[]={"war_a","war_b","war_c","war_d","war_e"};
    for (unsigned i=0;i<5;++i) Store_Common_War[i]=Store_Common_String(war[i],"",0);
    Store_Common_SetIntFromSource(store_comDvar_141f04900,1,0);
    store_comDvar_141f03810 = Inline("band_demosystem",6,64000,0,2147483647,0x20000);
    store_comDvar_141f03838 = Inline("band_2players",6,64000,0,2147483647,0x20000);
    store_comDvar_141f037f0 = Inline("band_4players",6,128000,0,2147483647,0x20000);
    Inline("band_6players",6,192000,0,2147483647,0x20000);
    store_comDvar_141f02f00 = Inline("band_8players",6,256000,0,2147483647,0x20000);
    store_comDvar_141f037b0 = Inline("band_12players",6,384000,0,2147483647,0x20000);
    store_comDvar_141f037f8 = Inline("band_18players",6,580000,0,2147483647,0x20000);
    store_comDvar_141f02f08 = Inline("band_dedicated",6,2048000,0,2147483647,0);
    store_comDvar_141f049a8 = Inline("tu9_testMissingContentPacks",1,1,0,0,0x20000);
    store_comDvar_141f049b0 = Inline("tu11_AddMapPackFlagsUserInfo",1,1,0,0,0x20000);
    store_comDvar_141f049d0 = Inline("tu11_use_animscripted_blends",1,1,0,0,0x20000);
    store_comDvar_141f049c0 = Inline("tu12_zombies_allow_hint_weapon_from_script",1,0,0,0,0x20000);
    store_comDvar_141f049c8 = Inline("tu12_validate_bonus_cards_on_server",1,0,0,0,0x20000);
    store_comDvar_141f048d0 = Inline("tu13_allow_no_player_melee_blood",1,0,0,0,0x20000);
    store_comDvar_141f049f0 = Inline("tu15_zombie_local_player_test_honors_client_server_divide",1,0,0,0,0x20000);
    Store_Common_Sub_1402983A0();
    store_comDvar_141de1438 = Store_Common_Bool("g_loadScripts",!Store_Common_ExitAfterConnectPaths(),0,"");
    store_comDvar_141f04988 = Inline("missingCommandWillError",1,0,0,0,0x20000);
}
} // namespace store_common_dvar
#endif
