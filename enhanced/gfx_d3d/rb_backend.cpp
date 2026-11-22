// enhanced/gfx_d3d/rb_backend.cpp: presenting on the flip-model swap chain (P0005; enhanced/gfx_d3d/r_init.cpp).
//
// Replaces RB_SwapBuffers and RB_RenderThread:
// - RB_SwapBuffers presents with DXGI_PRESENT_ALLOW_TEARING when the swap chain allows it, v-sync is off and the
//   window isn't in exclusive fullscreen (which tears with sync interval 0 anyway), so the frame rate isn't held to
//   the display's in a window; and since a flip-model present unbinds the back buffer, the render targets bound before
//   it are bound again after it (the renderer's state cache still believes they are);
// - RB_RenderThread resizes the swap chain's buffers with the swap chain's own flags (B0025): a flip-model swap chain
//   created with ALLOW_TEARING can't be resized without it.
// Store build only. Everything else is the original's (src/gfx_d3d/rb_backend.cpp).
#ifdef T6_STORE
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"

#include "enhanced/dxgi_flip.h"

#include "gfx_d3d/r_dx.h"
#include "qcommon/threads.h"
#include "t6_static.h"
#include "universal/dvar.h"
#include "universal/win_common.h"
#include "win32/win_local.h"

#include <setjmp.h>
#include <string.h>

extern const dvar_t *r_vsync;
extern const dvar_t *r_loadingScreen;
extern const dvar_t *r_texFilterQuality;
extern const dvar_t *r_warmShaders;
extern const dvar_t *r_fullscreen;
extern const dvar_t *r_convergence;
extern int g_showCursor;
extern int g_showGameCursor;
void R_DX11_ReleaseSamplerStates();
void Material_WarmAllShaders();
extern "C" int __cdecl NvAPI_Stereo_IsActivated(void *stereoHandle, unsigned char *isStereoOn);
extern "C" int __cdecl NvAPI_Stereo_SetConvergence(void *stereoHandle, float newConvergence);
void store_1404B15B0();
void store_1407362F0();
void store_140487390();
extern bool store_1421E2467;
extern bool store_14421181E;
extern int store_1427F1B60;
extern ID3D11View *store_142C3F7D8;
extern ID3D11Resource *store_143063C70;
extern ID3D11Resource *store_143063C78;
extern HANDLE store_143063C80;

// Presents the frame (P0005): tearing allowed when the swap chain allows it, without v-sync, in a window; the render
// targets bound before a flip-model present are bound again after it.
HRESULT R_Present(IDXGISwapChain *swapChain, UINT syncInterval)
{
	UINT flags = 0;
	if (r_allowTearing && syncInterval == 0)
	{
		BOOL fullscreen = FALSE;
		IDXGISwapChain_GetFullscreenState(swapChain, &fullscreen, 0);
		if (!fullscreen)
			flags = DXGI_PRESENT_ALLOW_TEARING;
	}
	ID3D11RenderTargetView *renderTarget = 0;
	ID3D11DepthStencilView *depthStencil = 0;
	if (r_flipModel)
	{
		Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
		ID3D11DeviceContext_OMGetRenderTargets(DX(context), 1, &renderTarget, &depthStencil);
		Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
	}
	HRESULT hr = IDXGISwapChain_Present(swapChain, syncInterval, flags);
	if (r_flipModel)
	{
		Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
		ID3D11DeviceContext_OMSetRenderTargets(DX(context), 1, &renderTarget, depthStencil);
		Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
		if (renderTarget)
			IUnknown_Release(renderTarget);
		if (depthStencil)
			IUnknown_Release(depthStencil);
	}
	return hr;
}

