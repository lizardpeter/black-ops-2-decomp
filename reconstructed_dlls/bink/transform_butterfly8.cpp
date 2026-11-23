// Direct source reconstruction of one original Bink audio/transform path
// numeric kernel, FUN_18002ED50 (RVA 0x2ED50). Original binkw64.dll SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Eight scalar SSE ADDSS/SUBSS operations in their actual dependency order.
// This is a genuine machine-code-derived codec kernel, NOT an existing decoder
// implementation. Not yet integrated into the larger Bink audio decoder.
// Compile without -ffast-math or floating-point reassociation for comparison.
extern "C" void bink_transform_butterfly8(float* x) {
    // All inputs captured before output stores, matching original XMM registers.
    const float x0 = x[0], x1 = x[1], x2 = x[2], x3 = x[3];
    const float x4 = x[4], x5 = x[5], x6 = x[6], x7 = x[7];
    const float diff26 = x2 - x6; // XMM6
    const float sum04  = x0 + x4; // XMM7
    const float diff15 = x1 - x5; // XMM8
    const float diff04 = x0 - x4; // XMM9
    const float sum26  = x2 + x6; // XMM3
    const float sum37  = x3 + x7; // XMM2
    const float sum15  = x1 + x5; // XMM5
    const float diff37 = x3 - x7; // XMM4

    x[0] = sum26 + sum04;
    x[1] = sum37 + sum15;
    x[4] = sum04 - sum26;
    x[2] = diff37 + diff04;
    x[3] = diff15 - diff26;
    x[7] = diff26 + diff15;
    x[6] = diff04 - diff37;
    x[5] = sum15 - sum37;
}
