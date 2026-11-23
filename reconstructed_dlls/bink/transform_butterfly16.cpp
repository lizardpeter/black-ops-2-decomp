// Direct reconstruction of original Bink 16-point transform kernel,
// FUN_18002E630 (RVA 0x2E630) from pinned binkw64.dll.
// SHA-256: 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// This pure floating-point kernel has no transitive decoder dependencies.
// Preserve sequential float operations and compile without FP reassociation
// or FMA contraction for bitwise comparison with original SSE instructions.
// x: 16 writable floats; coefficients: >=2 readable floats, [1] is used.
extern "C" void bink_transform_butterfly16(float* x, const float* coefficients) {
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10;
    float f11, f12, f13, f14, f15, f16, f17, f18, f19;
    f8 = x[1] - x[9];
    f7 = x[0] - x[8];
    f4 = x[4] + x[12];
    f5 = x[4] - x[12];
    f10 = x[1] + x[9];
    f12 = x[0] + x[8];
    f14 = x[3] - x[11];
    f3 = x[5] + x[13];
    f19 = x[5] - x[13];
    f17 = x[3] + x[11];
    f1 = f4 + f12;
    f12 = f12 - f4;
    f2 = f3 + f10;
    f10 = f10 - f3;
    f16 = f5 + f8;
    f8 = f8 - f5;
    f3 = x[7] + x[15];
    f5 = x[6] + x[14];
    f9 = x[7] - x[15];
    f6 = x[6] - x[14];
    f11 = x[2] - x[10];
    f18 = x[2] + x[10];
    f4 = f11 - f9;
    f9 = f9 + f11;
    f15 = f7 - f19;
    f19 = f19 + f7;
    f13 = f3 + f17;
    f7 = f6 + f14;
    f17 = f17 - f3;
    f14 = f14 - f6;
    f11 = f5 + f18;
    f18 = f18 - f5;
    const float scalar = coefficients[1];
    f6 = (f7 + f4) * scalar;
    f5 = (f9 - f14) * scalar;
    f4 = (f4 - f7) * scalar;
    f3 = (f14 + f9) * scalar;
    x[8] = f4 + f15;
    x[9] = f6 + f16;
    x[12] = f19 - f3;
    x[15] = f8 - f5;
    x[13] = f5 + f8;
    x[0] = f11 + f1;
    x[2] = f1 - f11;
    x[3] = f2 - f13;
    x[1] = f13 + f2;
    x[10] = f15 - f4;
    x[6] = f17 + f12;
    x[11] = f16 - f6;
    x[14] = f3 + f19;
    x[4] = f12 - f17;
    x[5] = f18 + f10;
    x[7] = f10 - f18;
}
