// gfx_d3d/rb_backend.cpp (CoDMPServer_PC: rb_backend.obj)
//
// The render back end. So far the end of a frame: presenting it.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\gfx_d3d\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "rb_backend.cpp"
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"

#include "gfx_d3d/r_dx.h"
#include "qcommon/threads.h"
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
extern int store_1427F1B60;
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
