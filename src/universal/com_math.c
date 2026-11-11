#include "t6_recon.h"

int8_t ClampChar(int32_t value)
{
    if (value < INT8_MIN)
        return INT8_MIN;
    if (value > INT8_MAX)
        return INT8_MAX;
    return (int8_t)value;
}

int16_t ClampShort(int32_t value)
{
    if (value < INT16_MIN)
        return INT16_MIN;
    if (value > INT16_MAX)
        return INT16_MAX;
    return (int16_t)value;
}
