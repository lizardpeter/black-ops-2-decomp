// gfx_d3d/rb_backend.cpp (CoDMPServer_PC: rb_backend.obj)
//
// The render back end: presenting a frame, and (the Store build) the render thread.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\gfx_d3d\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "rb_backend.cpp"
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"

#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_rendercmds.h"
#include "qcommon/threads.h"
#include "ui/ui.h"
#include "t6_static.h"
#include "t6_assert.h"
#include "universal/dvar.h"
#include "universal/win_common.h"
#include "win32/win_local.h"

extern const dvar_t *r_vsync;            // gfx_d3d/r_dvars.cpp
extern const dvar_t *r_loadingScreen;
extern const dvar_t *r_texFilterQuality;
extern const dvar_t *r_warmShaders;
extern const dvar_t *r_fullscreen;
extern const dvar_t *r_convergence;
extern int g_showCursor;                 // win32/win_input.cpp
extern int g_showGameCursor;
void R_DX11_ReleaseSamplerStates();      // gfx_d3d/r_state.cpp
void Material_WarmAllShaders();          // gfx_d3d/r_material.cpp
extern "C" int __cdecl NvAPI_Stereo_IsActivated(void *stereoHandle, unsigned char *isStereoOn);
extern "C" int __cdecl NvAPI_Stereo_SetConvergence(void *stereoHandle, float newConvergence);

#ifdef T6_STORE
// Store only (names made up): what the Store revision adds around the present (not yet read)
void store_1404B15B0();
void store_1407362F0();
void store_140487390();
extern bool store_1421E2467;
extern bool store_14421181E;
// While this is set (it also stops the mouse turning the view: cl_input.cpp), the frame is copied out instead of
// presented: the back buffer is resolved into one texture, copied to another, and an event says it's ready.
extern ID3D11View *store_142C3F7D8;
extern ID3D11Resource *store_143063C70;
extern ID3D11Resource *store_143063C78;
extern HANDLE store_143063C80;
#else
void PIXBeginNamedEvent(int color, const char *name, ...);
extern "C" __declspec(dllimport) int WINAPI D3DPERF_EndEvent();
void TRACK_bg_weapons();
#endif

// 00A5A510  gfx_d3d/rb_backend.cpp:7158 (140735BC0 in the Store build)
// Presents the frame (v-sync while r_vsync or r_loadingScreen asks for it), then the end-of-frame chores: a changed
// texture filter, shader warming, the cursor's display count, the render thread's window messages, minimizing the
// window when exclusive fullscreen was lost, and NVIDIA stereo convergence.
void RB_SwapBuffers()
{
#ifdef T6_STORE
	store_1404B15B0();
	if (store_1421E2467 && store_14421181E)
		store_1407362F0();
	if (s_hiResShot.active)
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
		IDXGISwapChain *swapChain = DX(windows)[DX(targetWindowIndex)].swapChain;
		IDXGISwapChain_Present(swapChain, Dvar_GetBool(r_vsync) || Dvar_GetBool(r_loadingScreen) ? 1 : 0, 0);
	}
	store_140487390();
#else
	assertftext_at(7161, (DX(targetWindowIndex) >= 0 && DX(targetWindowIndex) < DX(windowCount)),
	               "(dx.targetWindowIndex >= 0 && dx.targetWindowIndex < dx.windowCount)",
	               "(dx.targetWindowIndex) = %i", DX(targetWindowIndex));
	PIXBeginNamedEvent(0, "Present");
	UINT syncInterval = Dvar_GetBool(r_vsync) || Dvar_GetBool(r_loadingScreen) ? 1 : 0;
	IDXGISwapChain_Present(DX(windows)[DX(targetWindowIndex)].swapChain, syncInterval, 0);
	if (Sys_IsRenderThread())
		D3DPERF_EndEvent();
	TRACK_bg_weapons();
#endif
	if (Dvar_HasLatchedValue(r_texFilterQuality))
	{
		Dvar_MakeLatchedValueCurrent(r_texFilterQuality);
		R_DX11_ReleaseSamplerStates();
		if (DX(resolutionCallback))
			DX(resolutionCallback)(false);
	}
	if (DX(needsShaderWarming) && Dvar_GetBool(r_warmShaders))
		Material_WarmAllShaders();
	// ShowCursor keeps a count: step it to 0 (shown) or -1 (hidden)
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
#ifdef T6_STORE
		g_wv.sysMsgTime = msg.time;
