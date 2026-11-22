// <d3d11.h> / <dxgi.h> COM interfaces: only the methods the reconstruction calls, at their vtable slots, called
// through C-style vtables (WINAPI: __stdcall on x86, the x64 convention on x64).
#pragma once

#include <dxgi.h>

#define T6_COM_CALL(obj, slot, type, ...) (((type)((*(void ***)(obj))[slot]))((obj), ##__VA_ARGS__))

typedef long HRESULT;

struct IUnknown;
struct ID3D11Resource;
struct ID3D11View;
struct ID3D11SamplerState;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct IDXGIOutput;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
struct ID3D11Device;
struct ID3D11Buffer;

typedef enum D3D_FEATURE_LEVEL
{
	D3D_FEATURE_LEVEL_10_0 = 0xA000,
	D3D_FEATURE_LEVEL_10_1 = 0xA100,
	D3D_FEATURE_LEVEL_11_0 = 0xB000,
} D3D_FEATURE_LEVEL;

typedef enum D3D_DRIVER_TYPE
{
	D3D_DRIVER_TYPE_UNKNOWN = 0,
	D3D_DRIVER_TYPE_HARDWARE = 1,
} D3D_DRIVER_TYPE;

#define D3D11_SDK_VERSION 7

typedef struct D3D11_BUFFER_DESC
{
	UINT ByteWidth;
	UINT Usage;           // D3D11_USAGE
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
	UINT StructureByteStride;
} D3D11_BUFFER_DESC;

#define D3D11_USAGE_DYNAMIC 2
#define D3D11_BIND_CONSTANT_BUFFER 4
#define D3D11_CPU_ACCESS_WRITE 0x10000

extern "C" __declspec(dllimport) HRESULT WINAPI CreateDXGIFactory1(const IID *riid, void **factory);
extern "C" __declspec(dllimport) HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
	IDXGIAdapter1 *adapter, D3D_DRIVER_TYPE driverType, void *software, UINT flags,
	const D3D_FEATURE_LEVEL *featureLevels, UINT featureLevelCount, UINT sdkVersion,
	const DXGI_SWAP_CHAIN_DESC *swapChainDesc, IDXGISwapChain **swapChain, ID3D11Device **device,
	D3D_FEATURE_LEVEL *featureLevel, ID3D11DeviceContext **immediateContext);

// IUnknown::Release (slot 2)
inline unsigned long IUnknown_Release(void *obj)
{
	return T6_COM_CALL(obj, 2, unsigned long(WINAPI *)(void *));
}

// IUnknown::QueryInterface (slot 0)
inline HRESULT IUnknown_QueryInterface(void *obj, const IID *riid, void **out)
{
	return T6_COM_CALL(obj, 0, HRESULT(WINAPI *)(void *, const IID *, void **), riid, out);
}

// IDXGIObject::GetParent (slot 6)
inline HRESULT IDXGIObject_GetParent(void *obj, const IID *riid, void **parent)
{
	return T6_COM_CALL(obj, 6, HRESULT(WINAPI *)(void *, const IID *, void **), riid, parent);
}

// IDXGIFactory1: MakeWindowAssociation (slot 8), EnumAdapters1 (slot 12)
inline HRESULT IDXGIFactory1_MakeWindowAssociation(IDXGIFactory1 *factory, HWND window, UINT flags)
{
	return T6_COM_CALL(factory, 8, HRESULT(WINAPI *)(IDXGIFactory1 *, HWND, UINT), window, flags);
}
inline HRESULT IDXGIFactory1_EnumAdapters1(IDXGIFactory1 *factory, UINT index, IDXGIAdapter1 **adapter)
{
	return T6_COM_CALL(factory, 12, HRESULT(WINAPI *)(IDXGIFactory1 *, UINT, IDXGIAdapter1 **), index, adapter);
}

// IDXGIOutput::GetDesc (slot 7)
inline HRESULT IDXGIOutput_GetDesc(IDXGIOutput *output, DXGI_OUTPUT_DESC *desc)
{
	return T6_COM_CALL(output, 7, HRESULT(WINAPI *)(IDXGIOutput *, DXGI_OUTPUT_DESC *), desc);
}

