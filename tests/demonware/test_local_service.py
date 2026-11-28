"""The enhanced build's in-process Demonware service (enhanced/demonware/local_service.cpp) against the standalone
service it replaced (services/demonware): the protocol core built for the host, replaying parity_vectors.json (what
the standalone service does on the same lobby traffic, written by services/demonware/parity_test.go), plus the
checks of the standalone service's own tests that the vectors don't cover (auth tickets and their one use, the
publisher folder).

    python3 tests/demonware/test_local_service.py originals/t6mp_msstore.exe
"""
import ctypes as C
import hashlib
import hmac
import json
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

from cryptography.hazmat.decrepit.ciphers.algorithms import TripleDES
from cryptography.hazmat.primitives.ciphers import Cipher, modes

import tiger

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
LOCAL_USER = 0x2100000000000001
U8P = C.POINTER(C.c_ubyte)


class Platform:
    """What the Windows side provides, in Python: CNG's Triple DES and HMAC-SHA1, the client's Tiger IV, a queue of
    random bytes, the clocks, the profile's files and a publisher folder."""

    def __init__(self, boxes):
        self.boxes = boxes
        self.random = bytearray()
        self.unix, self.ms = 1_700_000_000, 0
        self.profile, self.publisher, self.folder = {}, {}, False
        self.traces = []

    def des(self, encrypt, key, iv, data, size):
        buf = bytes(data[:size])
        c = Cipher(TripleDES(bytes(key[:24])), modes.CBC(bytes(iv[:8])))
        out = (c.encryptor() if encrypt else c.decryptor()).update(buf)
        C.memmove(data, out, size)
        return 1

    def hmac(self, key, n, data, size, out):
        C.memmove(out, hmac.new(bytes(key[:n]), bytes(data[:size]), hashlib.sha1).digest(), 20)
        return 1

    def iv(self, seed, out):
        C.memmove(out, tiger.tiger(struct.pack("<I", seed), self.boxes), 24)

    def rand(self, out, size):
        if len(self.random) < size:
            self.random += b"\x5a" * (size - len(self.random))
        C.memmove(out, bytes(self.random[:size]), size)
        del self.random[:size]
        return 1

    def clock(self, monotonic):
        return self.ms if monotonic else self.unix

    def read(self, publisher, name, out, capacity):
        files = self.publisher if publisher else self.profile
        data = files.get(C.string_at(name).decode("latin-1"))
        if data is None:
            return -1
        if len(data) > capacity:
            return -2
        C.memmove(out, data, len(data))
        return len(data)

    def write(self, name, data, size):
        self.profile[C.string_at(name).decode("latin-1")] = bytes(data[:size]) if size else b""
        return 1


def build(tmp):
    so = Path(tmp) / "local_service.so"
    subprocess.run(["clang++", "-std=c++17", "-O1", "-fPIC", "-shared", "-DT6_HOST_TEST", "-Wall", "-Werror",
                    f"-I{ROOT / 'include'}", str(ROOT / "enhanced/demonware/local_service.cpp"),
                    str(HERE / "host_shim.cpp"), "-o", str(so)], check=True)
    return C.CDLL(str(so))


class Service:
    def __init__(self, lib, platform):
        self.lib, self.p = lib, platform
        types = [C.CFUNCTYPE(C.c_int, C.c_int, U8P, U8P, U8P, C.c_uint),
                 C.CFUNCTYPE(C.c_int, U8P, C.c_uint, U8P, C.c_uint, U8P),
                 C.CFUNCTYPE(None, C.c_uint, U8P),
                 C.CFUNCTYPE(C.c_int, U8P, C.c_uint),
                 C.CFUNCTYPE(C.c_ulonglong, C.c_int),
                 C.CFUNCTYPE(None, C.c_char_p),
                 C.CFUNCTYPE(C.c_int, C.c_int, C.c_char_p, U8P, C.c_uint),
                 C.CFUNCTYPE(C.c_int, C.c_char_p, U8P, C.c_uint),
                 C.CFUNCTYPE(C.c_int)]
        fns = [platform.des, platform.hmac, platform.iv, platform.rand, platform.clock,
               lambda e: platform.traces.append(e), platform.read, platform.write, lambda: platform.folder]
        self.callbacks = [t(f) for t, f in zip(types, fns)]  # kept alive
        lib.Setup(*self.callbacks)
        lib.TestAnswerAuth.restype = C.c_uint
        lib.TestHandleFrame.restype = C.c_int
        self.reply = C.create_string_buffer(65535 + 8)

    def auth(self, title, seed=0):
        out = C.create_string_buffer(0x1400)
        n = self.lib.TestAnswerAuth(title, seed, out, len(out))
        return out.raw[:n].decode() if n else None

    def handshake(self, hello, frame):
        session, ack = C.create_string_buffer(32), C.create_string_buffer(15)
        ok = self.lib.TestHandshake(hello, frame, len(frame), session, ack)
        return (session, ack.raw) if ok else (None, None)

    def frame(self, session, frame, seed):
        self.p.random = bytearray(seed)  # the reply's seed (frames that get no reply use none)
        n = self.lib.TestHandleFrame(session, frame, len(frame), self.reply)
        return "close" if n < 0 else self.reply.raw[:n].hex()


