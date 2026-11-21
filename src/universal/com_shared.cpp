// universal/com_shared.cpp (CoDMPServer_PC: com_shared.obj)
#include "universal/com_shared.h"

// 0076B690
int Com_HashKey(const char *string, int maxlen)
{
	if (!string)
		return 0;
	// The original sums in a 32-bit int and lets it wrap (`imul; add`); summed unsigned here so the wrap is
	// defined behaviour too. The mix shifts the signed value (`sar`).
	unsigned int sum = 0;
	for (int i = 0; i < maxlen && string[i]; ++i)
		sum += string[i] * (i + 119);
	int hash = (int)sum;
	return (((hash >> 10) ^ hash) >> 10) ^ hash;
}
