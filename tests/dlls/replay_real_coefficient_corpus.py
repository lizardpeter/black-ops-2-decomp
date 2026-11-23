#!/usr/bin/env python3
"""Replay 65 actual BO2 Bink coefficient blocks through ORIGINAL FUN_18002FD80.

The deterministic oracle corpus was traced directly from a successful
original FUN_1800252C0 first-keyframe decode. This test calls the ORIGINAL
coefficient function independently 65 times, using each captured original
bitreader and 128-byte output-buffer state and the same original compressed
packet, and asserts EXACT output bytes and final bitreader record.

This proves that the corpus is self-contained and suitable for future
differential testing of a native source translation. It does not claim
the coefficient routine has been reconstructed yet.
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image,Machine  # noqa: E402
from probe_original_bik_frame import PACKET,PIN_DLL,PIN_VIDEO  # noqa: E402

PIN_Y="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"

def main():
    dll=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    corpus_path=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/real-bink-coefficient-oracle.json"
    if hashlib.sha256(dll.read_bytes()).hexdigest()!=PIN_DLL:
        raise SystemExit("Original DLL SHA mismatch")
    corpus=json.loads(corpus_path.read_text())
    assert corpus["dll_sha256"]==PIN_DLL
    assert corpus["fixture_sha256"]==PIN_VIDEO
    assert corpus["original_luma_sha256"]==PIN_Y
    assert corpus["original_coefficient_function_rva"]=="0x2FD80"
    calls=corpus["calls"]
    assert len(calls)==corpus["coefficient_calls"]==65
    packet=bytes.fromhex(corpus["frame_packet_bytes_hex"])
    assert len(packet)==1676 and struct.unpack_from("<I",packet)[0]==1372

    img=Image(dll)
    machine=Machine(img)
    for i,c in enumerate(calls):
        assert c["index"]==i
        before=bytes.fromhex(c["coeff_buffer_before_hex"])
        after=bytes.fromhex(c["coeff_buffer_after_hex"])
        bits_before=bytes.fromhex(c["input_bitreader"]["original_bitreader_hex"])
        bits_after=bytes.fromhex(c["output_bitreader"]["original_bitreader_hex"])
        assert len(before)==len(after)==128
        assert len(bits_before)==len(bits_after)==24
        assert (struct.unpack_from("<Q",bits_before,8)[0]-PACKET
                ==c["input_bitreader"]["next_word_packet_byte_offset"])
        assert (struct.unpack_from("<Q",bits_after,8)[0]-PACKET
                ==c["output_bitreader"]["next_word_packet_byte_offset"])
        def setup(w):w.write(PACKET,packet)
        result,buffers,_,_=machine.call(img.base+0x2FD80,
                                        [before,bits_before],"void",setup=setup)
        if result is not None:
            raise AssertionError(f"Direct original coefficient replay fault #{i}: {result}")
        if buffers[0]!=after:
            pos=next(k for k,(a,b) in enumerate(zip(buffers[0],after)) if a!=b)
            raise AssertionError(f"Replay coefficient mismatch #{i} byte {pos}: "
                                 f"original direct {buffers[0][pos]}, "
                                 f"full-frame oracle {after[pos]}")
        if buffers[1]!=bits_after:
            raise AssertionError(f"Replay bitreader mismatch #{i}: "
                                 f"original direct={buffers[1].hex()}, "
                                 f"full-frame oracle={bits_after.hex()}")
    print(f"Original 0x2FD80 coefficient real-frame corpus replay PASS: "
          f"{len(calls)} independent direct calls, all 128-byte outputs "
          "and full 24-byte bitreader states matched captured real-frame oracle")

if __name__=="__main__":main()
