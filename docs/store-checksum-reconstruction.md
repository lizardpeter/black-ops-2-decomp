# Checksum challenges (live/live_anticheat.cpp)

Demonware's anti-cheat can send a client a challenge: checksum this range of your own memory, CRC-32 or a "split"
CRC-32 (bytes at even and odd addresses into two halves of one 64-bit result), and send the answer back. The client
hashes the range a block (0x2000 bytes) per frame in one of sixteen contexts (`s_hashInfos`).

## Reconstructed and verified

| Function | Server | Store MP | Test |
|---|---|---|---|
| `LiveAntiCheat_GenerateCRC32Checksum` | `008CA570` | inlined in ContinueHash | server: `test_live_anticheat.py`; Store: through ContinueHash |
| `LiveAntiCheat_GenerateCRC32ChecksumSplit` | `008CA5B0` | inlined in ContinueHash | as above |
| `LiveAntiCheat_StartHash` | `008CA610` | `140576490` | `test_live_anticheat.py`, `tests/replay/verify_live_anticheat.py` |
| `LiveAntiCheat_ContinueHash` | `008CA700` | `140576540` | both |
| `LiveAntiCheat_CompleteHash` | `008CA860` | inlined in AnswerChecksumRequest | server only |

The Store code is the server's: the same free-slot search (NULL when all sixteen are busy or the type is unknown),
the same 32-bit address range (the context keeps 32-bit fields even in the x64 build, so a range above 4 GB can't be
named), the same split by address parity. The only difference is the compiler's: every caller passes a block size
of 0x2000, which it folded into StartHash.

`LiveAntiCheat_AnswerChecksumRequest` (Store `140576630`) and the rest of the challenge handling are identified but
not reconstructed. With official services cut off (P0010), nothing sends these challenges; they matter only if a
replacement server ever uses them.