#endif
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

#ifdef T6_STORE
// ---- the render thread (Store only: the dedicated server has none; lineage: Black Ops' RB_RenderThread) -----------
#include <setjmp.h>
#include <string.h>

extern HANDLE store_144070008;     // names made up: two more requests to the render thread (what they ask is not read
extern HANDLE store_144070048;     // yet): 140721570 and 140721860 answer them
extern void *s_renderThreadBackEndData;  // the frame the render thread is drawing (name made up)
extern int store_142155DFC;        // Map is retried while this is set (name made up)
void RB_RenderThreadFrame();       // 140736950 (name made up): the back end, then RB_SwapBuffers
void RB_UploadDynamicBuffers(void *buffers);  // 140736BA0 (name made up)
void sub_140721570();
void sub_140721860();
void sub_140753660();
void sub_140752AA0();
void sub_1407370D0();
[[noreturn]] void sub_14039AE90();
void SCR_UpdateScreen();
void R_SetWndParms(GfxWindowParms *wndParms);              // 140720AE0
void R_StoreWindowSettings(const GfxWindowParms *wndParms);  // 14071E440
void sub_14071E360(DXGI_SWAP_CHAIN_DESC *swapChainDesc, const GfxWindowParms *wndParms);  // r_init.cpp


// Uploads the frame's dynamic data and draws it: under the device context's critical section, the skinned vertex
// cache is copied into its buffer (Map with WRITE_DISCARD, the smaller of the used and the buffer's size), then the three
// command buffers' index and vertex data; then the back end and the present.
static void RB_RenderThreadDrawFrame()
{
	Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
	char *data = (char *)s_renderThreadBackEndData;
	GfxVertexBufferState *skinnedCacheVb = GfxBackEndData_skinnedCacheVb((GfxBackEndData *)data);
	if (skinnedCacheVb->buffer)
	{
		int bytes = skinnedCacheVb->total;
		if (skinnedCacheVb->used < bytes)
			bytes = skinnedCacheVb->used;
		ID3D11DeviceContext *context = DX(context);
		ID3D11Buffer *buffer = skinnedCacheVb->buffer;
		D3D11_MAPPED_SUBRESOURCE mapped;
		do
			ID3D11DeviceContext_Map(context, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		while (store_142155DFC);
		memcpy(mapped.pData, GfxBackEndData_skinnedCacheVb((GfxBackEndData *)data)->verts, bytes);
		ID3D11DeviceContext_Unmap(DX(context), GfxBackEndData_skinnedCacheVb((GfxBackEndData *)data)->buffer, 0);
	}
	RB_UploadDynamicBuffers(data + 0x122C10);
	RB_UploadDynamicBuffers(*(void **)(data + 0x122C00));
	RB_UploadDynamicBuffers(data + 0x122C50);
	Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
	RB_RenderThreadFrame();
}

// The swap chain's buffers after the window changed (dx.doResizeBuffers): the window parms again from the dvars, the
// window's messages pumped, ResizeBuffers to the new size, and the resolution callback told whether the size changed.
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
	// the swap chain's flags are not passed on: ALLOW_MODE_SWITCH, as it was created with
	IDXGISwapChain_ResizeBuffers(DX(swapChain), swapChainDesc.BufferCount, swapChainDesc.BufferDesc.Width,
	                             swapChainDesc.BufferDesc.Height, swapChainDesc.BufferDesc.Format,
	                             DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	sub_140752AA0();
	sub_1407370D0();
	if (DX(resolutionCallback))
		DX(resolutionCallback)(sizeChanged);
	DX(doResizeBuffers) = false;
}

// 140736CD0 (Store only; started at 1403A1715 through 1403C2BD0)
// The render thread. Each pass: tells the main thread it runs, then takes the first of: a frame handed over in smpData
// (drawn and presented), the requests on store_144070008 and store_144070048, a resize of the swap chain's buffers;
// with nothing to do it yields (Sleep(0)) and goes round again: a busy wait (B0024). While another thread asks for
// remote screen updates (r_glob.remoteScreenUpdateNesting: the loading screen) it runs the screen update itself. An
// error (Com_Error's longjmp to the thread's jump buffer) during that screen update starts the loop again; during a
// frame it is fatal.
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
