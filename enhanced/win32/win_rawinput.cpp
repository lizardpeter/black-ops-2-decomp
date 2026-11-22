// enhanced/win32/win_rawinput.cpp: raw mouse input (P0002; records/bugs.tsv B0015).
//
// The original turns the view by the Windows cursor's movement (GetCursorPos), after Windows' pointer speed and
// acceleration ("Enhance pointer precision") and clipped by the screen's edges, sampled once a frame. Here a thread
// of its own owns a message-only window registered for the mouse's raw input and adds every relative report, in
// device counts, to two 64-bit counters; IN_MouseMove takes what has accumulated since its last call. Nothing is
// lost between frames, nothing is rounded or accelerated, and the counters can't overflow however high the DPI.
//
// Polling rates: each report is one WM_INPUT on this thread, never on the game's, so an 8 kHz mouse costs the game
// nothing. RIDEV_INPUTSINK keeps reports coming whichever window is in front; reports while the game isn't the
// foreground window are dropped, so alt-tabbing back doesn't turn the view by what the mouse did elsewhere.
// Store build only (the build that is played).
#ifdef T6_STORE
#include "enhanced/raw_input.h"

#include "win32/win_local.h"

static long long s_rawDx;
static long long s_rawDy;
static bool s_rawInputActive;
static bool s_rawInputTried;
static HANDLE s_rawInputReady;

void IN_RawInputAdd(const RAWMOUSE *mouse)
{
	if (mouse->usFlags & MOUSE_MOVE_ABSOLUTE)
		return;
	__atomic_fetch_add(&s_rawDx, (long long)mouse->lLastX, __ATOMIC_RELAXED);
	__atomic_fetch_add(&s_rawDy, (long long)mouse->lLastY, __ATOMIC_RELAXED);
}

void IN_TakeRawMouse(int *dx, int *dy)
{
	long long x = __atomic_exchange_n(&s_rawDx, 0, __ATOMIC_RELAXED);
	long long y = __atomic_exchange_n(&s_rawDy, 0, __ATOMIC_RELAXED);
	// a frame can't plausibly hold 2^31 counts; clamp rather than wrap
	*dx = x > 0x7FFFFFFF ? 0x7FFFFFFF : x < -0x7FFFFFFF ? -0x7FFFFFFF : (int)x;
	*dy = y > 0x7FFFFFFF ? 0x7FFFFFFF : y < -0x7FFFFFFF ? -0x7FFFFFFF : (int)y;
}

// WM_INPUT on the raw input thread's window (not static: tests/enhanced/test_mouse.py drives it)
LRESULT WINAPI IN_RawInputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INPUT)
	{
		RAWINPUT raw;
		UINT size = sizeof(raw);
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != (UINT)-1 &&
		    raw.header.dwType == RIM_TYPEMOUSE && g_wv.hWnd && GetForegroundWindow() == g_wv.hWnd)
			IN_RawInputAdd(&raw.data.mouse);
	}
	return DefWindowProcA(hWnd, msg, wParam, lParam);  // also frees the WM_INPUT data
}

static DWORD WINAPI IN_RawInputThread(void *)
{
	WNDCLASSA wc = {};
	wc.lpfnWndProc = IN_RawInputWndProc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.lpszClassName = "T6RawInput";
	RegisterClassA(&wc);
	HWND hWnd = CreateWindowExA(0, wc.lpszClassName, "", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);

	RAWINPUTDEVICE device;
	device.usUsagePage = HID_USAGE_PAGE_GENERIC;
	device.usUsage = HID_USAGE_GENERIC_MOUSE;
	device.dwFlags = RIDEV_INPUTSINK;
	device.hwndTarget = hWnd;
	s_rawInputActive = hWnd && RegisterRawInputDevices(&device, 1, sizeof(device));
	SetEvent(s_rawInputReady);
	if (!s_rawInputActive)
		return 1;

	MSG msg;
	while (GetMessageA(&msg, nullptr, 0, 0) > 0)
		DispatchMessageA(&msg);
	return 0;
}

bool IN_StartRawInput()
{
	if (s_rawInputTried)
		return s_rawInputActive;
	s_rawInputTried = true;
	s_rawInputReady = CreateEventA(nullptr, TRUE, FALSE, nullptr);
	HANDLE thread = CreateThread(nullptr, 0, IN_RawInputThread, nullptr, 0, nullptr);
	if (!thread)
		return false;
	// above the game's threads, so a burst of reports is taken as it comes (the thread mostly sleeps in GetMessage)
	SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
	WaitForSingleObject(s_rawInputReady, INFINITE);
	CloseHandle(thread);
	CloseHandle(s_rawInputReady);
	return s_rawInputActive;
}
#endif
