// Source reconstruction of FUN_18002EE30, a Bink audio-transform rotation
// stage, from original Store binkw64.dll scalar SSE machine instructions.
// Original DLL SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// No external codec source or decoder dependency is used.
// Requires valid non-overlapping arrays, count >= 6, and sufficient coefficient
// entries; those constraints are properties of its original internal callers,
// not runtime checks inserted by this leaf. Signed 32-bit arithmetic in the
// original quotient must be used if out-of-range values are ever tested.
#include <cstdint>

extern "C" void bink_transform_rotate_stage(int count,
                                              float* samples,
                                              int table_length,
                                              const float* coefficients) {
    const int half_count = count >> 1;
    const int quotient = (2 * table_length) / half_count;
    if (half_count <= 2) return;

    const float* coeff_lo = coefficients;
    const float* coeff_hi = coefficients + table_length;
    float* right = samples + count - 2;
    for (int i = 2; i < half_count; i += 2) {
        const float left_first = samples[i];
        const float left_second = samples[i + 1];
        coeff_lo += quotient;
        coeff_hi -= quotient;

        // Original RCX is decremented by 8 bytes before loads at RCX+8
        // and RCX+12: the first pair is samples[count-2 : count], NOT
        // samples[count-4 : count-2].
        const float diff = left_first - right[0];        // subss
        const float sum = left_second + right[1];        // addss
        const float secondary = 0.5f - *coeff_hi;        // subss
        const float p0 = diff * *coeff_lo;               // mulss
        const float p1 = sum * *coeff_lo;                // mulss
        const float p2 = diff * secondary;               // mulss
        const float p3 = sum * secondary;                // mulss
        const float rot_first = p2 - p1;                 // subss
        const float rot_second = p0 + p3;                // addss

        samples[i] = left_first - rot_first;
        samples[i + 1] = left_second - rot_second;
        right[1] -= rot_second;
        right[0] += rot_first;
        right -= 2;
    }
}

// Source reconstruction of the sibling audio rotation FUN_18002EF50.
// This differs from FUN_18002EE30 in the butterfly's sign/rotation order.
extern "C" void bink_transform_rotate_stage_alt(int count,
                                                  float* samples,
                                                  int table_length,
                                                  const float* coefficients) {
    const int half_count = count >> 1;
    const int quotient = (2 * table_length) / half_count;
    if (half_count <= 2) return;

    const float* coeff_lo = coefficients;
    const float* coeff_hi = coefficients + table_length;
    float* right = samples + count - 2;
    for (int i = 2; i < half_count; i += 2) {
        const float left_first = samples[i];
        const float left_second = samples[i + 1];
        coeff_lo += quotient;
        coeff_hi -= quotient;
        const float sum = left_second + right[1]; // addss
        const float diff = left_first - right[0]; // subss
        const float secondary = 0.5f - *coeff_hi;
        const float p0 = diff * *coeff_lo;
        const float p1 = sum * *coeff_lo;
        const float p2 = diff * secondary;
        const float p3 = sum * secondary;
        const float rot_first = p1 + p2;
        const float rot_second = p3 - p0;

        samples[i] = left_first - rot_first;
        samples[i + 1] = left_second - rot_second;
        right[1] -= rot_second;
        right[0] += rot_first;
        right -= 2;
    }
}
