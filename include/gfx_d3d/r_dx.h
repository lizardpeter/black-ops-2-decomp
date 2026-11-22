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
// offsets (x86) and the Store revision's (x64). The Store's dx is at 1427F1D80: the fields up to nvStereoHandle where an
// x64 layout puts them; txaaCtx at +0x80 (64-byte aligned there), so adapterIndex at +0x480 and needsShaderWarming
// at +0x488; the block from resolutionCallback to swapChain at +0x3C7B0 ..., with the server's spacing inside it.
typedef GfxWindowTarget GfxWindowTargets[1];
typedef ID3D11Buffer *GfxConstBuffers[4];
struct TxaaCtxDX { unsigned char opaque[0x400]; };  // NVIDIA TXAA's context (TxaaOpenDX fills it)
typedef void (*GfxResolutionCallback)(bool);
#define T6_DX_FIELD(type, name, x86, x64) \
	struct { unsigned char pad[sizeof(void *) == 8 ? (x64) : (x86)]; type value; } name
struct ID3D11Texture2D;
struct DxGlobals
{
	union
	{
	T6_DX_FIELD(ID3D11Device *, device, 0x4, 0x8);
	T6_DX_FIELD(ID3D11DeviceContext *, context, 0x8, 0x10);
	T6_DX_FIELD(GfxConstBuffers, constBuffer, 0xC, 0x18);
	T6_DX_FIELD(unsigned long, vendorId, 0x1C, 0x38);
	T6_DX_FIELD(bool, adapterNativeIsValid, 0x20, 0x3C);
	T6_DX_FIELD(int, adapterNativeWidth, 0x24, 0x40);
	T6_DX_FIELD(int, adapterNativeHeight, 0x28, 0x44);
	T6_DX_FIELD(int, adapterFullscreenWidth, 0x2C, 0x48);
	T6_DX_FIELD(int, adapterFullscreenHeight, 0x30, 0x4C);
	T6_DX_FIELD(bool, nvInitialized, 0x34, 0x50);
	T6_DX_FIELD(bool, nvStereoActivated, 0x35, 0x51);
	T6_DX_FIELD(void *, nvStereoHandle, 0x38, 0x58);
	T6_DX_FIELD(TxaaCtxDX, txaaCtx, 0x40, 0x80);
	T6_DX_FIELD(int, adapterIndex, 0x440, 0x480);
	T6_DX_FIELD(bool, doResizeBuffers, 0x446, 0x486);
	T6_DX_FIELD(bool, needsShaderWarming, 0x448, 0x488);
	T6_DX_FIELD(GfxResolutionCallback, resolutionCallback, 0x37AF4, 0x3C7B0);
	T6_DX_FIELD(unsigned int, multiSampleType, 0x37AF8, 0x3C7B8);
	T6_DX_FIELD(unsigned int, multiSampleQuality, 0x37AFC, 0x3C7BC);
	T6_DX_FIELD(int, targetWindowIndex, 0x37B08, 0x3C7D0);
	T6_DX_FIELD(int, windowCount, 0x37B0C, 0x3C7D4);
	T6_DX_FIELD(GfxWindowTargets, windows, 0x37B10, 0x3C7D8);
	T6_DX_FIELD(IDXGISwapChain *, swapChain, 0x37B24, 0x3C7F8);
	T6_DX_FIELD(ID3D11Texture2D *, backBuffer, 0x37B28, 0x3C800);
	};
};
#undef T6_DX_FIELD
static_assert(sizeof(void *) != 8 || sizeof(GfxWindowTarget) == 0x20, "GfxWindowTarget (x64)");

extern DxGlobals dx;
#define DX(name) (dx.name.value)

// The video settings the renderer works to (0x40 bytes, no pointers: the same on x64). Right after dx in both builds.
struct vidConfig_t
{
	unsigned int sceneWidth;
	unsigned int sceneHeight;
	unsigned int displayWidth;
	unsigned int displayHeight;
	unsigned short outputDisplayWidth;
	unsigned short outputDisplayHeight;
	unsigned int displayFrequency;
	bool isToolMode;
	int isWideScreen;
	int isHiDef;
	int isFullscreen;
	float aspectRatioWindow;
	float aspectRatioScenePixel;
	float aspectRatioDisplayPixel;
	unsigned int maxTextureSize;
	unsigned int maxTextureMaps;
	bool deviceSupportsGamma;
};
static_assert(sizeof(vidConfig_t) == 0x40, "vidConfig_t");
extern vidConfig_t vidConfig;

// The renderer's thread state (0x28 bytes, the same on x64; the Store's r_glob is at 1427E7330)
struct GfxGlobals
{
	bool startedRenderThread;
	bool isMultiplayer;
	bool isZombie;
	volatile int endFrameFence;
	bool isRenderingRemoteUpdate;          // the render thread runs SCR_UpdateScreen itself
	volatile int screenUpdateNotify;
	volatile int remoteScreenUpdateNesting;  // another thread asks the render thread for screen updates
	volatile int remoteScreenUpdateInGame;
	volatile int remoteScreenUpdateLUI;
	unsigned char remoteScreenLastSceneResolveTarget;
	int backEndFrameCount;
	unsigned char frameBuffer;
};
static_assert(sizeof(GfxGlobals) == 0x28, "GfxGlobals");
extern GfxGlobals r_glob;
