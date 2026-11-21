// universal/q_shared.cpp (CoDMPServer_PC: q_shared.obj)
#include "universal/q_shared.h"

// 0078DB60
unsigned char ColorIndex(unsigned char c)
{
	unsigned char index = c - '0';
	if (index < 10)
		return index;
	return 7;
}

// 0078DCC0
short ShortSwap(short l)
{
	unsigned char b1 = l & 255;
	unsigned char b2 = (l >> 8) & 255;
	return (b1 << 8) + b2;
}

// 0078DCE0
short ShortNoSwap(short l)
{
	return l;
}

// 0078DCF0
int LongSwap(int l)
{
	unsigned char b1 = l & 255;
	unsigned char b2 = (l >> 8) & 255;
	unsigned char b3 = (l >> 16) & 255;
	unsigned char b4 = (l >> 24) & 255;
	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

// 0078DD20
unsigned __int64 Long64Swap(unsigned __int64 ll)
{
	unsigned char b1 = ll & 255;
	unsigned char b2 = (ll >> 8) & 255;
	unsigned char b3 = (ll >> 16) & 255;
	unsigned char b4 = (ll >> 24) & 255;
	unsigned char b5 = (ll >> 32) & 255;
	unsigned char b6 = (ll >> 40) & 255;
	unsigned char b7 = (ll >> 48) & 255;
	unsigned char b8 = (ll >> 56) & 255;
	return ((unsigned __int64)b1 << 56) + ((unsigned __int64)b2 << 48) + ((unsigned __int64)b3 << 40) +
	       ((unsigned __int64)b4 << 32) + ((unsigned __int64)b5 << 24) + ((unsigned __int64)b6 << 16) +
	       ((unsigned __int64)b7 << 8) + b8;
}

// 0078DDD0
unsigned __int64 Long64NoSwap(unsigned __int64 ll)
{
	return ll;
}

// 0078DF00
bool I_islower(int c)
{
	return c >= 'a' && c <= 'z';
}

// 0078DF40
bool I_isdigit(int c)
{
	return c >= '0' && c <= '9';
}
