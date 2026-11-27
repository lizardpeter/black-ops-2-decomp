// Store MP 1401A7800 and 1401BBFE0: name index and cached rank-table metadata.
#define T6_SOURCE "ui/ui_rank.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_rank_lookup{
using B=unsigned char;using U=std::uint32_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
extern "C"{
extern const char* RankLookup_Names[126];
extern const char* RankLookup_TableName;
extern const B* RankLookup_Table;
extern int RankLookup_MaxPrestige;
int RankLookup_Compare(const char*,const char*);
void RankLookup_GetTable(const char*,const B**);
int RankLookup_FindRow(const B*,int,const char*);
int RankLookup_ParseInt(const char*);
}
U FindNameIndex(const char*name){
    for(U i=0;i<126;++i)if(!RankLookup_Compare(name,RankLookup_Names[i]))return i;
    return 0;
}
void CacheMaxPrestige(){
    if(RankLookup_MaxPrestige)return;
    const B*table=RankLookup_Table;
    if(!table){RankLookup_GetTable(RankLookup_TableName,&RankLookup_Table);table=RankLookup_Table;}
    const char*value="";
    if(table){
        int row=RankLookup_FindRow(table,0,"maxprestige"),cols=R<int>(table,8);
        if(cols>1&&row>=0&&row<R<int>(table,12)){
            const B*cells=R<const B*>(table,16);
            const char*s=R<const char*>(cells+static_cast<long long>(static_cast<int>(U(cols)*U(row)))*16,16);
            if(s)value=s;
        }
    }
    RankLookup_MaxPrestige=RankLookup_ParseInt(value);
}
}
#endif
