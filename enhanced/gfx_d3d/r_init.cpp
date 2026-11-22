// enhanced/gfx_d3d/r_init.cpp: a flip-model swap chain with at most one frame queued (P0005).
//
// Replaces the swap chain's description (14071E360) and the device's creation (R_CreateDeviceInternal):
// - the swap chain is DXGI_SWAP_EFFECT_FLIP_DISCARD, which lets Windows hand the game's buffers straight to the display
//   (independent flip) in fullscreen and in a borderless window covering the monitor, instead of copying them through
//   the compositor (the original's DISCARD is the blit model);
// - tearing is allowed when DXGI supports it (DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING), so r_vsync 0 runs uncapped or with
//   VRR in a window too (rb_backend.cpp presents with DXGI_PRESENT_ALLOW_TEARING);
// - the device queues at most one frame (IDXGIDevice1::SetMaximumFrameLatency(1)): Present waits instead of letting
//   the game run up to three frames ahead of the display;
// - a system that refuses the flip-model swap chain gets the original one.
// Store build only. Everything else is the original's (src/gfx_d3d/r_init.cpp).
#ifdef T6_STORE
#define T6_SOURCE "gfx_d3d/r_init.cpp"

#include "enhanced/dxgi_flip.h"

#include "gfx_d3d/r_dx.h"
#include "t6_static.h"
#include "universal/dvar.h"
#include "win32/win_local.h"

#include <string.h>

extern const dvar_t *r_aaSamples;
extern const dvar_t *r_aaSamplesMax;
extern const dvar_t *r_shaderWarming;
extern const dvar_t *r_txaaSupported;
extern "C" const IID IID_IDXGIFactory1;
extern "C" const IID IID_IDXGIDevice1;
extern "C" const IID IID_IDXGIAdapter1;
extern "C" int __cdecl NvAPI_Initialize();
extern "C" int __cdecl NvAPI_D3D11_CreateDeviceAndSwapChain(
	IDXGIAdapter1 *adapter, D3D_DRIVER_TYPE driverType, void *software, UINT flags,
	const D3D_FEATURE_LEVEL *featureLevels, UINT featureLevelCount, UINT sdkVersion,
	const DXGI_SWAP_CHAIN_DESC *swapChainDesc, IDXGISwapChain **swapChain, ID3D11Device **device,
	D3D_FEATURE_LEVEL *featureLevel, ID3D11DeviceContext **immediateContext, int *supportedLevel);
extern "C" int __cdecl NvAPI_Stereo_CreateHandleFromIUnknown(void *device, void **stereoHandle);
extern "C" int __cdecl NvAPI_Stereo_IsActivated(void *stereoHandle, unsigned char *isStereoOn);
extern "C" int __cdecl TxaaOpenDX(TxaaCtxDX *ctx, ID3D11Device *device, ID3D11DeviceContext *context);
void R_SetFullscreenOutput();

T6_STATIC const D3D_FEATURE_LEVEL s_featureLevels[3] T6_AT("", "s_featureLevels");
T6_STATIC const UINT s_constBufferSizes[4] T6_AT("", "s_constBufferSizes");

// DXGI 1.5's factory (not in the exe's dxguid)
static const IID IID_IDXGIFactory5_Enhanced = {0x7632E1F5, 0xEE65, 0x4DCA, {0x87, 0xFD, 0x84, 0xCD, 0x75, 0xF8, 0x83, 0x8D}};

bool r_flipModel = true;
bool r_allowTearing;
static bool s_tearingChecked;

