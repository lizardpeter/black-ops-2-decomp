// Basic numerical smoke test. The original-vs-source byte comparison is
// tests/dlls/differential_butterfly.py, not this test.
#include <cassert>
#include "transform_butterfly8.cpp"

int main() {
    float values[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    bink_transform_butterfly8(values);
    const float expected[8] = {16, 20, -8, 0, -4, -4, 0, -8};
    for (int i = 0; i < 8; ++i) assert(values[i] == expected[i]);
    float zeros[8] = {};
    bink_transform_butterfly8(zeros);
    for (auto v : zeros) assert(v == 0);
}