// src: Store 140735BC0 RB_SwapBuffers. Enhanced: the present goes through R_Present.
void RB_SwapBuffers()
{
	store_1404B15B0();
	if (store_1421E2467 && store_14421181E)
		store_1407362F0();
	if (store_1427F1B60)
	{
		Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
		ID3D11Resource *backBuffer;
		ID3D11View_GetResource(store_142C3F7D8, &backBuffer);
		ID3D11DeviceContext_ResolveSubresource(DX(context), store_143063C70, 0, backBuffer, 0,
		                                       DXGI_FORMAT_R8G8B8A8_UNORM);
		IUnknown_Release(backBuffer);
		ID3D11DeviceContext_CopyResource(DX(context), store_143063C78, store_143063C70);
		Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
		SetEvent(store_143063C80);
	}
	else
	{
		R_Present(DX(windows)[DX(targetWindowIndex)].swapChain,
		          Dvar_GetBool(r_vsync) || Dvar_GetBool(r_loadingScreen) ? 1 : 0);
	}
	store_140487390();
	if (Dvar_HasLatchedValue(r_texFilterQuality))
	{
		Dvar_MakeLatchedValueCurrent(r_texFilterQuality);
		R_DX11_ReleaseSamplerStates();
		if (DX(resolutionCallback))
			DX(resolutionCallback)(false);
	}
	if (DX(needsShaderWarming) && Dvar_GetBool(r_warmShaders))
		Material_WarmAllShaders();
	int wanted;
	BOOL show;
	if (!g_showCursor && !g_showGameCursor)
	{
		wanted = -1;
		show = FALSE;
	}
	else
	{
		wanted = 0;
		show = g_showCursor || g_showGameCursor;
	}
	for (int count = ShowCursor(show); count != wanted; count = ShowCursor(count < wanted))
		;
	MSG msg;
	while (PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessageA(&msg, 0, 0, 0))
			Sys_SetWin32QuitEvent();
		g_wv.sysMsgTime = msg.time;
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	if (Dvar_GetInt(r_fullscreen) == 1)
	{
		BOOL fullscreen;
		IDXGISwapChain_GetFullscreenState(DX(swapChain), &fullscreen, 0);
		if (!fullscreen)
			ShowWindow(DX(windows)[0].hwnd, SW_MINIMIZE);
	}
	if (DX(nvStereoHandle))
	{
		unsigned char stereoActivated = 0;
		if (NvAPI_Stereo_IsActivated(DX(nvStereoHandle), &stereoActivated) == 0)
		{
			DX(nvStereoActivated) = stereoActivated != 0;
			if (DX(nvStereoActivated))
				NvAPI_Stereo_SetConvergence(DX(nvStereoHandle), Dvar_GetFloat(r_convergence));
			return;
		}
	}
	DX(nvStereoActivated) = false;
}

// ---- the render thread ---------------------------------------------------------------------------------------------
extern HANDLE store_144070008;
extern HANDLE store_144070048;
extern void *s_renderThreadBackEndData;
extern int store_142155DFC;
void RB_RenderThreadFrame();
void RB_UploadDynamicBuffers(void *buffers);
void sub_140721570();
void sub_140721860();
void sub_140753660();
void sub_140752AA0();
void sub_1407370D0();
[[noreturn]] void sub_14039AE90();
void SCR_UpdateScreen();
void R_SetWndParms(GfxWindowParms *wndParms);
void R_StoreWindowSettings(const GfxWindowParms *wndParms);
void sub_14071E360(DXGI_SWAP_CHAIN_DESC *swapChainDesc, const GfxWindowParms *wndParms);

struct GfxDynamicUpload
{
	int used;
	int size;
	ID3D11Buffer *buffer;
	const void *data;
};