// Whether DXGI can present with tearing (IDXGIFactory5::CheckFeatureSupport(PRESENT_ALLOW_TEARING)); false with a
// factory older than DXGI 1.5. Asked once.
bool R_CheckTearingSupport()
{
	if (s_tearingChecked)
		return r_allowTearing;
	s_tearingChecked = true;
	IDXGIFactory1 *factory = 0;
	if (CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory) < 0 || !factory)
		return r_allowTearing = false;
	void *factory5 = 0;
	BOOL allowTearing = FALSE;
	if (IUnknown_QueryInterface(factory, &IID_IDXGIFactory5_Enhanced, &factory5) >= 0 && factory5)
	{
		if (IDXGIFactory5_CheckFeatureSupport(factory5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing,
		                                      sizeof(allowTearing)) < 0)
			allowTearing = FALSE;
		IUnknown_Release(factory5);
	}
	IUnknown_Release(factory);
	return r_allowTearing = allowTearing != FALSE;
}

// The original blit-model description, from now on (the system refused the flip model).
void R_UseBlitModel(DXGI_SWAP_CHAIN_DESC *swapChainDesc)
{
	r_flipModel = false;
	r_allowTearing = false;
	swapChainDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc->Flags &= ~(UINT)DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
}

// src: Store 14071E360. Enhanced: DXGI_SWAP_EFFECT_FLIP_DISCARD (two buffers, as before), and tearing allowed when DXGI
// supports it. Used for the swap chain's creation and for every resize of its buffers.
void sub_14071E360(DXGI_SWAP_CHAIN_DESC *swapChainDesc, const GfxWindowParms *wndParms)
{
	DX(multiSampleType) = Dvar_GetInt(r_aaSamples);
	DX(multiSampleQuality) = 0;
	if (DX(multiSampleType) == 16)
	{
		DX(multiSampleType) = 4;
		DX(multiSampleQuality) = 16;
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
	if (r_flipModel)
	{
		swapChainDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		if (R_CheckTearingSupport())
			swapChainDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}
}

// src: Store 14071FEA0 R_CreateDeviceInternal. Enhanced: when the device and its flip-model swap chain can't be made,
// they are made again with the original swap chain; and the device queues at most one frame.
HRESULT R_CreateDeviceInternal(HWND hwnd, int behaviorFlags, DXGI_SWAP_CHAIN_DESC *swapChainDesc,
                               const GfxWindowParms *wndParms)
{
	swapChainDesc->OutputWindow = hwnd;
	if (DX(vendorId) == 0x1002)
		DX(needsShaderWarming) = true;
	r_shaderWarming = _Dvar_RegisterBool("r_shaderWarming", DX(vendorId) == 0x1002, 0, "");
	DX(needsShaderWarming) = Dvar_GetBool(r_shaderWarming);
	DX(adapterFullscreenWidth) = GetSystemMetrics(SM_CXSCREEN);
	DX(adapterFullscreenHeight) = GetSystemMetrics(SM_CYSCREEN);
	IUnknown_Release(DX(device));
	IUnknown_Release(DX(context));

	IDXGIFactory1 *factory;
	HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory);
	if (hr < 0)
		return hr;
	IDXGIAdapter1 *adapter;
	hr = IDXGIFactory1_EnumAdapters1(factory, DX(adapterIndex), &adapter);
	if (hr < 0)
		return hr;
	IUnknown_Release(factory);

	D3D_FEATURE_LEVEL featureLevel;
	if (DX(vendorId) == 0x10DE)
	{
		if (!DX(nvInitialized))
			DX(nvInitialized) = NvAPI_Initialize() == 0;
		int supportedLevel;
		int status = NvAPI_D3D11_CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, s_featureLevels, 3,
		                                                  D3D11_SDK_VERSION, swapChainDesc, &DX(swapChain),
		                                                  &DX(device), &featureLevel, &DX(context), &supportedLevel);
		if (status != 0 && swapChainDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
		{
			R_UseBlitModel(swapChainDesc);
			status = NvAPI_D3D11_CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, s_featureLevels, 3,
			                                              D3D11_SDK_VERSION, swapChainDesc, &DX(swapChain),
			                                              &DX(device), &featureLevel, &DX(context),
			                                              &supportedLevel);
		}
		if (status != 0 || supportedLevel < 1)
			return -1;
		if (NvAPI_Stereo_CreateHandleFromIUnknown(DX(device), &DX(nvStereoHandle)) == 0)
		{
			NvAPI_Stereo_IsActivated(DX(nvStereoHandle), (unsigned char *)&behaviorFlags);
			DX(nvStereoActivated) = *(unsigned char *)&behaviorFlags != 0;
		}
		else
		{
			DX(nvStereoActivated) = false;
		}
		UINT qualityLevels;
		ID3D11Device_CheckMultisampleQualityLevels(DX(device), swapChainDesc->BufferDesc.Format, 4, &qualityLevels);
		if (qualityLevels > 16)
			Dvar_SetInt(r_aaSamplesMax, 16);
		if (TxaaOpenDX(&DX(txaaCtx), DX(device), DX(context)) == 0)
			Dvar_SetBool(r_txaaSupported, true);
	}
	else
	{
		HRESULT created = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, s_featureLevels, 3,
		                                                D3D11_SDK_VERSION, swapChainDesc, &DX(swapChain),
		                                                &DX(device), &featureLevel, &DX(context));
		if (created < 0 && swapChainDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
		{
			R_UseBlitModel(swapChainDesc);
			D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, s_featureLevels, 3, D3D11_SDK_VERSION,
			                              swapChainDesc, &DX(swapChain), &DX(device), &featureLevel, &DX(context));
		}
	}

	IDXGIDevice1 *dxgiDevice;
	IUnknown_QueryInterface(DX(device), &IID_IDXGIDevice1, (void **)&dxgiDevice);
	IDXGIDevice1_SetMaximumFrameLatency(dxgiDevice, 1);  // at most one frame queued for the display
	IDXGIObject_GetParent(dxgiDevice, &IID_IDXGIAdapter1, (void **)&adapter);
	hr = IDXGIObject_GetParent(adapter, &IID_IDXGIFactory1, (void **)&factory);
	IDXGIFactory1_MakeWindowAssociation(factory, hwnd,
	                                    DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN);
	IUnknown_Release(factory);
	IUnknown_Release(adapter);
	IUnknown_Release(dxgiDevice);

	bool nativeIsValid = false;
	bool haveMonitor = false;
	if (DX(swapChain))
	{
		IDXGIOutput *output;
		IDXGISwapChain_GetContainingOutput(DX(swapChain), &output);
		DXGI_OUTPUT_DESC outputDesc;
		IDXGIOutput_GetDesc(output, &outputDesc);
		IUnknown_Release(output);
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (GetMonitorInfoA(outputDesc.Monitor, &monitorInfo))
		{
			DX(adapterNativeWidth) = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			DX(adapterNativeHeight) = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
			nativeIsValid = true;
			haveMonitor = true;
		}
	}
	if (!haveMonitor)
	{
		DX(adapterNativeWidth) = GetSystemMetrics(SM_CXSCREEN);
		DX(adapterNativeHeight) = GetSystemMetrics(SM_CYSCREEN);
		nativeIsValid = DX(adapterNativeWidth) > 0 && DX(adapterNativeHeight) > 0;
	}
	DX(adapterNativeIsValid) = nativeIsValid;

	if (wndParms->fullscreen)
		R_SetFullscreenOutput();
	BOOL fullscreen;
	IDXGISwapChain_GetFullscreenState(DX(swapChain), &fullscreen, 0);
	if (!fullscreen && wndParms->fullscreen)
		ShowWindow(wndParms->hwnd, SW_MINIMIZE);

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	for (int i = 0; i < 4; i++)
	{
		bufferDesc.ByteWidth = s_constBufferSizes[i];
		ID3D11Device_CreateBuffer(DX(device), &bufferDesc, 0, &DX(constBuffer)[i]);
	}
	ID3D11DeviceContext_VSSetConstantBuffers(DX(context), 0, 4, DX(constBuffer));
	ID3D11DeviceContext_PSSetConstantBuffers(DX(context), 0, 4, DX(constBuffer));
	return hr;
}
#endif
