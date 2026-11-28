"""Tiger/192 (Anderson and Biham), with the S-boxes read from the Store exe (libtomcrypt's table, Store MP 1408D1570):
the lobby IV the client derives with bdCryptoUtils::calculateInitialVector, for the host build's tests."""
import struct

SBOX_VA, IMAGE_BASE = 0x1408D1570, 0x140000000
MASK = (1 << 64) - 1


def load_sboxes(exe):
    import pefile
    pe = pefile.PE(exe, fast_load=True)
    data = pe.get_data(SBOX_VA - IMAGE_BASE, 4 * 256 * 8)
    t = struct.unpack("<1024Q", data)
    assert t[0] == 0x02AAB17CF7E90C5E, "not the Tiger S-boxes"
    return t[0:256], t[256:512], t[512:768], t[768:1024]


def tiger(data, boxes):
    t1, t2, t3, t4 = boxes

    def rnd(a, b, c, x, mul):
        c ^= x
        a = (a - (t1[c & 0xFF] ^ t2[(c >> 16) & 0xFF] ^ t3[(c >> 32) & 0xFF] ^ t4[(c >> 48) & 0xFF])) & MASK
        b = (b + (t4[(c >> 8) & 0xFF] ^ t3[(c >> 24) & 0xFF] ^ t2[(c >> 40) & 0xFF] ^ t1[(c >> 56) & 0xFF])) & MASK
        return a, (b * mul) & MASK, c

    def passes(a, b, c, x, mul):
        a, b, c = rnd(a, b, c, x[0], mul)
        b, c, a = rnd(b, c, a, x[1], mul)
        c, a, b = rnd(c, a, b, x[2], mul)
        a, b, c = rnd(a, b, c, x[3], mul)
        b, c, a = rnd(b, c, a, x[4], mul)
        c, a, b = rnd(c, a, b, x[5], mul)
        a, b, c = rnd(a, b, c, x[6], mul)
        b, c, a = rnd(b, c, a, x[7], mul)
        return a, b, c

    def schedule(x):
        x = list(x)
        x[0] = (x[0] - (x[7] ^ 0xA5A5A5A5A5A5A5A5)) & MASK
        x[1] ^= x[0]
        x[2] = (x[2] + x[1]) & MASK
        x[3] = (x[3] - (x[2] ^ ((~x[1] & MASK) << 19 & MASK))) & MASK
        x[4] ^= x[3]
        x[5] = (x[5] + x[4]) & MASK
        x[6] = (x[6] - (x[5] ^ ((~x[4] & MASK) >> 23))) & MASK
        x[7] ^= x[6]
        x[0] = (x[0] + x[7]) & MASK
        x[1] = (x[1] - (x[0] ^ ((~x[7] & MASK) << 19 & MASK))) & MASK
        x[2] ^= x[1]
        x[3] = (x[3] + x[2]) & MASK
        x[4] = (x[4] - (x[3] ^ ((~x[2] & MASK) >> 23))) & MASK
        x[5] ^= x[4]
        x[6] = (x[6] + x[5]) & MASK
        x[7] = (x[7] - (x[6] ^ 0x0123456789ABCDEF)) & MASK
        return x

    state = (0x0123456789ABCDEF, 0xFEDCBA9876543210, 0xF096A5B4C3B2E187)
    msg = data + b"\x01" + b"\0" * ((56 - (len(data) + 1) % 64) % 64) + struct.pack("<Q", len(data) * 8)
    for off in range(0, len(msg), 64):
        x = struct.unpack("<8Q", msg[off:off + 64])
        a, b, c = state
        a, b, c = passes(a, b, c, x, 5)
        x = schedule(x)
        c, a, b = passes(c, a, b, x, 7)
        x = schedule(x)
        b, c, a = passes(b, c, a, x, 9)
        state = (a ^ state[0], (b - state[1]) & MASK, (c + state[2]) & MASK)
    return struct.pack("<3Q", *state)
