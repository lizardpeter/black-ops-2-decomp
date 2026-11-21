// universal/com_math.cpp (CoDMPServer_PC: com_math.obj)
#include "universal/com_math.h"

// 00761340
signed char ClampChar(int i)
{
	if (i < -128)
		return -128;
	if (i > 127)
		return 127;
	return i;
}

// 00761360
short ClampShort(int i)
{
	if (i < -32768)
		return -32768;
	if (i > 32767)
		return 32767;
	return i;
}
