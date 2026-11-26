#!/usr/bin/env python3
"""Compile Store input candidates as actual x64 COFF objects, without live linking."""
import pathlib
import shutil
import subprocess
import tempfile
from verify_store_camera_win64_objects import ROOT, build_recon, HEADERS
UNITS={
    "src/client/cl_input_support_store.cpp":["ShouldUpdateViewAngles","FinishMove","InputFrame"],
    "src/client/cl_input_commands_store.cpp":["CmdButtons","DemoViewAngleGate","SpectatorInput","UpdateAirburstMarkDistance"],
}
def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp=pathlib.Path(tmp)
        headers=tmp/"headers";headers.mkdir()
        adapters=dict(HEADERS)
        adapters["cstdint"]=adapters["cstdint"]+"namespace std { using ::uint16_t; }\n"
        adapters["cmath"]="#pragma once\nnamespace std { inline float sqrt(float x) { return __builtin_sqrtf(x); } }\n"
        for name,body in adapters.items():(headers/name).write_text(body)
        total=0
        for path,functions in UNITS.items():
            obj=tmp/(pathlib.Path(path).stem+".obj")
            build_recon.compile_unit(ROOT/path,obj,"x64",["-DT6_STORE","-DT6_STORE_INPUT_REPLAY","-isystem"+str(headers),"-ffunction-sections"])
            raw=obj.read_bytes()
            assert raw[:2]==b"\x64\x86",(path,raw[:2].hex())
            nm=shutil.which("llvm-nm") or shutil.which("nm") or "llvm-nm"
            syms=subprocess.check_output([nm,"--defined-only",str(obj)],text=True)
            for f in functions:assert "?"+f+"@" in syms,(path,f)
            total+=len(functions)
            print(f"{path}: Win64 COFF, {len(functions)} public bodies, {len(raw)} bytes")
        print(f"{total} Store input functions compiled; external references not linked.")
if __name__=="__main__":main()
