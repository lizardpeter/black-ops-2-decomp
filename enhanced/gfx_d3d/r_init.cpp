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
// P0007 adds the swap chain's frame-latency waitable object (IDXGISwapChain2, latency 1), which the main thread waits on
// before its frame reads input (R_WaitForFrameLatency below; enhanced/qcommon/common.cpp).
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

// DXGI 1.5's factory and DXGI 1.3's swap chain (not in the exe's dxguid)
static const IID IID_IDXGISwapChain2_Enhanced = {0xA8BE2AC4, 0x199F, 0x4946, {0xB3, 0x31, 0x79, 0x59, 0x9F, 0xB9, 0x8D, 0xE7}};
static const IID IID_IDXGIFactory5_Enhanced = {0x7632E1F5, 0xEE65, 0x4DCA, {0x87, 0xFD, 0x84, 0xCD, 0x75, 0xF8, 0x83, 0x8D}};

bool r_flipModel = true;
bool r_allowTearing;
bool r_latencyWaitable = true;
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

// The next description down, from now on, after the system refused this one: without the waitable object first (before
// Windows 8.1), then the original blit model.
static void R_FallBack(DXGI_SWAP_CHAIN_DESC *swapChainDesc)
{
	if (swapChainDesc->Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
	{
		r_latencyWaitable = false;
		swapChainDesc->Flags &= ~(UINT)DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		return;
	}
	R_UseBlitModel(swapChainDesc);
}

// The original blit-model description, from now on (the system refused the flip model).
void R_UseBlitModel(DXGI_SWAP_CHAIN_DESC *swapChainDesc)
{
	r_flipModel = false;
	r_allowTearing = false;
	r_latencyWaitable = false;
	swapChainDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc->Flags &= ~(UINT)(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
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
		if (r_latencyWaitable)
			swapChainDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	}
}

// ---- P0007: the frame-latency waitable object ------------------------------------------------------------------------
static HANDLE s_frameLatencyWaitable;        // the swap chain's waitable object, or 0 (no waits)
static volatile long s_presentCount;         // presents on that swap chain (render thread)
static long s_waitedPresentCount;            // presents waited for (main thread)
static volatile bool s_resyncFrameLatency;   // a wait timed out: take every pending signal, then count again

// The new swap chain's waitable object, with at most one frame queued (IDXGISwapChain2::SetMaximumFrameLatency;
// IDXGIDevice1's doesn't apply to a waitable swap chain). The counts start again. Without one (a swap chain created
// without the waitable flag, or no IDXGISwapChain2), the main thread doesn't wait.
void R_AcquireFrameLatencyWaitable(bool waitableSwapChain)
{
	if (s_frameLatencyWaitable)
		CloseHandle(s_frameLatencyWaitable);
	s_frameLatencyWaitable = 0;
	s_presentCount = 0;
	s_waitedPresentCount = 0;
	s_resyncFrameLatency = false;
	if (!waitableSwapChain || !DX(swapChain))
		return;
	void *swapChain2 = 0;
	if (IUnknown_QueryInterface(DX(swapChain), &IID_IDXGISwapChain2_Enhanced, &swapChain2) < 0 || !swapChain2)
		return;
	IDXGISwapChain2_SetMaximumFrameLatency(swapChain2, 1);
	s_frameLatencyWaitable = IDXGISwapChain2_GetFrameLatencyWaitableObject(swapChain2);
	IUnknown_Release(swapChain2);
}

void R_CountPresent()
{
	if (s_frameLatencyWaitable)
		InterlockedIncrement(&s_presentCount);
}

// How long one wait may take before it counts as a missed signal: two refresh periods, at least 50 ms
static DWORD R_FrameLatencyTimeout()
{
	DWORD timeout = 50;
	if (vidConfig.displayFrequency)
	{
		DWORD periods = (2000 + vidConfig.displayFrequency - 1) / vidConfig.displayFrequency;
		if (periods > timeout)
			timeout = periods > 1000 ? 1000 : periods;
	}
	return timeout;
}

// Waits once for each present counted since the last wait (ghosts-decomp's P0004). With latency 1 the wait for present
// p returns once present p-1 has been displayed, so one frame stays queued while this frame's input is read. Frames
// that weren't presented are never waited for, and the presents of the render thread's loading screens are paired up
// afterwards (their waits return at once). A wait that times out leaves its signal unaccounted for, which would let
// one more frame queue up, so the next frame takes every pending signal instead of counting (that frame may wait up
// to one refresh longer, once).
void R_WaitForFrameLatency()
{
	HANDLE waitable = s_frameLatencyWaitable;
	if (!waitable)
		return;
	long presented = s_presentCount;
	if (s_resyncFrameLatency)
	{
		s_resyncFrameLatency = false;
		while (WaitForSingleObject(waitable, 0) == WAIT_OBJECT_0)
			;
		s_waitedPresentCount = presented;
		return;
	}
	DWORD timeout = R_FrameLatencyTimeout();
	while (s_waitedPresentCount != presented)
	{
		++s_waitedPresentCount;
		if (WaitForSingleObject(waitable, timeout) != WAIT_OBJECT_0)
		{
			s_waitedPresentCount = presented;
			s_resyncFrameLatency = true;
			return;
		}
	}
}

// src: Store 14071FEA0 R_CreateDeviceInternal. Enhanced: when the device and its flip-model swap chain can't be made,
// they are made again with the next description down (without the waitable object, then the original swap chain);
// the device queues at most one frame; and the waitable object is taken (P0007).
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
		while (status != 0 && swapChainDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
		{
			R_FallBack(swapChainDesc);
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
		while (created < 0 && swapChainDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
		{
			R_FallBack(swapChainDesc);
			created = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, s_featureLevels, 3,
			                                        D3D11_SDK_VERSION, swapChainDesc, &DX(swapChain), &DX(device),
			                                        &featureLevel, &DX(context));
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
	R_AcquireFrameLatencyWaitable((swapChainDesc->Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) != 0);

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
