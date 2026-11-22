// enhanced/win32/win_keytime.cpp: key events on the game's clock, timed to the millisecond (P0004; B0019, B0020).
//
// The original gives each key event the Windows message's time (MSG.time: GetTickCount, milliseconds since the PC
// booted, in 15.6 ms steps), but the buttons measure against com_frameTime (timeGetTime since the game started).
// So a movement key's press frame counted nothing and its release frame counted in full (B0019), and even on one
// clock a key's times were only known to about 16 ms (B0020).
//
// Here the game's window procedure is subclassed: before the original handles a message, g_wv.sysMsgTime (the time
// it gives the events it queues) is set on the game's clock. For a key, it's the time the raw input thread saw the
// key change, read from the game's own clock (timeGetTime, 1 ms: the game sets timeBeginPeriod(1)); for anything
// else, or a key the raw input thread didn't see, the message's time moved onto the game's clock (to about 16 ms).
// Everything else (bindings, text, menus) still comes from the window's messages as before.
// Store build only.
#ifdef T6_STORE
#include "enhanced/raw_input.h"

#include "win32/win_local.h"

// win32/win_shared.cpp: Sys_Milliseconds' base, and whether it's set
extern int sys_timeBase;
extern int sys_timeInitialized __asm__("t6s@win32/win_shared.cpp@Sys_Milliseconds@initialized");

// A raw key change: per virtual key, a ring the raw input thread fills and the window procedure empties in order.
struct KeyTransition
{
	int time;
	bool down;
};
#define KEY_RING 8
#define KEY_MATCH_MS 250  // a raw change belongs to a message whose own (16 ms) time is this close
static KeyTransition s_keyRing[256][KEY_RING];
static unsigned char s_keyHead[256];  // written by the raw input thread
static unsigned char s_keyTail[256];  // written by the game's window thread
static HWND s_timedWindow;
static WNDPROC s_gameWndProc;

// The game's clock (Sys_Milliseconds) without its side effect: false until the game has set it up.
bool IN_GameMilliseconds(int *ms)
{
	if (!sys_timeInitialized)
		return false;
	*ms = (int)(timeGetTime() - (DWORD)sys_timeBase);
	return true;
}

void IN_RawKeyAdd(const RAWKEYBOARD *key)
{
	int vk = key->VKey;
	if (!vk || vk >= 0xFF)
		return;  // no key, or the fake prefix key of an escape sequence
	int now;
	if (!IN_GameMilliseconds(&now))
		return;
	unsigned char head = s_keyHead[vk];
	if ((unsigned char)(head - __atomic_load_n(&s_keyTail[vk], __ATOMIC_ACQUIRE)) >= KEY_RING)
		return;  // the window isn't taking them: its messages fall back to their own times
	s_keyRing[vk][head % KEY_RING] = {now, !(key->Flags & RI_KEY_BREAK)};
	__atomic_store_n(&s_keyHead[vk], (unsigned char)(head + 1), __ATOMIC_RELEASE);
}

// The time of a key message: the first raw change of the key in the same direction close to the message's own
// time. Changes before it that don't match (missed or out of step) are dropped.
static int IN_KeyMessageTime(UINT msg, WPARAM wParam, int fallback)
{
	bool down = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
	int vk = (int)(wParam & 0xFF);
	unsigned char tail = s_keyTail[vk];
	unsigned char head = __atomic_load_n(&s_keyHead[vk], __ATOMIC_ACQUIRE);
	int time = fallback;
	while (tail != head)
	{
		KeyTransition t = s_keyRing[vk][tail % KEY_RING];
		tail++;
		if (t.down == down && t.time - fallback <= KEY_MATCH_MS && fallback - t.time <= KEY_MATCH_MS)
		{
			time = t.time;
			break;
		}
	}
	__atomic_store_n(&s_keyTail[vk], tail, __ATOMIC_RELEASE);
	return time;
}

// The game's window procedure, subclassed (not static: tests/enhanced/test_keytime.py drives it)
LRESULT WINAPI IN_KeyTimeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int now;
	if (IN_GameMilliseconds(&now))
	{
		// the message's time (GetTickCount's clock) moved onto the game's
		int time = now - (int)(GetTickCount() - (DWORD)GetMessageTime());
		if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)
			time = IN_KeyMessageTime(msg, wParam, time);
		g_wv.sysMsgTime = time;
	}
	return CallWindowProcA(s_gameWndProc, hWnd, msg, wParam, lParam);
}

void IN_TimeWindowMessages()
{
	HWND hWnd = g_wv.hWnd;
	if (!hWnd || hWnd == s_timedWindow)
		return;
	WNDPROC previous = (WNDPROC)SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (__INTPTR_TYPE__)IN_KeyTimeWndProc);
	if (previous)
	{
		s_gameWndProc = previous;
		s_timedWindow = hWnd;
	}
}
#endif
