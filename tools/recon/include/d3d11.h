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

// IUnknown::Release (slot 2)
inline unsigned long IUnknown_Release(void *obj)
{
	return T6_COM_CALL(obj, 2, unsigned long(WINAPI *)(void *));
}

// IDXGISwapChain: Present (slot 8), GetFullscreenState (slot 11)
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
