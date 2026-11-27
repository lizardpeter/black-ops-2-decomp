"""Store CL_MapLoading_SetupDevmapParty: devmap party setup, against the original with its callees observed."""

import itertools

SESSION_MODE = 0x143ED9A14
PARTY_FLAGS = 0x143ED8224
CLIENT_SLOT = 0x141EF91E8
BACKUP_BLOCK = 0x141EF8675


def state(mode, party, backup):
    def run(w):
        w.u32(SESSION_MODE, mode)
        w.u32(PARTY_FLAGS, party)
        w.u32(CLIENT_SLOT, 0x7FFF)
        w.u8(BACKUP_BLOCK, backup)
    return run


CASES = [
    {"name": "CL_MapLoading_SetupDevmapParty", "ret": "void", "builds": ["t6mp_msstore"],
     "fakes": {"StoreSetControllerInUse": lambda mu: 0, "sub_1406F52E0": lambda mu: 0,
               "sub_1405A68D0": lambda mu: 0, "sub_1401B3900": lambda mu: 0, "Com_Error": lambda mu: 0},
     "observe": {"StoreSetControllerInUse": "ii", "sub_1406F52E0": "ii", "sub_1405A68D0": "i",
                 "sub_1401B3900": "s", "Com_Error": "is"},
     "inputs": [{"args": [b"mp_raid\0"], "setup": state(mode, party, backup)}
                for mode, party, backup in itertools.product((0, 1, 2, 4, 6, 0x80), (0, 4, 0xFB), (0, 1))]},
]
