// Replay probe: gfx_d3d/r_font.cpp (with stringed/stringed_hooks.cpp, inlined into it in the Store build) against
// the Store MP original. verify_r_font.py drives it.
#include "database/db_registry.h"
#include "gfx_d3d/r_font.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#define MS __attribute__((ms_abi))
using U = std::uint64_t;

namespace
{
Font_s font;
Glyph glyphs[160];
char text[512];
struct Capture
{
	U count;
	U events[32][5];
	U result;
} cap;

U Hash(const char *s)
{
	U h = 0;
	for (; *s; ++s)
		h = h * 131 + (unsigned char)*s;
	return h;
}

void Log(U n, U a = 0, U b = 0, U c = 0, U d = 0)
{
	assert(cap.count < 32);
	U *e = cap.events[cap.count++];
	e[0] = n, e[1] = a, e[2] = b, e[3] = c, e[4] = d;
}
}  // namespace

// What the functions call, for the reconstruction; the original reaches the same through the hooks below.
XAssetHeader DB_FindXAssetHeader(XAssetType type, const char *name, bool errorIfMissing, int waitTime)
{
	Log(2, type, Hash(name), errorIfMissing, (unsigned)waitTime);
	XAssetHeader header;
	header.data = reinterpret_cast<void *>(0x123456);
	return header;
}

// The Store's R_GetFontIconEntryInfo (140723450) never reads its localClientNum (RCX), and its LTCG callers leave
// RCX as it happens to be, so the client isn't compared.
void R_GetFontIconEntryInfo(LocalClientNum_t, char *iconName, int fontHeight, Material **material, float *iconWidth,
                            float *iconHeight)
{
	Log(3, 0, Hash(iconName), (unsigned)fontHeight);
	*material = reinterpret_cast<Material *>(0x543210);
	*iconWidth = 1.25f;
	*iconHeight = 0.5f;
}

static XAssetHeader MS HookFind(XAssetType t, const char *s, bool b, int n) { return DB_FindXAssetHeader(t, s, b, n); }
static void MS HookIcon(LocalClientNum_t c, char *s, int h, Material **m, float *w, float *y)
{
	R_GetFontIconEntryInfo(c, s, h, m, w, y);
}

extern "C" void *RFontHook(unsigned i)
{
	void *hooks[] = {reinterpret_cast<void *>(HookFind), reinterpret_cast<void *>(HookIcon)};
	return hooks[i];
}

extern "C" unsigned RFontCaptureSize() { return sizeof(Capture); }

// op 0 R_GetCharacterGlyph, 1 R_RegisterFont, 2 R_TextWidth; image null: the reconstruction
extern "C" void RFontRun(unsigned char *image, unsigned op, unsigned seed, void *out)
{
	cap = {};
	unsigned state = seed + 1;
	auto next = [&]() { return state = state * 1664525u + 1013904223u; };
	const unsigned count = 96 + next() % 65;
	font = {};
	font.glyphCount = (int)count;
	font.glyphs = glyphs;
	font.pixelHeight = (int)(next() % 100 + 1);
	for (unsigned i = 0; i < 160; ++i)
	{
		glyphs[i] = {};
		glyphs[i].letter = (unsigned short)(i < 96 ? i + 32 : 0x80 + (i - 96) * 7);
		glyphs[i].dx = (unsigned char)(next() >> 24);
	}
	const unsigned letters[] = {0, 1, 31, 32, 46, 65, 127, 128, 135, 0x1234, 0xFFFF, 0x10000, 0xFFFFFFFFu};
	const unsigned letter = (seed & 1) ? letters[next() % 13] : next() % 600;
	const char *parts[] = {"a", "XYZ", "^1", "^C2", "^L", "^F", "^^", "^X", "\n", "\r", "\x14", "\x15", "\xc2\x80",
	                       "\xe1\x80\x80", "^Bicon^", "^B^", " ", "Z", "^C^", "\xf0\x9f\x98\x80"};
	std::memset(text, 0, sizeof(text));
	for (unsigned i = 0, n = next() % 24, pos = 0; i < n; ++i)
	{
		const char *s = parts[next() % 20];
		const unsigned len = (unsigned)std::strlen(s);
		std::memcpy(text + pos, s, len);
		pos += len;
	}
	const int maxChars = (int)(next() % 32) - 3;
	const auto client = (LocalClientNum_t)((int)(next() % 3) - 1);
	if (image)
	{
		switch (op)
		{
		case 0:
			cap.result = (U)(reinterpret_cast<const Glyph *(MS *)(Font_s *, unsigned)>(image + 0x71A6E0)(&font, letter) - glyphs);
			break;
		case 1:
			cap.result = (U)reinterpret_cast<Font_s *(MS *)(const char *, int)>(image + 0x71A780)("fonts/720/normalfont", 0);
			break;
		case 2:
			cap.result = (unsigned)reinterpret_cast<int(MS *)(LocalClientNum_t, const char *, int, Font_s *)>(image + 0x71A7A0)(client, text, maxChars, &font);
			break;
		}
	}
	else
	{
		switch (op)
		{
		case 0: cap.result = (U)(R_GetCharacterGlyph(&font, letter) - glyphs); break;
		case 1: cap.result = (U)R_RegisterFont("fonts/720/normalfont", 0); break;
		case 2: cap.result = (unsigned)R_TextWidth(client, text, maxChars, &font); break;
		}
	}
	std::memcpy(out, &cap, sizeof(cap));
}