// ID3D11Device: CreateBuffer (slot 3), CheckMultisampleQualityLevels (slot 30)
inline HRESULT ID3D11Device_CreateBuffer(ID3D11Device *device, const D3D11_BUFFER_DESC *desc, const void *initialData,
                                         ID3D11Buffer **buffer)
{
	return T6_COM_CALL(device, 3, HRESULT(WINAPI *)(ID3D11Device *, const D3D11_BUFFER_DESC *, const void *,
	                                                ID3D11Buffer **), desc, initialData, buffer);
}
inline HRESULT ID3D11Device_CheckMultisampleQualityLevels(ID3D11Device *device, DXGI_FORMAT format, UINT sampleCount,
                                                          UINT *numQualityLevels)
{
	return T6_COM_CALL(device, 30, HRESULT(WINAPI *)(ID3D11Device *, DXGI_FORMAT, UINT, UINT *), format, sampleCount,
	                   numQualityLevels);
}

// ID3D11DeviceContext: VSSetConstantBuffers (slot 7), PSSetConstantBuffers (slot 16)
inline void ID3D11DeviceContext_VSSetConstantBuffers(ID3D11DeviceContext *context, UINT startSlot, UINT numBuffers,
                                                     ID3D11Buffer *const *buffers)
{
	T6_COM_CALL(context, 7, void(WINAPI *)(ID3D11DeviceContext *, UINT, UINT, ID3D11Buffer *const *), startSlot,
	            numBuffers, buffers);
}
inline void ID3D11DeviceContext_PSSetConstantBuffers(ID3D11DeviceContext *context, UINT startSlot, UINT numBuffers,
                                                     ID3D11Buffer *const *buffers)
{
	T6_COM_CALL(context, 16, void(WINAPI *)(ID3D11DeviceContext *, UINT, UINT, ID3D11Buffer *const *), startSlot,
	            numBuffers, buffers);
}

// IDXGISwapChain: Present (slot 8), GetFullscreenState (slot 11), GetContainingOutput (slot 15)
inline HRESULT IDXGISwapChain_GetContainingOutput(IDXGISwapChain *swapChain, IDXGIOutput **output)
{
	return T6_COM_CALL(swapChain, 15, HRESULT(WINAPI *)(IDXGISwapChain *, IDXGIOutput **), output);
}
inline HRESULT IDXGISwapChain_Present(IDXGISwapChain *swapChain, UINT syncInterval, UINT flags)
{
	return T6_COM_CALL(swapChain, 8, HRESULT(WINAPI *)(IDXGISwapChain *, UINT, UINT), syncInterval, flags);
}
inline HRESULT IDXGISwapChain_GetFullscreenState(IDXGISwapChain *swapChain, BOOL *fullscreen, IDXGIOutput **target)
{
	return T6_COM_CALL(swapChain, 11, HRESULT(WINAPI *)(IDXGISwapChain *, BOOL *, IDXGIOutput **), fullscreen,
	                   target);
}

// ID3D11View::GetResource (slot 7)
inline void ID3D11View_GetResource(ID3D11View *view, ID3D11Resource **resource)
{
	T6_COM_CALL(view, 7, void(WINAPI *)(ID3D11View *, ID3D11Resource **), resource);
}

// ID3D11DeviceContext: CopyResource (slot 47), ResolveSubresource (slot 57)
inline void ID3D11DeviceContext_CopyResource(ID3D11DeviceContext *context, ID3D11Resource *dst, ID3D11Resource *src)
{
	T6_COM_CALL(context, 47, void(WINAPI *)(ID3D11DeviceContext *, ID3D11Resource *, ID3D11Resource *), dst, src);
}
inline void ID3D11DeviceContext_ResolveSubresource(ID3D11DeviceContext *context, ID3D11Resource *dst, UINT dstSub,
                                                   ID3D11Resource *src, UINT srcSub, DXGI_FORMAT format)
{
	T6_COM_CALL(context, 57,
	            void(WINAPI *)(ID3D11DeviceContext *, ID3D11Resource *, UINT, ID3D11Resource *, UINT, DXGI_FORMAT),
	            dst, dstSub, src, srcSub, format);
}
