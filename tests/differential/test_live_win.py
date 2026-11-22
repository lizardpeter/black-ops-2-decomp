"""live/live_win.cpp (server; the Store revision's per-controller check is inlined, covered by CL_CreateCmd's Store
test)."""
SERVER = ["CoDMPServer_PC"]
SYSTEM_UI_ACTIVE = 0x08031459  # s_systemUiActive


CASES = [
    {"name": "Live_IsSystemUiActive", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [], "setup": (lambda v: lambda w: w.write(SYSTEM_UI_ACTIVE, bytes([v])))(v)}
                for v in (0, 1)]},
]
