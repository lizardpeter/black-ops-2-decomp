#!/usr/bin/env python3
"""Probe real BO2 Bink video-plane decoding through ORIGINAL Store DLL code.

This is a non-destructive *oracle discovery* harness, not a reconstructed
full-frame decoder and not proof of gameplay compatibility. The original
0x252C0 frame-plane core runs under Unicorn on the first actual keyframe
from a SHA-256-pinned BO2 .bik fixture. Its caller-derived arguments and
nine scratch-buffer pointers are built from original Ghidra 0x274A0/0x252C0.

It records original-decode call coverage, execution errors, consumed packet
input, and a checksum of the mutated 256x256 output plane. The report will
help implement real-frame traversal and compare a future source decoder.
Never claim a decoded frame when the core did not return successfully.
"""
import argparse
from collections import Counter
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine, OBJECTS, OBJECTS_SIZE  # noqa: E402

PIN_DLL = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
PIN_VIDEO = "c1171207707c3cdcc44160899efbdc7016b1df94b21d11c592546983dd15049c"
RVA = 0x252C0
CODEC_FUNCTIONS = {
    0x23840: "codebook",
    0x24140: "huffman_rle_bytes",
    0x243B0: "two_nibble_signed",
    0x246A0: "two_nibble_unsigned",
    0x24980: "huffman_bytes",
    0x24B80: "huffman_nibble_pair",
    0x24DB0: "huffman_signed_bytes",
    0x24FF0: "delta_rle_16",
    0x2F2E0: "inverse_8x8",
    0x2F630: "inverse_8x8_2x",
    0x2F9F0: "inverse_8x8_prediction",
    0x2FD80: "block_coefficients",
    0x30A60: "residual_prediction",
}
PACKET = OBJECTS + 0x10000
SCRATCH_POINTERS = OBJECTS + 0x50000
SCRATCH_FIRST = OBJECTS + 0x60000
SCRATCH_SPACING = 0x10000
STATS = OBJECTS + 0x1F000


def le32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def probe(img, filedata, *, frame=0, flag=0):
    nframes = le32(filedata, 8)
    width, height = le32(filedata, 20), le32(filedata, 24)
    assert 0 <= frame < nframes
    assert 0 < width <= 4096 and 0 < height <= 4096
    start = le32(filedata, 44 + 4 * frame) & ~1
    end = le32(filedata, 48 + 4 * frame) & ~1
    assert 44 + 4 * (nframes + 1) <= start < end <= len(filedata)
    payload = filedata[start:end]
    assert len(payload) >= 8 and len(payload) % 4 == 0
    partition = le32(payload, 0)
    if partition < 8 or partition > len(payload) or (partition & 3):
        raise ValueError("Unexpected original Bink plane partition offset")
    if len(payload) > SCRATCH_POINTERS - PACKET:
        raise ValueError("Input exceeds safe Unicorn object slot")
    if SCRATCH_FIRST + 9 * SCRATCH_SPACING > OBJECTS + OBJECTS_SIZE:
        raise ValueError("Unicorn scratch buffers exceed mapped OBJECTS")
    # The original 0x274A0 calls 0x252C0 on the luma plane, starting just
    # after the frame partition u32, ending at frame_start+partition.
    # Each plane's scratch regions are provided by caller record +0x150.
    scratch_ptrs = [SCRATCH_FIRST + i * SCRATCH_SPACING for i in range(9)]
    assert width * height <= 0x18000
    image = Machine(img)
    y_before = bytes([0xB7]) * (width * height)
    reference = bytes([0x80]) * (width * height)
    visits = {img.base + rva: "qqq" for rva in CODEC_FUNCTIONS}

    def setup(writer):
        writer.write(PACKET, payload)
        writer.write(SCRATCH_POINTERS, struct.pack("<9Q", *scratch_ptrs))
        writer.u32(STATS, 0)

    outcome, buffers, _, observed = image.call(
        img.base + RVA,
        [y_before, reference, width, height, width,
         PACKET + 4, PACKET + partition, 0,
         STATS, 0, 1, SCRATCH_POINTERS, flag, STATS],
        ret="uint64", setup=setup, observe=visits, limit=15_000_000)

    visited = Counter(CODEC_FUNCTIONS[addr - img.base] for addr, *_ in observed
                      if (addr - img.base) in CODEC_FUNCTIONS)
    report = {
        "fixture": "la_pip_seq_6.bik",
        "fixture_sha256": PIN_VIDEO,
        "frame_zero_based": frame,
        "keyframe": bool(le32(filedata, 44 + 4 * frame) & 1),
        "dimensions": [width, height],
        "compressed_frame_size": len(payload),
        "luma_partition": partition,
        "luma_payload_size": partition - 4,
        "other_plane_bytes": len(payload) - partition,
        "flags_trial": flag,
        "original_rva": f"0x{RVA:X}",
        "original_return": outcome if isinstance(outcome, str)
                           else f"0x{outcome:X}",
        "original_calls_observed": dict(visited),
        "decoded_plane_available": False,
    }
    if not isinstance(outcome, str) and buffers:
        y_after = buffers[0]
        changed = sum(a != b for a, b in zip(y_after, y_before))
        if changed > 0 and frame == 0 and flag == 0:
            # PGM P5 is a lossless grayscale image; no imaging libraries
            # or resampling. This is an ORIGINAL-DLL oracle image only.
            saved = Path("build/original-bink-luma-frame0.pgm")
            saved.parent.mkdir(parents=True, exist_ok=True)
            saved.write_bytes(
                f"P5\\n{width} {height}\\n255\\n".encode("ascii") + y_after)
            report["original_oracle_grayscale_pgm"] = str(saved)
        report.update({
            "decoded_plane_available": changed > 0,
            "y_plane_sha256": hashlib.sha256(y_after).hexdigest(),
            "changed_bytes": changed,
            "luma_byte_sample": y_after[:32].hex(),
            "remaining_input_bytes": PACKET + partition - outcome
                if PACKET + 4 <= outcome <= PACKET + partition else None,
        })
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dll", type=Path)
    ap.add_argument("video", type=Path)
    ap.add_argument("--report", type=Path, default=Path("build/real-bink-frame-probe.json"))
    ap.add_argument("--frame", type=int, default=0)
    ap.add_argument("--flags", type=int, nargs="*", default=[0, 0x8000])
    args = ap.parse_args()
    if hashlib.sha256(args.dll.read_bytes()).hexdigest() != PIN_DLL:
        raise SystemExit("Pinned original DLL SHA-256 mismatch")
    data = args.video.read_bytes()
    if hashlib.sha256(data).hexdigest() != PIN_VIDEO:
        raise SystemExit("Pinned BO2 media SHA-256 mismatch")
    if data[:4] != b"BIKi":
        raise SystemExit("Invalid Bink fixture magic")
    img = Image(args.dll)
    probes = []
    for flag in args.flags:
        try:
            item = probe(img, data, frame=args.frame, flag=flag)
        except Exception as e:
            item = {"flag": flag, "oracle_setup_error": str(e)}
        probes.append(item)
        print(json.dumps(item, sort_keys=True))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({
        "kind": "original-binary-frame-probe",
        "note": "Exploratory only: no source full-frame decode and no DLL replacement.",
        "probes": probes,
    }, indent=2) + "\n")
    # A probe failure is recorded rather than masking a break in the game's
    # existing unit/differential checks. Do NOT assert success without evidence.


if __name__ == "__main__":
    main()
