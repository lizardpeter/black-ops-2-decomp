// Replay probe: ui/ui_main.cpp's font selection (UI_GetFontHandleDefault, UI_GetFontHandle) with
// client/screen_placement.cpp and universal/dvar.cpp's Dvar_GetFloat, all inlined into them in the Store build,
// against the Store MP original. verify_ui_fonts.py drives it.
#include "ui/ui.h"
#include "universal/dvar.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define MS __attribute__((ms_abi))
using U = std::uint64_t;

// The globals the functions read, for the reconstruction; the original reads its own, which the test fills alike.
const dvar_t *ui_smallFont, *ui_bigFont, *ui_extraBigFont;
sharedUiInfo_t sharedUiInfo;
float cg_hudSplitscreenScale;
HiResShot s_hiResShot;
ScreenPlacement scrPlaceFull;

namespace
{
dvar_t dvars[3];
char strings[3][32];
ScreenPlacement placement;
}  // namespace

static double MS HookAtof(const char *s) { return std::atof(s); }
extern "C" void *UiFontsAtof() { return reinterpret_cast<void *>(HookAtof); }

// op 0 UI_GetFontHandleDefault, 1 UI_GetFontHandle; image null: the reconstruction. Returns the font chosen.
extern "C" U UiFontsRun(unsigned char *image, unsigned op, unsigned seed, std::uint32_t scaleBits)
{
	unsigned state = seed + 1;
	auto next = [&]() { return state = state * 1664525u + 1013904223u; };
	float scale;
	std::memcpy(&scale, &scaleBits, 4);
	const dvar_t **slots[] = {&ui_smallFont, &ui_bigFont, &ui_extraBigFont};
	for (unsigned i = 0; i < 3; ++i)
	{
		const unsigned kind = (seed >> (i * 2)) & 3;
		*slots[i] = kind ? &dvars[i] : nullptr;
		dvars[i] = {};
		dvars[i].type = kind == 1 ? DVAR_TYPE_FLOAT : kind == 2 ? DVAR_TYPE_INT : DVAR_TYPE_STRING;
		dvars[i].flags = DVAR_EXTERNAL;
		const int v = (int)(next() % 17) - 8;
		if (kind == 1)
			dvars[i].current.value = (float)v * 0.25f;
		else if (kind == 2)
			dvars[i].current.integer = v;
		else
		{
			std::snprintf(strings[i], sizeof(strings[i]), "%d.%d", v, (int)(next() % 4) * 25);
			dvars[i].current.string = strings[i];
		}
	}
	Font_s **fonts = &sharedUiInfo.assets.bigFont;
	for (unsigned i = 0; i < 8; ++i)
		fonts[i] = reinterpret_cast<Font_s *>(0x30000 + i * 16);
	s_hiResShot.active = (seed >> 6) & 1;
	s_hiResShot.scale[0] = (float)((seed >> 25) & 7) * 0.5f;
	s_hiResShot.scale[1] = (float)((seed >> 7) & 15) * 0.25f;
	cg_hudSplitscreenScale = (float)((seed >> 11) & 15) * 0.25f;
	placement = {};
	placement.scaleVirtualToReal[1] = (float)((seed >> 15) & 15) * 0.25f;
	const int fontEnum = (int)((seed >> 19) % 13) - 2;
	if (image)
	{
		std::memcpy(image + 0x2121448, &ui_smallFont, 8);
		std::memcpy(image + 0x2121368, &ui_bigFont, 8);
		std::memcpy(image + 0x21213B8, &ui_extraBigFont, 8);
		std::memcpy(image + 0x2126570, &sharedUiInfo.assets, sizeof(sharedUiInfo.assets));
		std::memcpy(image + 0x27F1B60, &s_hiResShot, sizeof(s_hiResShot));
		std::memcpy(image + 0xE6287C, &cg_hudSplitscreenScale, 4);
		if (op == 0)
			return (U)reinterpret_cast<Font_s *(MS *)(float)>(image + 0x3FCD20)(scale);
		return (U)reinterpret_cast<Font_s *(MS *)(const ScreenPlacement *, int, float)>(image + 0x3FCE40)(&placement, fontEnum, scale);
	}
	if (op == 0)
		return (U)UI_GetFontHandleDefault(scale);
	return (U)UI_GetFontHandle(&placement, fontEnum, scale);
}
