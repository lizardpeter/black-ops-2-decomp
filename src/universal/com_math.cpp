// universal/com_math.cpp (CoDMPServer_PC: com_math.obj)
#include "universal/com_math.h"

#include <math.h>

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

#ifdef T6_STORE
// com_math.cpp (inlined in the Store build; the server's, 00764390, folded with AngleSubtract, is x87 and stays the
// exe's): the difference between two angles in degrees, wrapped to [-180, 180). The Store's copy (CL_CreateCmd
// 140194E20) works in floats, with floorf inlined.
float AngleDelta(float a1, float a2)
{
	float d = (a1 - a2) * (1.0f / 360.0f);
	return (d - floorf(d + 0.5f)) * 360.0f;
}
#endif
