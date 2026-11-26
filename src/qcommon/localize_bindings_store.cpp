// Original Store MP localization lookup and binding substitutions.
#define T6_SOURCE "qcommon/strings.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstring>
#include <cstdint>
namespace store_localize{
using B=unsigned char;
extern "C"{
const char** Local_FindAsset(int,const char*,bool,int); // 140212150
int Local_Binding(int,const char*,char*,int,bool); // 14042A890
bool Local_Alternate(int,const char*,char*); // 140429180
const char* Local_SafeTranslate(const char*); // 1403FF450
void Local_Format(char*,unsigned,const char*,const char*,const char*); // 140471EC0
void Local_Blink(char*,unsigned); // 1403FF700
extern const B*Local_ClientTable;
extern const char*Local_Primary[2];
}
static char Lower(char c){return c>='A'&&c<='Z'?char(c+32):c;}
static bool Equal(const char*a,const char*b){while(Lower(*a)==Lower(*b)){if(!*a)return true;++a;++b;}return false;}
static void Copy(char*d,const char*s,unsigned n){
    if(!d)return;
    if(!s){*d=0;return;}
    for(unsigned i=0;i<n;++i){d[i]=s[i];if(!s[i])break;}d[n-1]=0;
}
// 1403E8C90
const char* Lookup(const char*key){const char**asset=Local_FindAsset(24,key,true,-1);return asset?*asset:nullptr;}
// 1403E8EF0
void BindingText(int client,const char*key,char*out){
    if(!Local_Binding(client,key,out,0,true)){
        char base[256];const char*p=key;while(*p&&*p!=':')++p;
        if(*p==':'){unsigned n=unsigned(p-key);std::memcpy(base,key,n);base[n]=0;}
        else Copy(base,key,256);
        if(!Local_Alternate(client,key,out)){
            if(Equal(base,"gocrouch")){
                if(!Local_Binding(client,"togglecrouch",out,0,true)&&!Local_Binding(client,"+movedown",out,0,true)&&!Local_Binding(client,"+stance",out,0,true)){
                    Local_Format(out,256,"%s(%s)",Local_SafeTranslate("KEY_UNBOUND"),key);return;
                }
            }else if(Equal(base,"primary")){
                int mode;std::memcpy(&mode,Local_ClientTable+static_cast<long long>(client)*24+0x14,4);
                Copy(out,Local_Primary[mode!=0],256);
            }else{
                Local_Format(out,256,"%s(%s)",Local_SafeTranslate("KEY_UNBOUND"),key);return;
            }
        }
    }
    Local_Blink(out,256);
}
// 1403E91A0: the UI binding variant uses 256-byte key/replacement temporaries.
bool ReplaceBinding(int client,unsigned*index,unsigned*length,char*text){
    char copy[1024],key[256],replacement[256];
    std::memcpy(copy,text,*length);copy[*length]=0;
    char*open=nullptr,*close=nullptr;
    for(char*p=copy+*index;*p;++p)if(p[0]=='['&&p[1]=='{'){open=p;break;}
    if(!open)return false;
    for(char*p=open;*p;++p)if(p[0]=='}'&&p[1]==']'){close=p;break;}
    if(!close)return false;
    int n=int(close-open)-2;if(!n)return false;
    std::memcpy(key,open+2,n);key[n]=0;BindingText(client,key,replacement);
    unsigned count=0;while(replacement[count])++count;
    unsigned total=*length-unsigned(n)-4+count;if(total+1>1024)return false;
    unsigned at=unsigned(open-copy);std::memcpy(text+at,replacement,count);
    unsigned tail=total-at-count;std::memcpy(text+at+count,close+2,tail);
    text[total]=0;*index=at+count;*length=total;return true;
}
}
#endif
