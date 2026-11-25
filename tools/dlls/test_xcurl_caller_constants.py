#!/usr/bin/env python3
"""Exercise branches and Win64 call clobbers without original files."""
from probe_xcurl_setopt import decoder,constant_arg,constant_args

def check(code,want,idx=-1):
    ins=list(decoder().disasm(bytes.fromhex(code),0x1000))
    k=idx if idx>=0 else len(ins)-1
    got=constant_arg(ins,k)
    assert got==want,(code,want,got,[(x.mnemonic,x.op_str) for x in ins])

# mov ebx,60; call; mov edx,ebx; call
check("bb3c000000 e800000000 89da e800000000",60)
# mov edx,60; call; call (volatile is no longer known)
check("ba3c000000 e800000000 e800000000",None)
# mov edx,44; test eax,eax; je +2; nop;nop;call
check("ba2c000000 85c0 7402 9090 e800000000",44)
# differing incoming values (branch skips second mov)
check("ba2c000000 85c0 7405 ba2e000000 e800000000",None)
# both paths same value
check("ba2c000000 85c0 7405 ba2c000000 e800000000",44)
# partial register overwrite loses proof
check("ba2c000000 b201 e800000000",None)
# lea edx,[rbx+2] preserves the recovered constant
check("bb2a000000 8d5302 e800000000",44)
# indirect branch refuses analysis
check("ba2c000000 ffe0 e800000000",None)
# Two legitimate constants are preserved, not incorrectly collapsed.
ins=list(decoder().disasm(bytes.fromhex("ba2c000000 85c0 7405 ba2e000000 e800000000"),0x1000))
assert constant_args(ins,len(ins)-1)==[44,46]
# cmp rcx,4; jne other; lea edi,[rcx+42]; jmp merge; mov edi,44; mov edx,edi; call
ins=list(decoder().disasm(bytes.fromhex("4883f904 7505 8d792a eb05 bf2c000000 89fa e800000000"),0x1000))
assert constant_args(ins,len(ins)-1)==[44,46]
print("XCurl caller constant propagation PASS: 10 control-flow cases")
