// VC100 (VS2010) <string.h>, as far as the reconstruction needs it.
//
// Build environment, not game source. The functions resolve to the original binary's CRT through the
// hybrid link (tools/build_recon.py), so reconstructed code calls the same _memset the original does.
#pragma once

typedef __SIZE_TYPE__ size_t;  // unsigned int on x86, unsigned __int64 on x64

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *dst, int val, size_t size);
void *memcpy(void *dst, const void *src, size_t size);
int memcmp(const void *a, const void *b, size_t size);

#ifdef __cplusplus
}
#endif
