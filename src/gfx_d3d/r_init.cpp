// gfx_d3d/r_init.cpp (CoDMPServer_PC: r_init.obj)
//
// Renderer start-up: the swap chain's description and the device's creation, which only the Store build has (the
// server's renderer has no device code), so the names are this reconstruction's (after Black Ops' R_CreateDevice and
// R_CreateDeviceInternal).
#define T6_SOURCE "gfx_d3d/r_init.cpp"

#include "gfx_d3d/r_dx.h"
#include "universal/dvar.h"
#include "win32/win_local.h"
#include "t6_static.h"

#include <string.h>

#ifdef T6_STORE
extern const dvar_t *r_aaSamples;
extern const dvar_t *r_aaSamplesMax;
extern const dvar_t *r_shaderWarming;
extern const dvar_t *r_txaaSupported;
extern "C" const IID IID_IDXGIFactory1;   // dxguid
extern "C" const IID IID_IDXGIDevice1;
extern "C" const IID IID_IDXGIAdapter1;
// NVIDIA's NVAPI and TXAA libraries, linked in
extern "C" int __cdecl NvAPI_Initialize();
extern "C" int __cdecl NvAPI_D3D11_CreateDeviceAndSwapChain(
	IDXGIAdapter1 *adapter, D3D_DRIVER_TYPE driverType, void *software, UINT flags,
	const D3D_FEATURE_LEVEL *featureLevels, UINT featureLevelCount, UINT sdkVersion,
	const DXGI_SWAP_CHAIN_DESC *swapChainDesc, IDXGISwapChain **swapChain, ID3D11Device **device,
	D3D_FEATURE_LEVEL *featureLevel, ID3D11DeviceContext **immediateContext, int *supportedLevel);
extern "C" int __cdecl NvAPI_Stereo_CreateHandleFromIUnknown(void *device, void **stereoHandle);
extern "C" int __cdecl NvAPI_Stereo_IsActivated(void *stereoHandle, unsigned char *isStereoOn);
extern "C" int __cdecl TxaaOpenDX(TxaaCtxDX *ctx, ID3D11Device *device, ID3D11DeviceContext *context);
void R_SetFullscreenOutput();  // 140720E00 (name made up): fullscreen on r_monitor's output

// 14071E360 (Store only; name made up)
// The multisampling the renderer uses for its own targets, from r_aaSamples (16 asks for 4 samples at quality 16),
// and the swap chain to create: the window's size, R8G8B8A8, one sample, two buffers, windowed (exclusive
// fullscreen is entered afterwards), DXGI_SWAP_EFFECT_DISCARD (the blt model), allowing mode switches.
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
}

T6_STATIC const D3D_FEATURE_LEVEL s_featureLevels[3] T6_AT("", "s_featureLevels")
	T6_INIT(= {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0});
// The four constant buffers bound to both the vertex and the pixel stage (bytes)
T6_STATIC const UINT s_constBufferSizes[4] T6_AT("", "s_constBufferSizes") T6_INIT(= {2448, 1040, 512, 192});

// 14071FEA0 (Store only; name made up)
// Creates the device, its immediate context and the swap chain from swapChainDesc for hwnd: through NVAPI on an
// NVIDIA adapter (which also opens stereo and TXAA), else D3D11CreateDeviceAndSwapChain, on the default adapter with
// feature level 11_0 down to 10_0. Then: DXGI may not handle Alt+Enter or the window's changes, the native resolution
// is read from the swap chain's output, exclusive fullscreen is entered when wndParms asks (the window minimized if
// it did not take), and the four constant buffers are created and bound. Returns the HRESULT of getting the DXGI
// factory back from the device (whether the device was created is not checked).
//
// behaviorFlags (Black Ops' D3D9 parameter) is not used; the caller leaves it unset. Its slot doubles as
// NvAPI_Stereo_IsActivated's output, which is read back whether the call succeeded or not.
HRESULT R_CreateDeviceInternal(HWND hwnd, int behaviorFlags, DXGI_SWAP_CHAIN_DESC *swapChainDesc,
                               const GfxWindowParms *wndParms)
{
	swapChainDesc->OutputWindow = hwnd;
	if (DX(vendorId) == 0x1002)  // AMD; dead: overwritten from r_shaderWarming just below
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
	if (DX(vendorId) == 0x10DE)  // NVIDIA
	{
		if (!DX(nvInitialized))
			DX(nvInitialized) = NvAPI_Initialize() == 0;
		int supportedLevel;
		if (NvAPI_D3D11_CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, s_featureLevels, 3,
		                                         D3D11_SDK_VERSION, swapChainDesc, &DX(swapChain), &DX(device),
		                                         &featureLevel, &DX(context), &supportedLevel) != 0
		    || supportedLevel < 1)
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
		D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, s_featureLevels, 3, D3D11_SDK_VERSION,
		                              swapChainDesc, &DX(swapChain), &DX(device), &featureLevel, &DX(context));
	}

	IDXGIDevice1 *dxgiDevice;
	IUnknown_QueryInterface(DX(device), &IID_IDXGIDevice1, (void **)&dxgiDevice);
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
