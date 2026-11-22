// gfx_d3d/r_init.cpp (CoDMPServer_PC: r_init.obj)
//
// Renderer start-up. So far the swap chain's description, which only the Store build has: the server's renderer has
// no device code, so the name and the file are this reconstruction's.
#define T6_SOURCE "gfx_d3d/r_init.cpp"

#include "gfx_d3d/r_dx.h"
#include "universal/dvar.h"

#include <string.h>

#ifdef T6_STORE
extern const dvar_t *r_aaSamples;

// 14071E360 (Store only; name made up)
// The multisampling the renderer uses for its own targets, from r_aaSamples (16 asks for 4 samples at quality 16),
// and the swap chain to create: the window's size, R8G8B8A8, one sample, two buffers, windowed (exclusive
// fullscreen is entered afterwards), DXGI_SWAP_EFFECT_DISCARD (the blt model), allowing mode switches.
void sub_14071E360(DXGI_SWAP_CHAIN_DESC *swapChainDesc, const GfxWindowParms *wndParms)
{
	dx.multiSampleType = Dvar_GetInt(r_aaSamples);
	dx.multiSampleQuality = 0;
	if (dx.multiSampleType == 16)
	{
		dx.multiSampleType = 4;
		dx.multiSampleQuality = 16;
	}
	memset(swapChainDesc, 0, sizeof(*swapChainDesc));
	swapChainDesc->BufferDesc.Width = wndParms->displayWidth;
	swapChainDesc->BufferDesc.Height = wndParms->displayHeight;
	swapChainDesc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc->SampleDesc.Count = 1;
	swapChainDesc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc->BufferCount = 2;
	swapChainDesc->OutputWindow = wndParms->hwnd;
	swapChainDesc->Windowed = TRUE;
	swapChainDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}
#endif
