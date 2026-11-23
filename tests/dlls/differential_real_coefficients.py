#!/usr/bin/env python3
"""Real BO2 compressed frame: original vs reconstructed Bink 0x2FD80.

The 65 coefficient blocks are captured from a successfully decoded ORIGINAL
la_pip_seq_6.bik frame. Each block is REPLAYED independently under Unicorn
using the pinned x64 Store Bink DLL, and the same original-layout coefficients
and word reservoir are passed to independently compiled C++ source.

Checks all 128 coefficient bytes, bit-cache, consumed-next-word pointer, and
24-byte original bitreader state byte-for-byte (after pointer relocation).
Also verifies the captured real packet, media SHA256 and unchanged input.
No third-party decoder source or precompiled multimedia replacement is used.
This does NOT constitute an end-to-end reconstructed full video frame.
"""
import ctypes
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image,Machine  # noqa: E402
from differential_entropy_bytes import BitStream  # noqa: E402
from probe_original_bik_frame import PACKET,PIN_DLL,PIN_VIDEO  # noqa: E402

ORIG_RVA=0x2FD80
Y_SHA="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"

def main():
    original=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    source=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_video_coefficients.so"
    corpus_file=Path(sys.argv[3]) if len(sys.argv)>3 else ROOT/"build/real-bink-coefficient-oracle.json"
    video=Path(sys.argv[4]) if len(sys.argv)>4 else ROOT/"originals/bik/la_pip_seq_6.bik"
    if hashlib.sha256(original.read_bytes()).hexdigest()!=PIN_DLL:
        raise SystemExit("Original Bink SHA-256 mismatch")
    videobytes=video.read_bytes()
    if hashlib.sha256(videobytes).hexdigest()!=PIN_VIDEO:
        raise SystemExit("Original video SHA-256 mismatch")
    corpus=json.loads(corpus_file.read_text())
    assert corpus["dll_sha256"]==PIN_DLL and corpus["fixture_sha256"]==PIN_VIDEO
    assert corpus["original_luma_sha256"]==Y_SHA
    frame=videobytes[corpus["frame_packet_offset"]:
                     corpus["frame_packet_offset"]+1676]
    packet=bytes.fromhex(corpus["frame_packet_bytes_hex"])
    assert len(packet)==1676 and packet==frame
    assert len(corpus["calls"])==65
    assert ctypes.sizeof(BitStream)==24

    image=Image(original)
    machine=Machine(image)
    native=ctypes.CDLL(str(source.resolve()))
    fn=native.bink_video_decode_coefficients
    fn.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(BitStream))
    fn.restype=None

    # 32-byte zero padding accommodates original lookahead just past the last
    # word; original inputs are always well formed and bounded by the packet.
    full_native_packet=ctypes.create_string_buffer(packet+bytes(32))
    native_packet_base=ctypes.addressof(full_native_packet)
    checked=0
    for entry in corpus["calls"]:
        before=bytes.fromhex(entry["coeff_buffer_before_hex"])
        recorded_output=bytes.fromhex(entry["coeff_buffer_after_hex"])
        original_bits_before=bytes.fromhex(
            entry["input_bitreader"]["original_bitreader_hex"])
        original_bits_after=bytes.fromhex(
            entry["output_bitreader"]["original_bitreader_hex"])
        assert len(before)==len(recorded_output)==128
        assert len(original_bits_before)==len(original_bits_after)==24

        def setup(writer):writer.write(PACKET,packet)
        orig_return,orig_buffers,_,_=machine.call(
            image.base+ORIG_RVA,[before,original_bits_before],
            "void",setup=setup)
        if orig_return is not None:
            raise AssertionError(
                f"Original coefficient block {entry['index']} failed: {orig_return}")
        if (orig_buffers[0],orig_buffers[1])!=(recorded_output,original_bits_after):
            raise AssertionError(
                f"Captured original corpus differs from direct original "
                f"block replay at index {entry['index']}")

        dest=(ctypes.c_uint8*128).from_buffer_copy(before)
        native_bits=BitStream()
        native_bits.cache,=struct.unpack_from("<I",original_bits_before,0)
        native_bits.reserved4,=struct.unpack_from("<I",original_bits_before,4)
        native_bits.available_bits,=struct.unpack_from("<I",original_bits_before,16)
        native_bits.reserved14,=struct.unpack_from("<I",original_bits_before,20)
        original_next,=struct.unpack_from("<Q",original_bits_before,8)
        offset=original_next-PACKET
        if offset<4 or offset>len(packet)+16 or offset%4:
            raise AssertionError("Original next-word pointer outside packed frame")
        native_bits.next_word=ctypes.cast(
            native_packet_base+offset,ctypes.POINTER(ctypes.c_uint32))
        fn(dest,ctypes.byref(native_bits))
        got_output=ctypes.string_at(ctypes.addressof(dest),128)
        if got_output!=recorded_output:
            mismatch=next(i for i,(a,b) in enumerate(zip(got_output,recorded_output))
                          if a!=b)
            raise AssertionError(
                f"Coefficient block #{entry['index']} byte {mismatch}: "
                f"original={recorded_output[mismatch]}, source={got_output[mismatch]}")
        relocated_expected=bytearray(original_bits_after)
        offset_after=struct.unpack_from("<Q",original_bits_after,8)[0]-PACKET
        struct.pack_into("<Q",relocated_expected,8,native_packet_base+offset_after)
        native_bits_bytes=ctypes.string_at(ctypes.addressof(native_bits),24)
        if native_bits_bytes!=relocated_expected:
            raise AssertionError(
                f"Source bitstream mismatch block #{entry['index']}: "
                f"original relocated={relocated_expected.hex()} "
                f"source={native_bits_bytes.hex()}")
        checked+=1
    if full_native_packet.raw[:len(packet)]!=packet:
        raise AssertionError("Reconstructed coefficient routine modified packet input")
    print(
        f"Original-vs-C++ REAL Bink coefficient decoder RVA 0x{ORIG_RVA:X} "
        f"PASS: {checked} 128-byte blocks and all 24-bitstream-byte states "
        f"from original la_pip_seq_6.bik keyframe; "
        f"Y oracle SHA-256 {Y_SHA}")

if __name__=="__main__":
    main()
