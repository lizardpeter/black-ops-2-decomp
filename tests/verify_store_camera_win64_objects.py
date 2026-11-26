#!/usr/bin/env python3
"""Compile each recovered camera translation unit using the real Win64 build flags.

This is a COFF object audit, not a link/binding/activation test. The isolated
standard-header adapters provide the tiny subset used by these source files;
they do not modify the runtime or the repository's global standard headers.
"""
import pathlib
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT
sys.path.insert(0,str(ROOT/"tools"))
import build_recon

UNITS={
    "src/bgame/bg_camera_bob_store.cpp":("T6_STORE_CAMERA_REPLAY",["VerticalBob","HorizontalBob"]),
    "src/cgame/cg_camera_detail_store.cpp":("T6_STORE_CAMERA_REPLAY",["UpdateShake","OffsetOrigin","CalcRemoteEyesView"]),
    "src/cgame/cg_camera_mode_store.cpp":("T6_STORE_CAMERA_REPLAY",["UpdateCameraMode"]),
    "src/cgame/cg_camera_motion_store.cpp":("T6_STORE_CAMERA_REPLAY",["AnglesToAxis","UpdateCameraVelocity","PerturbCamera","UpdateCameraTween"]),
    "src/cgame/cg_camera_projection_store.cpp":("T6_STORE_CAMERA_REPLAY",["CalcFov"]),
    "src/cgame/cg_camera_support_store.cpp":("T6_STORE_CAMERA_REPLAY",["GetExtraCamMode","CopySpikeView","AdsTransitionA","AdsTransitionB"]),
    "src/cgame/cg_fov_defaults_store.cpp":("T6_STORE_CAMERA_REPLAY",["GetDefaultFovForView","ClearViewModeAndSync","SetViewModeAndSync","SyncViewMode"]),
    "src/cgame/cg_third_person_trace_store.cpp":("T6_STORE_CAMERA_REPLAY",["ThirdPersonViewTrace"]),
    "src/cgame/cg_turret_fov_store.cpp":("T6_STORE_CAMERA_REPLAY",["GetTurretViewFov"]),
    "src/cgame/cg_weapon_fov_store.cpp":("T6_STORE_CAMERA_REPLAY",["GetWeaponViewFov"]),
    "src/cgame_mp/cg_camera_effects_store.cpp":("T6_STORE_CAMERA_REPLAY",["OffsetFirstPersonView","ShakeCamera"]),
    "src/cgame_mp/cg_extra_camera_store.cpp":("T6_STORE_CAMERA_REPLAY",["CalcExtraView"]),
    "src/cgame_mp/cg_fov_mode_store.cpp":("T6_STORE_CG_FOV_MODE_REPLAY",["SetHudAndFovMode"]),
    "src/cgame_mp/cg_main_view_store.cpp":("T6_STORE_CAMERA_REPLAY",["CalcViewValues"]),
    "src/cgame_mp/cg_third_person_store.cpp":("T6_STORE_CAMERA_REPLAY",["OffsetThirdPersonView"]),
    "src/cgame_mp/cg_view_fov_store.cpp":("T6_STORE_CAMERA_REPLAY",["CalculateFov"]),
}
HEADERS={
    "cstdint":"""#pragma once
#include <stdint.h>
namespace std { using ::uint32_t; using ::uint64_t; using ::int16_t;
using ::uintptr_t; using ::intptr_t; }
""",
    "cstddef":"""#pragma once
#include <stddef.h>
namespace std { using ::size_t; using ::ptrdiff_t; }
""",
    "cstring":"""#pragma once
#include <stddef.h>
namespace std { inline void* memcpy(void*d,const void*s,size_t n) {
    return __builtin_memcpy(d,s,n);
} }
""",
}
def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp=pathlib.Path(tmp)
        headers=tmp/"headers";headers.mkdir()
        for name,body in HEADERS.items():(headers/name).write_text(body)
        total=0
        for path,(macro,functions) in UNITS.items():
            obj=tmp/(pathlib.Path(path).stem+".obj")
            build_recon.compile_unit(ROOT/path,obj,"x64",["-DT6_STORE","-D"+macro,"-isystem"+str(headers),"-ffunction-sections"])
            raw=obj.read_bytes()
            assert raw[:2]==b"\x64\x86",(path,raw[:2].hex())
            syms=subprocess.check_output(["llvm-nm","--defined-only",str(obj)],text=True)
            for f in functions:
                # Match public function names in MSVC decorated symbols.
                assert "?"+f+"@" in syms,(path,f)
            print(f"{path}: Win64 COFF, {len(functions)} public function bodies, {len(raw)} bytes")
            total+=len(functions)
        print(f"{len(UNITS)} Windows x64 camera objects; {total} expected public function bodies compiled. External references are not linked.")
if __name__=="__main__":main()
