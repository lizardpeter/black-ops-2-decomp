// gfx_d3d/r_font.cpp (CoDMPServer_PC: r_font.obj)
//
// Fonts: glyph lookup, registration and text width. One source for the servers and the Store build; the Store
// compiler inlines SEH_ReadCharFromString and GetButtonProperties into R_TextWidth, and the Store has no LoadObj
// font path.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\gfx_d3d\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "r_font.cpp"
#define T6_SOURCE "gfx_d3d/r_font.cpp"

#include "gfx_d3d/r_font.h"

#include "database/db_registry.h"
#include "stringed/stringed_hooks.h"
#include "t6_assert.h"
#include "universal/dvar.h"

#ifndef T6_STORE
extern const dvar_t *useFastFile;  // universal/com_files.cpp

Font_s *R_RegisterFont_LoadObj(const char *fontName, int imageTrack);
#endif

// 00A28200  gfx_d3d/r_font.cpp:25
// Store MP 14071A6E0 (tests/replay/verify_r_font.py)
const Glyph *R_GetCharacterGlyph(Font_s *font, unsigned int letter)
{
	if (letter - 0x20 <= 0x5F)
	{
		const Glyph *glyph = &font->glyphs[letter - 0x20];
		assert_at(54, glyph->letter == letter);
		return glyph;
	}
	int top = font->glyphCount - 1;
	int bottom = 0x60;
	while (bottom <= top)
	{
		const int mid = (bottom + top) / 2;
		if (font->glyphs[mid].letter == letter)
			return &font->glyphs[mid];
		if (font->glyphs[mid].letter < letter)
			bottom = mid + 1;
		else
			top = mid - 1;
	}
	return &font->glyphs[14];  // '.'
}

// 00A28470  gfx_d3d/r_font.cpp:175 (inlined in the Store build)
Font_s *R_RegisterFont_FastFile(const char *fontName, int imageTrack)
{
	return DB_FindXAssetHeader(ASSET_TYPE_FONT, fontName, true, -1).font;
}

// 00A28490  gfx_d3d/r_font.cpp:181
// Store MP 14071A780 (tests/replay/verify_r_font.py): fast files only.
Font_s *R_RegisterFont(const char *name, int imageTrack)
{
#ifdef T6_STORE
	return R_RegisterFont_FastFile(name, imageTrack);
#else
	if (Dvar_GetBool(useFastFile))
		return R_RegisterFont_FastFile(name, imageTrack);
	return R_RegisterFont_LoadObj(name, imageTrack);
#endif
}

// 00A28530  gfx_d3d/r_font.cpp:213 (inlined in the Store build's R_TextWidth)
// "^B<icon>^": the icon's name, at most 63 characters, and its width at the font's height.
static void GetButtonProperties(LocalClientNum_t localClientNum, const char *text, Font_s *font,
                                int *buttonCharLength, int *buttonMaterialWidth)
{
	assert_at(220, text);
	char buttonName[64];
	int i = 1;
	while (text[i] != '^' && i < 64)
	{
		buttonName[i - 1] = text[i];
		++i;
	}
	buttonName[i - 1] = '\0';
	*buttonCharLength = i + 1;
	Material *handle;
	float iconWidth, iconHeight;
	R_GetFontIconEntryInfo(localClientNum, buttonName, font->pixelHeight, &handle, &iconWidth, &iconHeight);
	*buttonMaterialWidth = (int)((float)font->pixelHeight * iconWidth);
}

// 00A285F0  gfx_d3d/r_font.cpp:241
// Store MP 14071A7A0 (tests/replay/verify_r_font.py). The widest line, in pixels: color codes (^0-^9, ^F, ^L,
// ^C<digit>) take no room, ^B<icon>^ takes its icon's width, and \x14 and \x15 are skipped.
int R_TextWidth(LocalClientNum_t localClientNum, const char *text, int maxChars, Font_s *font)
{
	assert_at(247, text);
	assert_at(248, font);
	int maxWidth = 0;
	int lineWidth = 0;
	if (maxChars <= 0)
		maxChars = 0x7FFFFFFF;
	int count = 0;
	while (*text && count < maxChars)
	{
		const unsigned int letter = SEH_ReadCharFromString(&text);
		if (letter == 0x14 || letter == 0x15)
			continue;
		if (letter == '\r' || letter == '\n')
		{
			lineWidth = 0;
			continue;
		}
		if (letter == '^' && text && *text != '^')
		{
			if ((*text >= '0' && *text <= '9') || *text == 'F')
			{
				++text;
				continue;
			}
			if (*text == 'C' && text[1] != '^' && text[1] >= '0' && text[1] <= '9')
			{
				text += 2;
				continue;
			}
			if (*text == 'L')
			{
				++text;
				continue;
			}
			if (*text == 'B')
			{
				int buttonCharLength = 0;
				int buttonMaterialWidth = 0;
				GetButtonProperties(localClientNum, text, font, &buttonCharLength, &buttonMaterialWidth);
				text += buttonCharLength;
				lineWidth += buttonMaterialWidth;
				if (maxWidth < lineWidth)
					maxWidth = lineWidth;
				continue;
			}
		}
		lineWidth += R_GetCharacterGlyph(font, letter)->dx;
		if (maxWidth < lineWidth)
			maxWidth = lineWidth;
		++count;
	}
	return maxWidth;
}
