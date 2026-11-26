#!/usr/bin/env python3
"""Check the enhanced loader's inline UI default against the two pinned Store PEs."""
import ctypes as C
import pathlib, struct, subprocess, sys, tempfile
from verify_store_render_release import reader, SPECS, ROOT

MODES = {'mp':(0x6758BF4F,0x045AC000,0x3FA37C), 'zm':(0x6758BC50,0x045AA000,0x3FAD4C)}

def main():
    folder=pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tmp:
        source=pathlib.Path(tmp)/'probe.c'; so=pathlib.Path(tmp)/'probe.so'
        source.write_text('''#include "ui_build_default.h"
unsigned site(unsigned t,unsigned s) {const struct BuildNumberDefaultSite *p=T6_BuildNumberDefaultSite(t,s);return p?p->rva:0;}
int patch(unsigned t,unsigned s,unsigned char *p) {const struct BuildNumberDefaultSite *a=T6_BuildNumberDefaultSite(t,s);if(!a||!T6_BuildNumberDefaultMatches(p,a))return 0;p[T6_BUILD_NUMBER_DEFAULT_OFFSET]=0;return 1;}
''')
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(ROOT/'runtime/loader'),str(source),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));u=C.c_uint;lib.site.argtypes=[u,u];lib.site.restype=u;lib.patch.argtypes=[u,u,C.c_void_p];lib.patch.restype=C.c_int
        for mode,(timestamp,size,rva) in MODES.items():
            path=folder/('t6'+mode+'.exe');read=reader(path,SPECS[mode][0]);b=path.read_bytes();nt,=struct.unpack_from('<I',b,0x3c)
            assert struct.unpack_from('<I',b,nt+8)[0]==timestamp and struct.unpack_from('<I',b,nt+0x50)[0]==size
            original=bytes(read(rva,19));assert original[10:14]==b'\xc6\x45\xd7\x01'
            name=rva+10+struct.unpack_from('<i',original,6)[0]
            assert read(name,len(b'ui_drawBuildNumber\0'))==b'ui_drawBuildNumber\0'
            assert lib.site(timestamp,size)==rva
            buf=C.create_string_buffer(original,19);assert lib.patch(timestamp,size,buf)==1
            assert buf.raw==original[:13]+b'\0'+original[14:]
            for offset in range(19):
                for bit in range(8):
                    bad=bytearray(original);bad[offset]^=1<<bit;buf=C.create_string_buffer(bytes(bad),19)
                    assert lib.patch(timestamp,size,buf)==0 and buf.raw==bytes(bad)
            for bad_t,bad_s in ((timestamp^1,size),(timestamp,size^1),(0,0)):
                buf=C.create_string_buffer(original,19);assert lib.site(bad_t,bad_s)==0 and lib.patch(bad_t,bad_s,buf)==0 and buf.raw==original
            print(mode, 'default immediate and 155 rejection cases passed')
if __name__=='__main__':main()
