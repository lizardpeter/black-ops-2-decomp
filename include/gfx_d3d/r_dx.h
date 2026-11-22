// gfx_d3d/r_dx.h: the renderer's D3D state, as far as reconstructed code reads it.
#pragma once

#include <d3d11.h>

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

// A window with its swap chain (0x14 bytes on the server, 0x20 on x64).
struct GfxWindowTarget
{
	HWND hwnd;
	IDXGISwapChain *swapChain;
	ID3D11View *view;
	int width;
	int height;
};

// The D3D globals (0x37B80 bytes on the server: the PDB's larger DxGlobals, with the 512-entry display-mode tables;
// pdb/types.h has the smaller one, see pdb/type-variants.txt); only what reconstructed code reads, at the server's
// offsets (x86) and the Store revision's (x64). The Store's dx is at 1427F1D80: device, context, vendorId and the
// NVIDIA stereo fields where an x64 layout puts them; needsShaderWarming at +0x488, and the block from
// resolutionCallback to swapChain at +0x3C7B0 ..., with the server's spacing inside it.
typedef GfxWindowTarget GfxWindowTargets[1];
typedef void (*GfxResolutionCallback)(bool);
#define T6_DX_FIELD(type, name, x86, x64) \
	struct { unsigned char pad[sizeof(void *) == 8 ? (x64) : (x86)]; type value; } name
struct DxGlobals
{
	union
	{
	T6_DX_FIELD(ID3D11DeviceContext *, context, 0x8, 0x10);
	T6_DX_FIELD(bool, nvStereoActivated, 0x35, 0x51);
	T6_DX_FIELD(void *, nvStereoHandle, 0x38, 0x58);
	T6_DX_FIELD(bool, needsShaderWarming, 0x448, 0x488);
	T6_DX_FIELD(GfxResolutionCallback, resolutionCallback, 0x37AF4, 0x3C7B0);
	T6_DX_FIELD(unsigned int, multiSampleType, 0x37AF8, 0x3C7B8);
	T6_DX_FIELD(unsigned int, multiSampleQuality, 0x37AFC, 0x3C7BC);
	T6_DX_FIELD(int, targetWindowIndex, 0x37B08, 0x3C7D0);
	T6_DX_FIELD(int, windowCount, 0x37B0C, 0x3C7D4);
	T6_DX_FIELD(GfxWindowTargets, windows, 0x37B10, 0x3C7D8);
	T6_DX_FIELD(IDXGISwapChain *, swapChain, 0x37B24, 0x3C7F8);
	};
};
#undef T6_DX_FIELD
static_assert(sizeof(void *) != 8 || sizeof(GfxWindowTarget) == 0x20, "GfxWindowTarget (x64)");

extern DxGlobals dx;
#define DX(name) (dx.name.value)
