// gfx_d3d/r_dx.h: the renderer's D3D state, as far as reconstructed code reads it.
#pragma once

#include <dxgi.h>

// The window the renderer draws to (0x24 bytes on the server; x64: 0x28).
struct GfxWindowParms
{
	HWND hwnd;
	int hz;
	bool fullscreen;
	int x;
	int y;
	int sceneWidth;
	int sceneHeight;
	int displayWidth;
	int displayHeight;
};
static_assert(sizeof(void *) != 8 || __builtin_offsetof(GfxWindowParms, displayWidth) == 0x20, "GfxWindowParms");

// The D3D globals (0x1C380 bytes on the server); only what reconstructed code reads. The Store revision's dx is at
// 1427F1D80: device, context and vendorId where the server's x64 layout puts them (+8, +0x10, +0x38), and
// multiSampleType 0x1DC20 bytes further on than that layout (+0x3C7B8; targetWindowIndex and windows follow it with
// the same spacing).
struct DxGlobals
{
#ifdef T6_STORE
	unsigned char unknown0[0x3C7B8];
#else
	unsigned char unknown0[0x1C2F8];
#endif
	unsigned int multiSampleType;
	unsigned int multiSampleQuality;
};
#ifdef T6_STORE
static_assert(__builtin_offsetof(DxGlobals, multiSampleType) == 0x3C7B8, "DxGlobals (Store)");
#endif

extern DxGlobals dx;
