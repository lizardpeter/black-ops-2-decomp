#!/usr/bin/env python3
"""Real BO2 first keyframe original-vs-source Bink pixel tile modes 6/8.

Reads a *freshly generated* original-machine Unicorn trace of the pinned
la_pip_seq_6.bik first keyframe. Mode 8 input colors, exact eight original
pattern bytes and all 8x8 output pixels are captured at actual original
instruction entry/exit points; compare them with our independently compiled
C++ transcription of original BINKCONS pixel mask operations.

Mode 6 compares each 8x8 solid-color fill using the original tile color.
This does NOT claim the complete frame coordinator has been reconstructed.
"""
import ctypes
import json
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
Y_SHA="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"

def main():
    libpath=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"build/bink_video_block_modes.so"
    oracle=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/real-bink-block-modes.json"
    data=json.loads(oracle.read_text())
    assert data["original_luma_sha256"]==Y_SHA
    assert data["block_modes"]["8"]==16
    assert data["block_modes"]["6"]==27
    lib=ctypes.CDLL(str(libpath.resolve()))
    solid=lib.bink_video_fill8x8
    solid.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int32,ctypes.c_uint8)
    solid.restype=None
    pattern=lib.bink_video_two_color8x8
    pattern.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int32,
                      ctypes.c_uint8,ctypes.c_uint8,
                      ctypes.POINTER(ctypes.c_uint8))
    pattern.restype=None

    fill16=lib.bink_video_fill16x16
    fill16.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int32,ctypes.c_uint8)
    fill16.restype=None
    pattern16=lib.bink_video_two_color16x16
    pattern16.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int32,
                        ctypes.c_uint8,ctypes.c_uint8,
                        ctypes.POINTER(ctypes.c_uint8))
    pattern16.restype=None

    checks=0
    for mode in (6,8):
        cases=data["sample_blocks"].get(str(mode),[])
        assert len(cases)==data["block_modes"][str(mode)]
        for i,c in enumerate(cases):
            assert c["mode"]==mode
            assert len(bytes.fromhex(c["after_hex"]))==64
            got=(ctypes.c_uint8*64).from_buffer_copy(bytes.fromhex(c["before_hex"]))
            if mode==6:
                color=bytes.fromhex(c["after_hex"])[0]
                solid(got,8,color)
            else:
                assert "mode8_color0" in c and "mode8_color1" in c
                packed=bytes.fromhex(c["mode8_pattern_hex"])
                assert len(packed)==8
                cp=(ctypes.c_uint8*8).from_buffer_copy(packed)
                pattern(got,8,c["mode8_color0"],c["mode8_color1"],cp)
            actual=bytes(got)
            expected=bytes.fromhex(c["after_hex"])
            if actual!=expected:
                off=next(j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b)
                raise AssertionError(
                    f"Real Bink original-vs-C++ mode={mode} case={i} "
                    f"position={c['x']},{c['y']}, pixel={off}: "
                    f"original={expected[off]}, C++={actual[off]}; "
                    f"colors={c.get('mode8_color0')},{c.get('mode8_color1')}, "
                    f"pattern={c.get('mode8_pattern_hex')}")
            checks+=1
    for group,n in (("1:6",195),("1:8",7)):
        cases=data["sample_blocks"].get(group,[])
        if len(cases)!=n:
            raise AssertionError(f"Expected {n} real original 16x16 tiles in {group}, "
                                 f"received {len(cases)}")
        for i,c in enumerate(cases):
            assert c["mode"]==1 and c["submode"]==int(group[-1])
            before=bytes.fromhex(c["before16_hex"])
            expected=bytes.fromhex(c["after16_hex"])
            assert len(before)==len(expected)==256
            got=(ctypes.c_uint8*256).from_buffer_copy(before)
            if group=="1:6":
                fill16(got,16,expected[0])
            else:
                assert "mode1_pattern_color0" in c and "mode1_pattern_color1" in c
                encoded=bytes.fromhex(c["mode1_pattern_hex"])
                assert len(encoded)==8
                patterns=(ctypes.c_uint8*8).from_buffer_copy(encoded)
                pattern16(got,16,c["mode1_pattern_color0"],
                          c["mode1_pattern_color1"],patterns)
            if bytes(got)!=expected:
                at=next(j for j,(a,b) in enumerate(zip(bytes(got),expected))
                        if a!=b)
                raise AssertionError(
                    f"Original native 16x16 tile mismatch group={group} case={i} "
                    f"at {c['x']},{c['y']}, pixel={at}, "
                    f"original={expected[at]},native={bytes(got)[at]}; "
                    f"pattern={c.get('mode1_pattern_hex')}")
            checks+=1
    print(f"Original BO2 Bink real-frame block modes PASS: "
          f"{checks} tiles: 27 solid 8x8, 16 two-color 8x8, "
          "195 solid 16x16 and 7 two-color patterned 16x16")

if __name__=="__main__":main()
