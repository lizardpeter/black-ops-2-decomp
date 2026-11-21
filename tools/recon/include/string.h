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
size_t strlen(const char *str);
int strcmp(const char *a, const char *b);
char *strchr(const char *str, int c);

#ifdef __cplusplus
}
#endif

// VC100 expands strcpy and strcat inline (/Oi) and the servers have no out-of-line copy, so these are the
// expansions: a forward byte copy (strcat: after a strlen). Forward copying is what makes the original's
// overlapping strcpy (q_shared.cpp Info_RemoveKey) work.
static inline char *strcpy(char *dst, const char *src)
{
	char *d = dst;
	while ((*d++ = *src++) != 0)
		;
	return dst;
}

static inline char *strcat(char *dst, const char *src)
{
	strcpy(dst + strlen(dst), src);
	return dst;
}
