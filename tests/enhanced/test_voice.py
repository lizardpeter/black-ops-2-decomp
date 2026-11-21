"""P0001 (B0002): voice chat leaves the Windows recording devices alone.

Reuses tests/differential/test_win_voice.py's mixer model and fakes. The enhanced functions must make no mixer call
and leave every mixer as it was; everything else (dvars, the voice back end, globals) must match the original,
apart from what the original only kept for the mixer (wavein_current_reclevel, s_waveInLineInfo).
"""
import test_win_voice as v

PATCH = "P0001"
BUILDS = ["CoDMPServer_PC"]
MIXER_STATE = range(v.MODEL, v.MIXERS + 0x20 * 16)

def qwords(lo, hi):
    """The harness reports writes as whole aligned qwords."""
    return set(range(lo & ~7, (hi + 7) & ~7))


MIXER_ONLY = qwords(v.G["wavein_current_reclevel"], v.G["wavein_current_reclevel"] + 4) | \
    qwords(v.G["s_waveInLineInfo"], v.G["s_waveInLineInfo"] + 0xA0)


def compare(ctx, name, inputs, ret, fakes=v.BACKEND_FAKES, observe=v.BACKEND_SEEN):
    ctx.imports(v.IMPORTS)
    fk, ob = ctx.hooks(fakes), ctx.hooks(observe)
    for i, inp in enumerate(inputs):
        want = ctx.m.call(ctx.original(name), inp["args"], ret, inp["setup"], fk, ob)
        got = ctx.m.call(ctx.enhanced(name), inp["args"], ret, inp["setup"], fk, ob)
        assert not isinstance(got[0], str), f"input {i}: {got[0]}"
        winmm = [n for n in ctx.m.imports_called if n.lower().startswith("winmm.dll!")]
        assert not winmm, f"input {i}: called {winmm}"
        touched = sorted(a for a in got[2] if a in MIXER_STATE)
        assert not touched, f"input {i}: changed the mixer model at {[hex(a) for a in touched[:4]]}"
        assert got[0] == want[0], f"input {i}: returned {got[0]!r}, original {want[0]!r}"
        assert got[3] == want[3], f"input {i}: calls differ\n  original {want[3]}\n  enhanced {got[3]}"
        rest = lambda w: {a: b for a, b in w.items() if a not in MIXER_STATE and a not in MIXER_ONLY}
        diff = sorted(a for a in set(rest(got[2])) | set(rest(want[2])) if got[2].get(a) != want[2].get(a))
        assert not diff, f"input {i}: other globals differ at {[hex(a) for a in diff[:4]]}"


def check_init_leaves_mixers(ctx):
    compare(ctx, "Voice_Init", v.init_inputs(), "bool")


def check_playback_leaves_mixers(ctx):
    compare(ctx, "Voice_Playback", v.playback_inputs(), "void")


def check_shutdown_leaves_mixers(ctx):
    compare(ctx, "Voice_Shutdown", v.shutdown_inputs(), "void")


def check_mute_is_in_game(ctx):
    """winvoice_mic_mute: never talking while set; otherwise as the original."""
    fk = ctx.hooks(v.BACKEND_FAKES)
    for mute in (0, 1):
        for a in (0, 1):
            for b in (0, 1):
                for talk in (0, 1):
                    s = v.setup(talk=talk, dvars={"winvoice_mic_mute": mute},
                                globals={"g_voice_initialized": a, "g_voice_record_initialized": b})
                    want = ctx.m.call(ctx.original("Voice_IsLocalClientTalking"), [0, 0], "bool", s, fk)
                    got = ctx.m.call(ctx.enhanced("Voice_IsLocalClientTalking"), [0, 0], "bool", s, fk)
                    expect = 0 if mute else want[0]
                    assert got[0] == expect, f"mute={mute} init={a},{b} talk={talk}: {got[0]}, expected {expect}"


CHECKS = [check_init_leaves_mixers, check_playback_leaves_mixers, check_shutdown_leaves_mixers, check_mute_is_in_game]
