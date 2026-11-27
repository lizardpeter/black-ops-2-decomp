// gfx_d3d/r_font.cpp: fonts, glyph lookup and text measurement.
#pragma once

#include "qcommon/qcommon.h"

struct Material;

struct Glyph  // 0x18
{
	unsigned short letter;
	char x0;
	char y0;
	unsigned char dx;
	unsigned char pixelWidth;
	unsigned char pixelHeight;
	float s0;
	float t0;
	float s1;
	float t1;
};
static_assert(sizeof(Glyph) == 0x18, "Glyph");

struct KerningPairs  // 0x8
{
	unsigned short wFirst;
	unsigned short wSecond;
	int iKernAmount;
};

struct Font_s  // 0x24 on the servers, 0x38 in the Store build (pointers)
{
	const char *fontName;
	int pixelHeight;
	int isScalingAllowed;
	int glyphCount;
	int kerningPairsCount;
	Material *material;
	Material *glowMaterial;
	Glyph *glyphs;
	KerningPairs *kerningPairs;
};
#if __SIZEOF_POINTER__ == 8  // x64 (the Store build, and replay tests on 64-bit hosts)
static_assert(sizeof(Font_s) == 0x38 && __builtin_offsetof(Font_s, glyphs) == 0x28, "Font_s (x64)");
#else
static_assert(sizeof(Font_s) == 0x24 && __builtin_offsetof(Font_s, glyphs) == 0x1C, "Font_s");
#endif

const Glyph *R_GetCharacterGlyph(Font_s *font, unsigned int letter);
Font_s *R_RegisterFont_FastFile(const char *fontName, int imageTrack);
Font_s *R_RegisterFont(const char *name, int imageTrack);
int R_TextWidth(LocalClientNum_t localClientNum, const char *text, int maxChars, Font_s *font);

// gfx_d3d/r_fonticon.cpp
void R_GetFontIconEntryInfo(LocalClientNum_t localClientNum, char *iconName, int fontHeight, Material **material,
                            float *iconWidth, float *iconHeight);
