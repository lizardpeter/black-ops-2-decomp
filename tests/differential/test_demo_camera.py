"""demo/demo_camera.cpp and demo/demo_playback.cpp (server; the Store build inlines them all, covered by
CL_CreateCmd's Store test)."""
import struct

from unicorn import x86_const as x86

from harness import OBJECTS

SERVER = ["CoDMPServer_PC"]
DEMO = 0x080D3580             # demoMain: state +0x84, playback +0xB0680, playbackInited +0xB0684
CG_ENTITIES = 0x011ADC30      # cg_entitiesArray[0]
CG_AI_TYPES = 0x011B7588 + 0x3E1C  # cg_level.aiTypes
CL_MAX_LOCAL_CLIENTS, CLIENTS_PTR = 0x01237580, 0x01237588
VIEWANGLES = 0x2C8C
# demoPlayback is 0x26A980 bytes: its pointer is placed so the two fields read land in the objects region
CAMERA_MODE = OBJECTS + 0x188000
PLAYBACK = CAMERA_MODE - 0xD4C00
LOCKED = PLAYBACK + 0xEBCB4   # freeCameraLockedEntity, then freeCameraLockedOnEntityActive
ENTITIES = OBJECTS + 0x1A0000  # centity_t[], 0x380 apart
AI_TYPES = OBJECTS + 0x1B0000  # AITypeScript objects, 0x80 apart
CENT = 0x380
ET_SCRIPTMOVER, ET_HELICOPTER, ET_ACTOR, ET_ACTOR_CORPSE = 6, 12, 16, 18
STATE = {}


def demo(state=2, inited=1, mode=2, locked=-1, active=1, playback=PLAYBACK):
    def run(w):
        w.write(DEMO + 0x84, struct.pack("<i", state))
        w.u32(DEMO + 0xB0680, playback)
        w.write(DEMO + 0xB0684, bytes([inited]))
        w.write(CAMERA_MODE, struct.pack("<i", mode))
        w.write(LOCKED, struct.pack("<iB", locked, active))
    return run


def entity(w, index, etype, valid=1, actor_ai=0, mover_ai=0):
    base = ENTITIES + index * CENT
    w.write(base + 0x246, struct.pack("<H", actor_ai))
    w.write(base + 0x24E, struct.pack("<H", mover_ai))
    w.write(base + 0x2B4, struct.pack("<h", etype))
    w.write(base + 0x378, struct.pack("<I", 0x2 if valid else 0x1))


def ai_type(w, index, views):
    """cg_level.aiTypes[index] -> an AITypeScript whose demoLockOn views are ((pitchMin, pitchMax), ...)."""
    obj = AI_TYPES + index * 0x80
    w.write(obj + 0x24, struct.pack("<f", 300.0) + b"".join(struct.pack("<iff", 5, lo, hi) for lo, hi in views))
    w.u32(CG_AI_TYPES + 4 * index, obj)


def world(pitch=30.0):
    """Entities 0-9 of every kind, with AI types 3 (two views) and 7 (unset)."""
    def run(w):
        w.u32(CG_ENTITIES, ENTITIES)
        w.u32(CL_MAX_LOCAL_CLIENTS, 1)
        w.u32(CLIENTS_PTR, OBJECTS)
        w.write(OBJECTS + VIEWANGLES, struct.pack("<3f", pitch, 1.0, 2.0))
        ai_type(w, 3, ((-20.0, 45.0), (-5.0, 10.0)))
        ai_type(w, 0x103 & 0xFF, ((-20.0, 45.0), (-5.0, 10.0)))
        for i, (etype, valid, actor, mover) in enumerate((
                (ET_ACTOR, 1, 3, 0), (ET_ACTOR, 1, 7, 0), (ET_ACTOR, 1, 0, 0), (ET_HELICOPTER, 1, 0, 0),
                (1, 1, 3, 3), (ET_ACTOR, 0, 3, 0), (ET_SCRIPTMOVER, 1, 0, 3), (ET_ACTOR_CORPSE, 1, 3, 0),
                (ET_ACTOR, 1, 0x103, 0), (ET_SCRIPTMOVER, 1, 3, 7))):
            entity(w, i, etype, valid, actor, mover)
    return run


def both(*fns):
    def run(w):
        for f in fns:
            f(w)
    return run


def legs(mu):
    return STATE.get("legs", 0)


FAKES = {"Assert_MyHandler": lambda mu: 1, "Com_Error": lambda mu: None,
         "BG_AnimStateDef_InUse": lambda mu: STATE.get("anim", 0),
         "BG_AnimStateDef_IsMissingLegs": legs}
SEEN = {"Assert_MyHandler": "siiss", "Com_Error": "is", "BG_AnimStateDef_IsMissingLegs": "q"}


def mode_inputs():
    return [{"args": [], "setup": demo(state, inited, mode, locked, active)}
            for state in (2, 1, 0) for inited in (1, 0) for mode in (0, 1, 2, 3)
            for locked, active in ((-1, 1), (4, 1), (4, 0), (0, 1), (-2, 1))]


def with_state(fn, **state):
    def run(w):
        STATE.clear()
        STATE.update(state)
        fn(w)
    return run


def lock_on_inputs():
    out = []
    for i in range(10):
        for anim, legs_ in ((0, 0), (1, 0), (1, 1), (0, 1)):
            for pitch in (30.0, -50.0, 80.0, -100.0, -10.0, float("nan")):
                out.append({"args": [0], "setup": with_state(both(world(pitch), demo(locked=i)), anim=anim,
                                                             legs=legs_)})
    # not locked on: the assert fires and the entity is -1's
    out.append({"args": [0], "setup": with_state(both(world(), demo(locked=-1)))})
    return out


def entity_inputs():
    """An entity pointer, in the register the original takes it in."""
    out = []
    for i in range(10):
        for anim, legs_ in ((0, 0), (1, 0), (1, 1)):
            out.append({"args": [ENTITIES + i * CENT], "setup": with_state(world(), anim=anim, legs=legs_)})
    out.append({"args": [0], "setup": with_state(world())})  # the "(cent)" assert, then a read at 0x246: faults
    return out


def get_entity_inputs():
    """CG_GetEntity (cgame/cg_local.h; reached here through the theater's lock-on): the dedicated server's build always
    calls Com_Error first. Out-of-range indexes assert."""
    return [{"args": [lc, i], "setup": with_state(world())} for lc in (0, 1) for i in (0, 5, 0x6FF, 0x700, -1)]


CASES = [
    {"name": "CG_GetEntity", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": get_entity_inputs()},
    {"name": "Demo_IsPlaybackInited", "ret": "bool", "builds": SERVER, "inputs": mode_inputs()},
    {"name": "Demo_IsThirdPersonCamera", "ret": "bool", "builds": SERVER, "inputs": mode_inputs()},
    {"name": "Demo_IsAnyFreeMoveCamera", "ret": "bool", "builds": SERVER, "inputs": mode_inputs()},
    {"name": "Demo_IsFreeCameraLockedOnEntity", "ret": "bool", "builds": SERVER, "inputs": mode_inputs()},
    {"name": "Demo_GetAILockOnValues", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": entity_inputs()},
    {"name": "Demo_GetAILockOnViewIndex", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": entity_inputs()},
    {"name": "Demo_ClampViewAngleForLockedOnEntity", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": lock_on_inputs()},
]