static void RB_RenderThreadDrawFrame()
{
	Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
	char *data = (char *)s_renderThreadBackEndData;
	GfxDynamicUpload *upload = *(GfxDynamicUpload **)(data + 0x4BE9C0);
	if (upload->buffer)
	{
		int bytes = upload->size;
		if (upload->used < bytes)
			bytes = upload->used;
		ID3D11DeviceContext *context = DX(context);
		ID3D11Buffer *buffer = upload->buffer;
		D3D11_MAPPED_SUBRESOURCE mapped;
		do
			ID3D11DeviceContext_Map(context, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		while (store_142155DFC);
		memcpy(mapped.pData, (*(GfxDynamicUpload **)(data + 0x4BE9C0))->data, bytes);
		ID3D11DeviceContext_Unmap(DX(context), (*(GfxDynamicUpload **)(data + 0x4BE9C0))->buffer, 0);
	}
	RB_UploadDynamicBuffers(data + 0x122C10);
	RB_UploadDynamicBuffers(*(void **)(data + 0x122C00));
	RB_UploadDynamicBuffers(data + 0x122C50);
	Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
	RB_RenderThreadFrame();
}

// src: RB_ResizeBuffers (inlined in RB_RenderThread). Enhanced: ResizeBuffers with the description's flags, the ones
// the swap chain was created with (B0025).
static void RB_ResizeBuffers()
{
	sub_140753660();
	if (DX(backBuffer))
	{
		IUnknown_Release(DX(backBuffer));
		DX(backBuffer) = 0;
	}
	GfxWindowParms wndParms;
	R_SetWndParms(&wndParms);
	wndParms.hwnd = DX(windows)[0].hwnd;
	bool sizeChanged = !((unsigned int)wndParms.displayWidth == vidConfig.displayWidth
	                     && (unsigned int)wndParms.displayHeight == vidConfig.displayHeight);
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	sub_14071E360(&swapChainDesc, &wndParms);
	R_StoreWindowSettings(&wndParms);
	MSG msg;
	while (PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessageA(&msg, 0, 0, 0))
			Sys_SetWin32QuitEvent();
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	IDXGISwapChain_ResizeBuffers(DX(swapChain), swapChainDesc.BufferCount, swapChainDesc.BufferDesc.Width,
	                             swapChainDesc.BufferDesc.Height, swapChainDesc.BufferDesc.Format,
	                             swapChainDesc.Flags);
	sub_140752AA0();
	sub_1407370D0();
	if (DX(resolutionCallback))
		DX(resolutionCallback)(sizeChanged);
	DX(doResizeBuffers) = false;
}

// src: Store 140736CD0 RB_RenderThread. Enhanced: its resize (RB_ResizeBuffers above).
void RB_RenderThread(unsigned int threadContext)
{
	for (;;)
	{
		jmp_buf *abortframe = (jmp_buf *)Sys_GetValue(2);
		if (setjmp(*abortframe))
		{
			if (r_glob.isRenderingRemoteUpdate)
			{
				r_glob.isRenderingRemoteUpdate = false;
				r_glob.screenUpdateNotify = 1;
				s_renderThreadBackEndData = 0;
				continue;
			}
			if (s_renderThreadBackEndData)
				sub_14039AE90();
			continue;
		}
		for (;;)
		{
			Sys_RendererRunning();
			if (Sys_RendererWorkPending())
			{
				s_renderThreadBackEndData = Sys_TakeRendererData();
				if (s_renderThreadBackEndData)
					RB_RenderThreadDrawFrame();
				s_renderThreadBackEndData = 0;
			}
			else if (WaitForSingleObject(store_144070008, 0) == WAIT_OBJECT_0)
			{
				sub_140721570();
				WaitForSingleObject(store_144070008, INFINITE);
				ResetEvent(store_144070008);
			}
			else if (WaitForSingleObject(store_144070048, 0) == WAIT_OBJECT_0)
			{
				sub_140721860();
				ResetEvent(store_144070048);
			}
			else if (!DX(doResizeBuffers))
			{
				Sleep(0);
			}
			else if (DX(windows)[0].hwnd && DX(swapChain))
			{
				RB_ResizeBuffers();
			}
			if (r_glob.remoteScreenUpdateNesting)
			{
				r_glob.screenUpdateNotify = 1;
				r_glob.isRenderingRemoteUpdate = true;
				do
					SCR_UpdateScreen();
				while (r_glob.remoteScreenUpdateNesting);
				r_glob.isRenderingRemoteUpdate = false;
				r_glob.screenUpdateNotify = 1;
			}
		}
	}
}
#endif
