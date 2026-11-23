#!/usr/bin/env python3
"""Experimental native SOURCE first BO2 Bink 256x256 luma keyframe.

Builds on original-machine-derived source entropy / transforms / pixel blocks.
No original DLL is loaded or called during this test: the reference image was
already produced separately by original DLL execution under Unicorn.
A PASS requires the entire 65536-byte native Y image to match the pinned
original image SHA256 AND every byte in the original PGM oracle. On a failed
experimental port, print precise phase/first-pixel diagnostics and exit
nonzero; do not mislabel partial work as complete frame playback.
"""
import ctypes
import hashlib
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
PIN_VIDEO="c1171207707c3cdcc44160899efbdc7016b1df94b21d11c592546983dd15049c"
PIN_Y="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"

class Trace(ctypes.Structure):
    _fields_=[
        ("status",ctypes.c_uint32),
        ("error_x",ctypes.c_uint32),
        ("error_y",ctypes.c_uint32),
        ("mode",ctypes.c_uint32),
        ("submode",ctypes.c_uint32),
        ("type_counts",ctypes.c_uint32*10),
        ("submode_counts",ctypes.c_uint32*10),
        ("input_words",ctypes.c_uint32),
        ("input_bits_available",ctypes.c_uint32),
        ("input_cache",ctypes.c_uint32),
        ("first_mode_count",ctypes.c_uint32),
        ("first_mode_bytes",ctypes.c_uint8*32),
        ("first_submode_count",ctypes.c_uint32),
        ("first_submode_bytes",ctypes.c_uint8*32),
    ]

def main():
    src=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"build/bink_source_first_luma.so"
    fixture=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"originals/bik/la_pip_seq_6.bik"
    oracle=Path(sys.argv[3]) if len(sys.argv)>3 else ROOT/"build/original-bink-luma-frame0.pgm"
    b=fixture.read_bytes()
    if hashlib.sha256(b).hexdigest()!=PIN_VIDEO:
        raise SystemExit("Pinned first Bink media SHA-256 mismatch")
    assert b[:4]==b"BIKi"
    f=struct.unpack_from("<I",b,44)[0]&~1
    e=struct.unpack_from("<I",b,48)[0]&~1
    packet=b[f:e]
    assert len(packet)==1676 and struct.unpack_from("<I",packet)[0]==1372
    raw=(ctypes.c_uint8*len(packet)).from_buffer_copy(packet)
    result=(ctypes.c_uint8*65536)(*([0xB7]*65536))
    trace=Trace()
    lib=ctypes.CDLL(str(src.resolve()))
    fn=lib.bink_research_decode_boi_first_luma
    fn.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,
                 ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,
                 ctypes.POINTER(Trace))
    fn.restype=ctypes.c_int
    status=fn(raw,len(packet),result,256,ctypes.byref(trace))
    native=bytes(result)
    got_sha=hashlib.sha256(native).hexdigest()
    print(f"Native experimental first-frame luma: status={status}, trace.status={trace.status}, "
          f"position=({trace.error_x},{trace.error_y}), "
          f"mode={trace.mode}, submode={trace.submode}")
    print("Native first-frame mode counts:",list(trace.type_counts))
    print("Native mode-1 submode counts:",list(trace.submode_counts))
    print("First mode buffer:",trace.first_mode_count,
          bytes(trace.first_mode_bytes).hex())
    print("First submode buffer:",trace.first_submode_count,
          bytes(trace.first_submode_bytes).hex())
    print(f"Native compressed-word cursor={trace.input_words}, "
          f"available={trace.input_bits_available}, cache={trace.input_cache:#x}")
    print("Native Y SHA256:",got_sha)
    print("Original Y SHA256:",PIN_Y)
    if oracle.exists():
        raw_oracle=oracle.read_bytes()
        header=b"P5\n256 256\n255\n"
        assert raw_oracle.startswith(header)
        expected=raw_oracle[len(header):]
        assert len(expected)==65536 and hashlib.sha256(expected).hexdigest()==PIN_Y
        mismatches=[i for i,(a,b) in enumerate(zip(native,expected)) if a!=b]
        print("Native vs original Y pixel differences:",len(mismatches))
        if mismatches:
            print("First 12 differing original/native pixels:",
                  [(i%256,i//256,expected[i],native[i]) for i in mismatches[:12]])
    if status!=0:
        raise AssertionError(f"Experimental reconstructed frame coordinator "
                             f"not yet complete: status={status} "
                             f"at ({trace.error_x},{trace.error_y})")
    if got_sha!=PIN_Y:
        raise AssertionError("Experimental reconstructed source Y plane "
                             "does not yet match the original")
    native_pgm=ROOT/"build/native-bink-first-luma-frame0.pgm"
    native_pgm.parent.mkdir(parents=True,exist_ok=True)
    native_pgm.write_bytes(b"P5\\n256 256\\n255\\n"+native)
    assert native_pgm.read_bytes()==b"P5\\n256 256\\n255\\n"+native
    print("FIRST FULL Y PLANE decoded entirely from original-derived "
          "reconstructed native C++ source: 65536 BYTES EXACT MATCH")
    print("Native verified luma image:",native_pgm)

if __name__=="__main__":main()