def decrypt_reply(key, reply):
    seed, data = reply[5:9], reply[9:]
    return Cipher(TripleDES(key), modes.CBC(tiger.tiger(seed, BOXES)[:8])).decryptor().update(data)


def bundle_entries(file):
    magic, _, count, version = struct.unpack_from("<4I", file)
    assert magic == 0x543377AB and version == 0xFFFFFFFF, file[:16].hex()
    out = []
    for i in range(count):
        record = file[16 + 44 * i:60 + 44 * i]
        size, length, offset = struct.unpack(">3I", record[32:44])
        text = zlib.decompress(file[offset:offset + size])
        assert len(text) == length
        out.append({"name": record[:32].rstrip(b"\0").decode(), "text": text.decode()})
    return out


def check_vectors(svc, vectors):
    key = bytes.fromhex(vectors["key"])
    # the lobby auth frames: a ticket the service issued, then the hello and the frame
    hello = bytes([200, 0, 0, 0, 255, 255, 0, 0])
    for v in vectors["auth"]:
        payload = bytes.fromhex(v["payload"])
        # (a title of 0 decodes, but neither service signs in without a title, so there's no ticket for it)
        opens = bool(v["ticket"]) and v["title"] != 0
        if opens:
            svc.p.random += bytes(24) + bytes.fromhex(v["ticket"])
            assert svc.auth(v["title"]), "the service issued no ticket"
        session, ack = svc.handshake(hello, payload)
        assert (session is not None) == opens, v
        if session is not None:
            assert ack[:7] == bytes([11, 0, 0, 0, 0, 4, 10]), ack.hex()
            assert svc.handshake(hello, payload)[0] is None, "a ticket was accepted twice"
    session = C.create_string_buffer(struct.pack("<Q", 1) + key)
    for v in vectors["frames"]:
        got = svc.frame(session, bytes.fromhex(v["frame"]), bytes.fromhex(v["seed"]))
        want = v["reply"]
        if v["kind"] == "bytes" or want in ("close", "") or got in ("close", ""):
            assert got == want, (v["name"], got[:120], want[:120])
            continue
        g, w = decrypt_reply(key, bytes.fromhex(got)), decrypt_reply(key, bytes.fromhex(want))
        if v["kind"] == "time":
            assert g[:-8] == w[:-8] and struct.unpack_from("<I", g, len(g) - 8)[0] == svc.p.unix, v["name"]
        else:  # the bundle: the same entries and contents, compressed differently
            name = v["name"].split()[1]
            expect = next(b for b in vectors["bundles"] if b["name"] == f"online_tu17_{name}_english.wad")
            for reply in (g, w):
                assert reply[:21] == w[:21] and reply[21:31] == bytes([8, 1, 0, 0, 0, 8, 1, 0, 0, 0]), reply[:40].hex()
                size = struct.unpack_from("<I", reply, 33)[0]
                assert bundle_entries(reply[37:37 + size]) == expect["entries"], v["name"]
    print(f"parity: {len(vectors['auth'])} auth frames and {len(vectors['frames'])} lobby frames as the standalone "
          "service", flush=True)


