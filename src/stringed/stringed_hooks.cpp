// stringed/stringed_hooks.cpp (CoDMPServer_PC: stringed_hooks.obj)
//
// Localized string helpers. So far the UTF-8 reader text drawing and measuring use (inlined in the Store build).
#define T6_SOURCE "stringed/stringed_hooks.cpp"

#include "stringed/stringed_hooks.h"

// 00703FA0  stringed/stringed_hooks.cpp:1148
// One letter of UTF-8, up to three bytes (no four-byte sequences); a byte that starts no sequence is itself.
unsigned int SEH_DecodeLetter(unsigned char firstChar, unsigned char secondChar, unsigned char thirdChar,
                              int *usedCount)
{
	if (!(firstChar & 0x80))
	{
		*usedCount = 1;
		return firstChar;
	}
	if ((firstChar & 0xE0) == 0xC0)
	{
		*usedCount = 2;
		return ((firstChar & 0x1F) << 6) | (secondChar & 0x3F);
	}
	if ((firstChar & 0xF0) == 0xE0)
	{
		*usedCount = 3;
		return (((firstChar & 0x0F) << 6 | (secondChar & 0x3F)) << 6) | (thirdChar & 0x3F);
	}
	*usedCount = 1;
	return firstChar;
}

// 00704020  stringed/stringed_hooks.cpp:1174
unsigned int SEH_ReadCharFromString(const char **text)
{
	const unsigned char *str = (const unsigned char *)*text;
	int usedCount;
	const unsigned int letter = SEH_DecodeLetter(str[0], str[1], str[2], &usedCount);
	*text += usedCount;
	return letter;
}
