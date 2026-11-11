#include "t6_recon.h"

int16_t ShortSwap(int16_t value)
{
    uint16_t bits = (uint16_t)value;
    uint16_t swapped = (uint16_t)((bits << 8) | (bits >> 8));
    if (swapped <= INT16_MAX)
        return (int16_t)swapped;
    return (int16_t)(-1 - (UINT16_MAX - swapped));
}

int16_t ShortNoSwap(int16_t value)
{
    return value;
}

int32_t LongSwap(int32_t value)
{
    uint32_t bits = (uint32_t)value;
    uint32_t swapped = (bits << 24) | ((bits & UINT32_C(0x0000ff00)) << 8)
                     | ((bits & UINT32_C(0x00ff0000)) >> 8) | (bits >> 24);
    if (swapped <= INT32_MAX)
        return (int32_t)swapped;
    return -1 - (int32_t)(UINT32_MAX - swapped);
}

uint64_t Long64Swap(uint64_t value)
{
    uint64_t swapped = 0;
    for (unsigned int byte = 0; byte < 8; ++byte) {
        swapped = (swapped << 8) | (value & UINT64_C(0xff));
        value >>= 8;
    }
    return swapped;
}

uint64_t Long64NoSwap(uint64_t value)
{
    return value;
}

uint8_t ColorIndex(uint8_t value)
{
    uint8_t index = (uint8_t)(value - 0x30);
    return index < 10 ? index : 7;
}

bool I_islower(int32_t value)
{
    return value >= 0x61 && value <= 0x7a;
}

bool I_isdigit(int32_t value)
{
    return value >= 0x30 && value <= 0x39;
}
