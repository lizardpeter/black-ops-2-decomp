#!/usr/bin/env python3
"""SHA-pinned complete Store BIKi corpus verifier.

Each shard has a deterministic load-balanced subset. Every frame compares
the original whole-frame coordinator against the independently routed native
planes, then separately compares source-linked playback output. A failed
clip is recorded and never promoted to a corpus pass. Originals stay ignored.
"""
import argparse
import hashlib
import json
import struct
import subprocess
import sys
import urllib.parse
import urllib.request
from pathlib import Path

from differential_bink_coordinator import run, ROOT, BASE

MANIFEST=ROOT/"docs/bink-store-media-inventory.json"

def sha(path):
    h=hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda:f.read(1<<20),b""):h.update(chunk)
    return h.hexdigest()

def fetch(clip,directory):
    name=clip["name"]
    assert Path(name).name==name and name.endswith(".bik")
    directory.mkdir(parents=True,exist_ok=True)
    path=directory/name
    if not path.exists() or path.stat().st_size!=clip["size_bytes"] or sha(path)!=clip["sha256"]:
        partial=path.with_suffix(".partial")
        try:
            req=urllib.request.Request(BASE+urllib.parse.quote("bo2/DUMP/video/"+name),
                headers={"User-Agent":"bo2-bink-corpus/1"})
            h=hashlib.sha256()
            size=0
            with urllib.request.urlopen(req,timeout=90) as remote,partial.open("wb") as out:
                while chunk:=remote.read(1<<20):
                    size+=len(chunk)
                    if size>clip["size_bytes"]:raise ValueError("fixture size overflow")
                    h.update(chunk)
                    out.write(chunk)
            if size!=clip["size_bytes"] or h.hexdigest()!=clip["sha256"]:
                raise ValueError("fixture integrity mismatch")
            partial.replace(path)
        finally:
            partial.unlink(missing_ok=True)
    with path.open("rb") as stream:
        words=struct.unpack("<11I",stream.read(44))
    assert words[0]==0x694b4942 and words[1]+8==clip["size_bytes"]
    assert words[2]==words[4]==clip["frames"]
    assert words[3]==clip["max_frame_bytes"]
    assert (words[5],words[6],words[9],words[10])==(clip["width"],clip["height"],clip["flags"],clip["audio_tracks"])
    return path

def same_file(a,b):
    if a.stat().st_size!=b.stat().st_size:return False
    with a.open("rb") as fa,b.open("rb") as fb:
        while True:
            left,right=fa.read(1<<20),fb.read(1<<20)
            if left!=right:return False
            if not left:return True

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dll",type=Path,required=True)
    p.add_argument("--library",type=Path,required=True)
    p.add_argument("--player",type=Path,required=True)
    p.add_argument("--fixture-dir",type=Path,default=ROOT/"originals/bik-corpus")
    p.add_argument("--report-dir",type=Path,default=ROOT/"build/corpus")
    p.add_argument("--shard",type=int,default=0)
    p.add_argument("--shards",type=int,default=1)
    p.add_argument("--frames",type=int,default=16,help="0 = full sequence, otherwise per-clip frame limit")
    a=p.parse_args()
    assert 0<=a.shard<a.shards and a.frames>=0
    inventory=json.loads(MANIFEST.read_text())
    clips=inventory["clips"]
    assert len(clips)==inventory["files"]==44
    assert len({c["name"] for c in clips})==44
    assert sum(c["frames"] for c in clips)==inventory["total_frames"]
    assert all(c["audio_tracks"]==0 for c in clips)
    def count(c):return c["frames"] if a.frames==0 else min(c["frames"],a.frames)
    def weight(c):return count(c)*c["width"]*c["height"]*(5 if c["flags"] else 3)
    groups=[[] for _ in range(a.shards)]
    loads=[0]*a.shards
    for c in sorted(clips,key=lambda c:(-weight(c),c["name"])):
        group=min(range(a.shards),key=lambda i:(loads[i],i))
        groups[group].append(c)
        loads[group]+=weight(c)
    root=a.report_dir
    root.mkdir(parents=True,exist_ok=True)
    summary={"shard":a.shard,"shards":a.shards,"frame_limit":a.frames,
             "expected_clips":len(groups[a.shard]),"results":[]}
    def save():(root/"shard-summary.json").write_text(json.dumps(summary,indent=2)+"\n")
    for c in groups[a.shard]:
        clipdir=root/Path(c["name"]).stem
        clipdir.mkdir(exist_ok=True)
        report=clipdir/"coordinator-report.json"
        native=clipdir/"playback.yuv"
        oracle=clipdir/"coordinator-matched.yuv"
        result={"name":c["name"],"requested_frames":count(c),"pass":False}
        summary["results"].append(result)
        print(f"CORPUS START {c['name']} frames={count(c)}",flush=True)
        try:
            fixture=fetch(c,a.fixture_dir)
            run(a.dll,fixture,a.library,count(c),report,c["sha256"])
            native.unlink(missing_ok=True)
            subprocess.run([str(a.player.resolve()),str(fixture),str(native),
                            "--frames",str(count(c))],check=True)
            if not same_file(native,oracle):raise AssertionError("playback output mismatch")
            details=json.loads(report.read_text())
            assert details["complete_matching_frames"]==count(c)
            result.update(pass_=True,matched_frames=details["complete_matching_frames"],
                          matched_planes=details["matched_planes"],matched_bytes=details["matched_bytes"],
                          decoded_sha256=sha(native))
            result["pass"]=True
            result.pop("pass_")
            print("CORPUS PASS",json.dumps(result),flush=True)
        except Exception as e:
            result["error"]=str(e)
            print("CORPUS FAIL",json.dumps(result),flush=True)
        finally:
            # Keep compact JSON evidence, not gigabytes of decoded game media.
            for path in (native,oracle,native.with_suffix(native.suffix+".partial")):
                path.unlink(missing_ok=True)
            save()
    if not all(r["pass"] for r in summary["results"]):
        raise SystemExit("Corpus shard failed")
    print("CORPUS SHARD PASS",a.shard,len(summary["results"]),flush=True)

if __name__=="__main__":main()
