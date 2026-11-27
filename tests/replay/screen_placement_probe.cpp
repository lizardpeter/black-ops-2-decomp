// Replay probe: client/screen_placement.cpp (ScrPlace_ApplyRect, ScrPlace_VirtualFromReal_*) and the Store's aim
// assist screen fractions, against the Store MP original. verify_screen_placement.py drives it.
#include "aim_assist/aim_assist.h"
#include "client/client.h"
#include "ui/ui.h"

#include <cstdint>
#include <cstring>

#define MS __attribute__((ms_abi))
using U = std::uint64_t;

// The globals, for the reconstruction; the test copies the same values into the original's.
ScreenPlacement scrPlaceView[MAX_UI_CONTEXTS], scrPlaceFull;
float cg_hudSplitscreenScale;
HiResShot s_hiResShot;
clientStatic_t cls;
AimAssistGlobals aaGlobArray[MAX_LOCAL_CLIENTS];
ClientGameState clientGameStates[MAX_LOCAL_CLIENTS];

namespace
{
ScreenPlacement placement;
unsigned state;
unsigned Next() { return state = state * 1664525u + 1013904223u; }
float F() { return (float)((int)(Next() % 4097) - 2048) * 0.0625f; }
}  // namespace

struct Result
{
	float rect[4];
	float value;
};

// op 0 ScrPlace_ApplyRect, 1 ScrPlace_VirtualFromReal_Width, 2 _Height, 3 AimAssist_VirtualToScreenX, 4 _Y.
extern "C" void ScrPlaceRun(unsigned char *image, unsigned op, unsigned seed, Result *out)
{
	state = seed + 1;
	float *fields = &placement.scaleVirtualToReal[0];
	for (unsigned i = 0; i < sizeof(placement) / 4; ++i)
		fields[i] = F();
	scrPlaceView[0] = placement;
	s_hiResShot.active = (int)(seed & 1);
	s_hiResShot.scale[0] = F(), s_hiResShot.scale[1] = F(), s_hiResShot.offset[0] = F(), s_hiResShot.offset[1] = F();
	cls.vidConfig.displayWidth = Next() % 8000, cls.vidConfig.displayHeight = Next() % 8000;
	cg_hudSplitscreenScale = F();
	aaGlobArray[0].screenWidth = F(), aaGlobArray[0].screenHeight = F();
	clientGameStates[0] = {};
	clientGameStates[0].localClientNum = (seed & 2) ? LOCAL_CLIENT_0 : (LocalClientNum_t)7;  // a mismatch reads index -1
	clientGameStates[0].uiContextIndex = UI_CONTEXT_0;
	const int horz = (int)(Next() % 13) - 1, vert = (int)(Next() % 13) - 1;
	Result r{};
	for (float &v : r.rect)
		v = F();
	const float value = F();
	if (image)
	{
		std::memcpy(image + 0xE63390, scrPlaceView, sizeof(scrPlaceView));
		std::memcpy(image + 0x27F1B60, &s_hiResShot, sizeof(s_hiResShot));
		std::memcpy(image + 0x2CC0658, &cls.vidConfig.displayWidth, 8);
		std::memcpy(image + 0xE6287C, &cg_hudSplitscreenScale, 4);
		std::memcpy(image + 0x2C64A70, &aaGlobArray[0].screenWidth, 8);
		std::memcpy(image + 0x1EF91E0, clientGameStates, sizeof(clientGameStates));
		switch (op)
		{
		case 0:
			reinterpret_cast<void(MS *)(const ScreenPlacement *, float *, float *, float *, float *, int, int)>(image + 0x1F65E0)(
				&placement, &r.rect[0], &r.rect[1], &r.rect[2], &r.rect[3], horz, vert);
			break;
		case 1: r.value = reinterpret_cast<float(MS *)(const ScreenPlacement *, float)>(image + 0xDDB40)(&placement, value); break;
		case 2: r.value = reinterpret_cast<float(MS *)(const ScreenPlacement *, float)>(image + 0xD7110)(&placement, value); break;
		case 3: r.value = reinterpret_cast<float(MS *)(LocalClientNum_t, float)>(image + 0xA9AB0)(LOCAL_CLIENT_0, value); break;
		case 4: r.value = reinterpret_cast<float(MS *)(LocalClientNum_t, float)>(image + 0xA9B20)(LOCAL_CLIENT_0, value); break;
		}
	}
	else
	{
		switch (op)
		{
		case 0: ScrPlace_ApplyRect(&placement, &r.rect[0], &r.rect[1], &r.rect[2], &r.rect[3], horz, vert); break;
		case 1: r.value = ScrPlace_VirtualFromReal_Width(&placement, value); break;
		case 2: r.value = ScrPlace_VirtualFromReal_Height(&placement, value); break;
		case 3: r.value = AimAssist_VirtualToScreenX(LOCAL_CLIENT_0, value); break;
		case 4: r.value = AimAssist_VirtualToScreenY(LOCAL_CLIENT_0, value); break;
		}
	}
	*out = r;
}
