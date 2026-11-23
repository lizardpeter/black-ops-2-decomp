// Standalone numerical smoke tests of original SSE-derived rotation stage.
// Differential original DLL tests live in tests/dlls/differential_rotate.py.
#include <cassert>
#include <cmath>
#include "transform_rotate.cpp"

int main() {
    float samples[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    float coeff[33];
    for (auto& value : coeff) value = 0.25f;
    bink_transform_rotate_stage(8, samples, 32, coeff);
    assert(samples[2] == 5.5f && samples[3] == 1.5f);
    assert(samples[4] == 4.0f && samples[5] == 5.0f);
    assert(samples[6] == 2.5f && samples[7] == 5.5f);
    assert(samples[0] == 0.0f && samples[1] == 1.0f);
    float alt[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    bink_transform_rotate_stage_alt(8, alt, 32, coeff);
    assert(alt[2] == 0.5f && alt[3] == -0.5f);
    assert(alt[6] == 7.5f && alt[7] == 3.5f);
    assert(alt[0] == 0.0f && alt[1] == 1.0f);
}