def check_auth(svc):
    """services/demonware/main_test.go and lobby_test.go's checks of the tickets."""
    response = json.loads(svc.auth(2410, 21))
    assert (response["auth_task"], response["code"], response["iv_seed"]) == (45, 700, 21), response
    import base64
    client, server = base64.b64decode(response["client_ticket"]), base64.b64decode(response["server_ticket"])
    assert len(client) == len(server) == 128
    assert client[:4] == bytes([0xDE, 0xAD, 0xBD, 0xEF]) and struct.unpack_from("<I", client, 5)[0] == 2410
    assert struct.unpack_from("<Q", client, 25)[0] == LOCAL_USER and client[33:45] == b"Local Player"
    assert json.loads(response["extra_data"]) == {"sbx": "RETAIL", "dpi": "LOCAL"}
    assert svc.auth(0) is None, "a sign-in without a title was answered"

    # a ticket opens the lobby once, for its title, within a minute
    def frame_for(title, ticket):
        bits = bytearray(138)

        def put(start, count, value):
            for b in range(count):
                if value >> b & 1:
                    bits[(start + b) // 8] |= 1 << ((start + b) % 8)
        put(0, 1, 1), put(1, 5, 8), put(6, 32, title), put(38, 5, 8)
        for i, byte in enumerate(ticket):
            put(75 + 8 * i, 8, byte)
        return bytes([0, 7]) + bytes(bits)
    hello = bytes([200, 0, 0, 0, 255, 255, 0, 0])
    for case, title, delay in (("used", 2410, 0), ("wrong title", 2405, 0), ("expired", 2410, 60_000)):
        ticket = bytes([0x33]) * 128
        svc.p.random += bytes(24) + ticket
        assert svc.auth(2410)
        svc.p.ms += delay
        session, _ = svc.handshake(hello, frame_for(title, ticket))
        assert (session is not None) == (case == "used"), case
    assert svc.handshake(bytes(8), frame_for(2410, bytes(128)))[0] is None, "a wrong hello was accepted"
    # at most 128 sign-ins wait at once (expired ones don't count)
    svc.p.ms += 61_000
    issued = sum(1 for i in range(130) if svc.auth(2410))
    assert issued == 128, issued
    svc.p.ms += 61_000
    assert svc.auth(2410), "expired sign-ins were still counted"
    print("auth: tickets, their one use, title, expiry and the 128 pending limit as the standalone service",
          flush=True)


def check_publisher_folder(svc, key):
    """services/demonware/tasks_test.go's TestPublisherFileReply: a folder of publisher files in place of the
    generated bundles."""
    svc.p.folder = True
    svc.p.publisher = {"startup.wad": bytes([42, 53]), "big.wad": bytes(65492), "online_tu17_mp_english.wad": b"x"}
    session = C.create_string_buffer(struct.pack("<Q", 1) + key)

    def get(name):
        plain = bytes([0x0A, 3, 7, 16]) + name.encode() + b"\0"
        plain = bytes(4) + plain + bytes(-(4 + len(plain)) % 8)
        mac = hmac.new(key, plain[5:], hashlib.sha1).digest()[:4]
        seed = bytes([1, 2, 3, 4])
        data = Cipher(TripleDES(key), modes.CBC(tiger.tiger(seed, BOXES)[:8])).encryptor().update(mac + plain[4:])
        reply = decrypt_reply(key, bytes.fromhex(svc.frame(session, b"\x01" + seed + data, seed)))
        status = struct.unpack_from("<I", reply, 15)[0]
        return status, reply[31:37 + struct.unpack_from("<I", reply, 33)[0]] if status == 0 else None
    assert get("startup.wad") == (0, bytes([19, 8, 2, 0, 0, 0, 42, 53]))
    assert get("online_tu17_mp_english.wad") == (0, bytes([19, 8, 1, 0, 0, 0]) + b"x"), "the bundle wasn't the folder's"
    for name in ("missing.wad", "../startup.wad", "..\\startup.wad", "a:b", "..", "NUL", "con.txt"):
        assert get(name)[0] == 1000, name
    assert get("big.wad")[0] == 108, "an oversized file was served"
    svc.p.folder = False
    print("publisher folder: files by name, no paths or device names, the size limit", flush=True)


def main(exe):
    global BOXES
    BOXES = tiger.load_sboxes(exe)
    vectors = json.loads((HERE / "parity_vectors.json").read_text())
    with tempfile.TemporaryDirectory() as tmp:
        lib = build(tmp)
        svc = Service(lib, Platform(BOXES))
        check_vectors(svc, vectors)
        check_auth(svc)
        check_publisher_folder(svc, bytes.fromhex(vectors["key"]))
        assert svc.p.profile.get("a b/../c:d") == bytes([4]), "a profile file's name wasn't kept as given"


if __name__ == "__main__":
    main(sys.argv[1])
