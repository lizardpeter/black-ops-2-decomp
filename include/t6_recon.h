#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int8_t ClampChar(int32_t value);
int16_t ClampShort(int32_t value);
int32_t Com_HashKey(const char *text, int32_t max_length);
int16_t ShortSwap(int16_t value);
int16_t ShortNoSwap(int16_t value);
int32_t LongSwap(int32_t value);
uint64_t Long64Swap(uint64_t value);
uint64_t Long64NoSwap(uint64_t value);
uint8_t ColorIndex(uint8_t value);
bool I_islower(int32_t value);
bool I_isdigit(int32_t value);

#ifdef __cplusplus
}
#endif
