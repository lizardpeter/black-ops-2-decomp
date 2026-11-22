"""client/cl_keys.cpp (server; the Store build inlines Key_IsCatcherActive, covered by CL_CreateCmd's Store test)."""
import struct

SERVER = ["CoDMPServer_PC"]
CLIENT_UI = 0x01237050  # clientUIActives[0]: keyCatchers at +4


def catchers(v):
    return lambda w: w.write(CLIENT_UI + 4, struct.pack("<i", v))


CASES = [
    {"name": "Key_IsCatcherActive", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [lc, mask], "setup": catchers(kc)}
                for lc in (0, -1) for mask in (1, 4, 0x10, 0x48, -1) for kc in (0, 4, 0x10, 0x40, 0x7FFFFFFF)]},
]
