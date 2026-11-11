#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int8_t ClampChar(int32_t value);
int16_t ClampShort(int32_t value);
int32_t Com_HashKey(const char *text, int32_t max_length);

#ifdef __cplusplus
}
#endif
