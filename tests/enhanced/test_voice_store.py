"""P0001 (B0002) in the Store build: Mixer_InitWaveInLineInfo saves no line, so no recording device is ever changed.

Reuses tests/differential/test_win_voice.py's mixer model (x64 there for the Store's faithful test).
"""
import test_win_voice as v

PATCH = "P0001"
BUILDS = ["t6mp_msstore"]
MIXER_STATE = range(v.MODEL, v.MIXERS + 0x20 * 16)
TABLE = v.STORE_G["s_waveInLineInfo"]


def check_no_line_is_saved(ctx):
    ctx.imports(v.IMPORTS)
    saved = 0
    for i in range(60):
        model = v.mixers(v.rng.randrange(0, 9), v.rng.choice([0, 1, 3]))
        setup = v.store_mixer_setup(model, [(1, 9, 9, 9, 9)] * 3)
        want = ctx.m.call(ctx.original("Mixer_InitWaveInLineInfo"), [], "void", setup)
        got = ctx.m.call(ctx.enhanced("Mixer_InitWaveInLineInfo"), [], "void", setup)
        assert not isinstance(got[0], str), f"input {i}: {got[0]}"
        winmm = [n for n in ctx.m.imports_called if n.lower().startswith("winmm.dll!")]
        assert not winmm, f"input {i}: called {winmm}"
        assert not [a for a in got[2] if a in MIXER_STATE], f"input {i}: changed the mixer model"
        # the setup leaves three valid lines in the table: the enhanced function clears them and saves none
        table = bytes(got[2].get(a, b"\x01" if (a - TABLE) % 20 == 0 and a < TABLE + 60 else b"\0")[0]
                      for a in range(TABLE, TABLE + 0xA0))
        assert table == bytes(0xA0), f"input {i}: a line was saved or kept"
        saved += not isinstance(want[0], str) and bool(v.lines_for(model))
    assert saved > 10, f"the original saved lines in only {saved} inputs: no test"


CHECKS = [check_no_line_is_saved]
