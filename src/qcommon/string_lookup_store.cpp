// Store MP 14050A9B0: string lookup with original move-to-front chain update.
#define T6_SOURCE "qcommon/stringed.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_string_lookup{
using U=std::uint32_t;using B=unsigned char;
struct Bucket{U flagsAndNext;U id;};
extern "C"{
extern Bucket* StringLookup_Buckets;
extern const B* StringLookup_Pool;
U StringLookup_Hash(const void*,U);
void StringLookup_Lock();
void StringLookup_Unlock();
int StringLookup_Compare(const void*,const void*,U);
}
U FindExistingString(const void*text,U length){
    U index=StringLookup_Hash(text,length);
    StringLookup_Lock();
    Bucket*const buckets=StringLookup_Buckets;
    Bucket&head=buckets[index];
    if((head.flagsAndNext&0x30000u)==0x20000u){
        const B*const pool=StringLookup_Pool;
        auto match=[&](U id){const B*p=pool+U(id*24u);return p[3]==B(length)&&StringLookup_Compare(p+4,text,length)==0;};
        if(match(head.id)){U id=head.id;StringLookup_Unlock();return id;}
        U prev=index,next=head.flagsAndNext&0xffffu;
        while(next!=index){
            Bucket&node=buckets[next];
            if(match(node.id)){
                buckets[prev].flagsAndNext=(buckets[prev].flagsAndNext&0x30000u)|(node.flagsAndNext&0xffffu);
                node.flagsAndNext=(node.flagsAndNext&0x30000u)|(head.flagsAndNext&0xffffu);
                head.flagsAndNext=(head.flagsAndNext&0x30000u)|next;
                U id=node.id;node.id=head.id;head.id=id;
                StringLookup_Unlock();return id;
            }
            prev=next;next=node.flagsAndNext&0xffffu;
        }
    }
    StringLookup_Unlock();return 0;
}
}
#endif
