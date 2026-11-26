#!/usr/bin/env python3
"""Compile the actual enhanced IN_MouseMove against a tiny host window model.

The full hybrid build has unrelated blockers. This source-body test exercises
the production function, not a copied implementation or a Windows API claim.
"""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
HEADER=r'''
#pragma once
struct POINT {int x,y;};
struct WinMouseVars_t {POINT oldPos;};
struct WinVars_t {int hWnd;int recenterMouse;};
extern WinVars_t g_wv;
struct dvar_t {};
using LocalClientNum_t=int;
constexpr int LOCAL_CLIENT_0=0,LAST_INPUT_MOUSEMOVE=1;
#define T6_STATIC static
#define T6_AT(...)
bool IN_StartRawInput();
void IN_TimeWindowMessages();
void IN_TakeRawMouse(int*,int*);
int GetForegroundWindow();
int GetCursorPos(POINT*);
int ScreenToClient(int,POINT*);
int Dvar_GetInt(const dvar_t*);
void IN_ClampMouseMove(POINT*);
void IN_RecenterMouse();
int Com_LocalClients_GetPrimary();
void Com_LocalClient_LastInput_Set(int,int);
'''
PROBE=r'''
#include <cassert>
#include "stubs.h"
#define T6_STORE
#include "enhanced/win32/win_input.cpp"
WinVars_t g_wv{1,0};
const dvar_t *r_fullscreen=nullptr;
static int active,foreground,success,rx,ry,dx,dy,events,takes,updates,center;
static POINT cursor;
bool IN_StartRawInput(){return active;}
void IN_TimeWindowMessages(){}
void IN_TakeRawMouse(int*x,int*y){++takes;*x=rx;*y=ry;}
int GetForegroundWindow(){return foreground;}
int GetCursorPos(POINT*p){if(success)*p=cursor;return success;}
int ScreenToClient(int,POINT*){return 1;}
int Dvar_GetInt(const dvar_t*){return 0;}
void IN_ClampMouseMove(POINT*){}
void IN_RecenterMouse(){++center;window_center_x=100;window_center_y=200;}
int Com_LocalClients_GetPrimary(){return 0;}
void Com_LocalClient_LastInput_Set(int c,int v){assert(c==0&&v==1);++updates;}
int CL_MouseEvent(int c,int,int,int x,int y){assert(c==0);++events;dx=x;dy=y;return 1;}
int main(){
 for(int mode=0;mode<2;++mode)for(int focused=0;focused<2;++focused)
 for(int ok=0;ok<2;++ok)for(int raw=0;raw<2;++raw)for(int moved=0;moved<2;++moved){
   active=mode;foreground=focused?1:2;success=ok;
   rx=raw?117:0;ry=raw?-29:0;
   cursor={moved?115:100,moved?191:200};
   s_wmv.oldPos={100,200};events=takes=updates=center=0;dx=dy=999;
   IN_MouseMove();
   assert(takes==1);
   if(!focused){assert(events==0&&center==0);continue;}
   const int x=ok&&moved?15:0,y=ok&&moved?-9:0;
   assert(events==1&&dx==(mode?rx:x)&&dy==(mode?ry:y));
   assert(center==(ok&&moved));
 }
}
'''
def main():
    with tempfile.TemporaryDirectory() as d:
        p=Path(d);(p/"stubs.h").write_text(HEADER)
        for name in ("enhanced/raw_input.h","client/client.h","qcommon/qcommon.h","t6_static.h","universal/dvar.h","win32/win_local.h"):
            f=p/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_text('#include "stubs.h"\n')
        source=p/"probe.cpp";source.write_text(PROBE)
        exe=p/"probe"
        subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-I"+str(p),"-I"+str(ROOT),str(source),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
        print("32 enhanced mouse source-body cases passed: raw availability, focus, cursor failure and motion.")
if __name__=="__main__":main()
