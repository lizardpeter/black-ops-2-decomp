// Store MP HUD world projection, edge clamping and fade helpers.
#define T6_SOURCE "cgame/cg_hudelem.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_hud_math {
using B=unsigned char;using U=std::uint32_t;
template<class T>static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const B*>(p)+off,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<B*>(p)+off,&v,sizeof(v));}
extern "C"{
extern B*Hud_State; // 140E53A20
extern const B*Hud_Clients,*Hud_Placements;
extern const B*Hud_ColorEnable,*Hud_ColorScale,*Hud_EdgeEnable,*Hud_EdgeRadius,*Hud_FadeRadius,*Hud_FadeTime;
int Hud_Atoi(const char*);
double Hud_Atof(const char*);
}
static int Int(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(U(t-6)<2)return R<int>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Hud_Atoi(R<const char*>(d,0x20));
}
static float Float(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(t==2)return R<float>(d,0x20);
    if(t==6)return float(R<int>(d,0x20));
    return float(Hud_Atof(R<const char*>(d,0x20)));
}
static bool Bool(const B*d){return d&&(R<int>(d,0x18)==1?d[0x20]!=0:Hud_Atoi(R<const char*>(d,0x20))!=0);}
static float Abs(float f){U v=R<U>(&f,0)&0x7fffffffu;return R<float>(&v,0);}
static float Sqrt(float f){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(f)));}
static const B*Placement(int client){
    const B*p=Hud_Clients+static_cast<long long>(client)*24;
    int i=R<int>(p,0)==client?R<int>(p,8):-1;
    return Hud_Placements+static_cast<long long>(i)*0x7c;
}
// 1400F3D70
void UnpackHudColor(const B*c,float*out){
    for(unsigned i=0;i<4;++i)out[i]=float(c[i])*0.003921569f;
    if(Int(Hud_ColorEnable)){
        float s=Float(Hud_ColorScale);
        out[0]=s*out[0];out[1]=s*out[1];out[2]=s*out[2];
    }
}
static bool Project(int client,const float*world,float*out,unsigned origin,unsigned axis,unsigned fov){
    const B*s=Hud_State,*p=Placement(client);
    float dx=world[0]-R<float>(s,origin),dy=world[1]-R<float>(s,origin+4),dz=world[2]-R<float>(s,origin+8);
    auto dot=[&](unsigned off){return (dy*R<float>(s,off+4)+dx*R<float>(s,off))+dz*R<float>(s,off+8);};
    float forward=dot(axis),x=dot(axis+12),y=dot(axis+24);
    float w=R<float>(p,0x40),h=R<float>(p,0x44);
    if(forward>=0){
        float inv=1.0f/forward;
        out[0]=((1.0f-(x/R<float>(s,fov))*inv)*w)*0.5f+R<float>(p,0x38);
        out[1]=((1.0f-(y/R<float>(s,fov+4))*inv)*h)*0.5f+R<float>(p,0x3c);
        return true;
    }
    x=-x;y=-y;out[0]=x;out[1]=y;
    if(!(Abs(x)>=0.001f)){
        if(!(Abs(y)>=0.001f)){out[1]=h+h;return false;}
        out[0]=x=0.001f;
    }
    if(!(Abs(y)>=0.001f))out[1]=y=0.001f;
    while(!(Abs(x)>=w)){x=w*out[0];y=y*w;out[0]=x;out[1]=y;}
    while(!(Abs(y)>=h)){y=y*h;out[1]=y;out[0]=h*out[0];}
    return false;
}
// 1400F4C00
bool WorldToHud(int client,const float*world,float*out){return Project(client,world,out,0x4d924,0x4d934,0x4d910);}
// 1400F4E90
bool WorldToHudAlternate(int client,const float*world,float*out){return Project(client,world,out,0x64918,0x64924,0x64948);}
// 1400F5120: original rectangular projection and optional rounded corners.
bool ClampHud(int client,float*pos,float left,float right,float top,float bottom,float*direction,float*distance){
    const B*p=Placement(client);
    float originalX=pos[0],originalY=pos[1];
    float w=R<float>(p,0x40),h=R<float>(p,0x44),cx=w*0.5f,cy=h*0.5f;
    float xmin=(left+R<float>(p,0x60))-cx;
    float xmax=cx-((w-R<float>(p,0x68))+right);
    float ymin=(top+R<float>(p,0x64))-cy;
    float ymax=cy-((h-R<float>(p,0x6c))+bottom);
    float x=originalX-cx,y=originalY-cy;bool changed=false;
    float bound=x<xmin?xmin:xmax;
    if(x<xmin||xmax<x){float f=bound/x;y=y*f;x=x*f;changed=true;}
    bound=y<ymin?ymin:ymax;
    if(y<ymin||ymax<y){float f=bound/y;x=x*f;y=y*f;changed=true;}
    pos[0]=x;pos[1]=y;
    if(Bool(Hud_EdgeEnable)){
        float radius=((R<float>(p,4)+R<float>(p,0))*Float(Hud_EdgeRadius))*0.5f;
        float cornerX=x>=0?xmax-radius:radius+xmin;
        float cornerY=y>=0?ymax-radius:radius+ymin;
        float dx=x-cornerX,dy=y-cornerY;
        float len=Sqrt(dy*dy+dx*dx);
        float inv=1.0f/(len>0?len:1.0f);
        dx*=inv;dy*=inv;
        if((x>=0?dx>0:dx<0)&&(y>=0?dy>0:dy<0)&&len>radius){
            changed=true;pos[0]=dx*radius+cornerX;pos[1]=dy*radius+cornerY;
        }
    }
    pos[0]=cx+pos[0];pos[1]=cy+pos[1];
    if(!changed)return false;
    float dx=originalX-pos[0],dy=originalY-pos[1];
    *distance=Sqrt(dx*dx+dy*dy);
    float len=Sqrt(dy*dy+dx*dx),inv=1.0f/(len>0?len:1.0f);
    direction[0]=dx*inv;direction[1]=dy*inv;return true;
}
// 1400F5510
void FadeHudNearCenter(int client,const B*elem,const float*pos,float*rgba){
    B*s=Hud_State;
    if(R<float>(s,0x48288)<1.0f){W(s,0x6b770,0);return;}
    float radius=Float(Hud_FadeRadius);
    if(!Hud_FadeRadius||!(radius>0))return;
    const B*p=Placement(client);
    float dx=R<float>(p,0x40)*0.5f-pos[0],dy=R<float>(p,0x44)*0.5f-pos[1];
    float squared=dy*dy+dx*dx,rs=radius*radius;
    if(squared>rs)return;
    int now=R<int>(s,0x4808c),start=R<int>(s,0x6b770);
    if(!start){start=now;W(s,0x6b770,start);}
    int duration=Int(Hud_FadeTime);float fade=0;
    if(now<=int(U(start)+U(duration))&&start<int(U(now)+U(duration))&&Hud_FadeTime){
        duration=Int(Hud_FadeTime);
        if(duration>0){
            float f=1.0f-float(int(U(now)-U(R<int>(s,0x6b770))))/float(duration);
            if(f>0)fade=f>=1?1:f;
        }
    }
    float frac=squared/rs;
    if(fade<=frac)fade=frac;
    rgba[3]=(float(elem[0x1b])*0.003921569f)*fade;
}
// 1400DE970: original precomputed quaternion multiplier, no renormalization.
void QuaternionAxis(const float*q,float*out){
    float z=q[2],y=q[1],scale=q[7],ys=y*scale,xs=q[0]*scale,w=q[3];
    float zw=z*scale*w,xx=xs*q[0],zz=z*scale*z;
    out[0]=1.0f-(zz+ys*y);out[1]=zw+y*xs;out[2]=z*xs-ys*w;
    out[3]=y*xs-zw;out[4]=1.0f-(zz+xx);out[5]=z*ys+xs*w;
    out[6]=ys*w+z*xs;out[7]=z*ys-xs*w;out[8]=1.0f-(ys*y+xx);
}
}
#endif
