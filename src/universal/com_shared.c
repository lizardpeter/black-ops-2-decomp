#include "t6_recon.h"

static uint32_t arithmetic_shift_right_10(uint32_t value)
{
    return (value >> 10) | ((value & UINT32_C(0x80000000)) ? UINT32_C(0xffc00000) : 0);
}

int32_t Com_HashKey(const char *text, int32_t max_length)
{
    uint32_t hash = 0;
    if (text) {
        for (int32_t i = 0; i < max_length && text[i]; ++i) {
            uint32_t byte = (unsigned char)text[i];
            uint32_t signed_byte = byte < 128 ? byte : byte - 256;
            hash += signed_byte * ((uint32_t)i + 119);
        }
    }
    uint32_t mixed = arithmetic_shift_right_10(arithmetic_shift_right_10(hash) ^ hash) ^ hash;
    if (mixed <= INT32_MAX)
        return (int32_t)mixed;
    return -1 - (int32_t)(UINT32_MAX - mixed);
}
