#!/usr/bin/env python3
"""Replay Store UI_Refresh with modeled external UI, timing and TLS boundaries."""
import ctypes as C
import pathlib, random, struct, subprocess, sys, tempfile
from verify_store_ui_registration import ROOT, SPECS, image
GLOBALS='clients primaryClient info menuCount focusCount focusStack menu menuType transitionDuration transitionStart transitionActive refreshBlocked refreshDeadline refreshIndex refreshLimit luiReady luiState luiDirty luiEvent luiLock buildDvar friendsDvar playerDvar hudScale placements hiresActive hiresX hiresY versionMinor versionPatch white controllerTable'.split()
CONFIG={
'mp':(0x3fd090,[0xe61790,0xa53d88,0x21214c0,0x2123c78,0x2123d80,0x2123c80,0x2125590,0x212560c,0x2125914,0x2125918,0x212591c,0x2126528,0x21553d0,0x213cd70,0x21505f4,0x43c49ec,0x43c5520,0x43c49ef,0xa58920,0x41fba60,0x2121320,0x21212f0,0x21212f8,0xe6287c,0xe63390,0x27f1b60,0x27f1b64,0x27f1b68,0x40b8fd4,0xd2d288,0x905728,0],[0x406fbf0,0x449e974,0x21e448c],[0x8a7258,0x8a7ba0,0x8a7dc0,0x8a72e8,0x8a72f8],[0x4197b0,0,0x6aa840,0x6ab860,0x6abdd0,0x4687d0,0x3eef00,0x42f990,0x415fc0,0x3fd940,0x42f2f0,0x3fcd20,0x472000,0x71a7a0,0x3fc6d0]),
'zm':(0x3fda60,[0xe5f810,0xa51d88,0x211f540,0x2121cf8,0x2121e00,0x2121d00,0x2123610,0x212368c,0x2123994,0x2123998,0x212399c,0x21245a8,0x2153450,0x213adf0,0x214e674,0x43c2a6c,0x43c35a0,0x43c2a6f,0xa56920,0x41f9ae0,0x211f3a0,0x211f370,0x211f378,0xe608fc,0xe61410,0x27efbe0,0x27efbe4,0x27efbe8,0x40b7054,0xd2b308,0x903728,0x1ef7260],[0x406dc70,0x449ca04,0x21e250c],[0x8a5258,0x8a5ba0,0x8a5dc0,0x8a52e8,0x8a52f8],[0x41aad0,0x4996c0,0x6a7f30,0x6a8f50,0x6a94c0,0x4694c0,0x3ef0b0,0x430cb0,0x417320,0x3fe4e0,0x430610,0x3fd6f0,0x472cf0,0x718350,0x3fd0a0]),
}
def events(buf):
    return [struct.unpack_from('<15Q',buf,8+i*120) for i in range(struct.unpack_from('<I',buf)[0])]
def main():
    folder=pathlib.Path(sys.argv[1]);p=C.c_void_p;u=C.c_uint
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-DSTORE_UI_REFRESH_REPLAY','-I'+str(ROOT/'include'),str(ROOT/'src/ui/ui_refresh_store.cpp'),str(ROOT/'tests/store_ui_refresh_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.RefreshMock.argtypes=[u];lib.RefreshMock.restype=p;lib.RunRefresh.argtypes=[p,p,p,p,p,u,C.c_bool,p];lib.RunRefresh.restype=None;lib.RefreshCaptureSize.restype=u
        for mode,(entry,globals_,sysglobals,iat,targets) in CONFIG.items():
            mem=image(folder/('t6'+mode+'.exe'),SPECS[mode][0]);base=C.addressof(C.c_char.from_buffer(mem));g=(p*len(GLOBALS))(*[base+a if a else 0 for a in globals_]);system=[base+a for a in sysglobals]
            for i,t in enumerate(iat):struct.pack_into('<Q',mem,t,lib.RefreshMock(i))
            for i,t in enumerate(targets,5):
                if t:mem[t:t+12]=b'\x48\xb8'+struct.pack('<Q',lib.RefreshMock(i))+b'\xff\xe0'
            o=C.create_string_buffer(lib.RefreshCaptureSize());n=C.create_string_buffer(lib.RefreshCaptureSize());counts={};rng=random.Random(42)
            for seed in [rng.getrandbits(32) for _ in range(4096)]:
                lib.RunRefresh(base+entry,g,*system,seed,mode=='zm',o);lib.RunRefresh(None,g,*system,seed,mode=='zm',n)
                if o.raw!=n.raw or struct.unpack_from('<I',o.raw,4)[0]:
                    print(mode,hex(seed),events(o.raw),events(n.raw),struct.unpack_from('<18I',o.raw,8+48*120),struct.unpack_from('<18I',n.raw,8+48*120),flush=True);raise AssertionError('refresh mismatch')
                for event in events(o.raw):counts[event[0]]=counts.get(event[0],0)+1
            print(mode,4096,'complete refresh comparisons; event counts:',counts,flush=True);mem.close()
if __name__=='__main__':main()
