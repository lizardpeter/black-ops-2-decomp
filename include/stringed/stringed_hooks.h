// stringed/stringed_hooks.cpp: localized string helpers.
#pragma once

unsigned int SEH_ReadCharFromString(const char **text);
unsigned int SEH_DecodeLetter(unsigned char firstChar, unsigned char secondChar, unsigned char thirdChar,
                              int *usedCount);
