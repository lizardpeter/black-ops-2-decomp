// enhanced/raw_input.h: raw mouse input (enhanced/win32/win_rawinput.cpp, P0002) and key timing
// (enhanced/win32/win_keytime.cpp, P0004).
#pragma once

#include <windows.h>

// Starts the raw input thread once; later calls return its state. False if the mouse couldn't be registered for raw
// input: IN_MouseMove then stops the game with an error (raw input is the only mode; no fallback to the cursor).
bool IN_StartRawInput();

// The Windows error code of the failed raw input registration (or thread start).
unsigned int IN_RawInputError();

// Adds one raw mouse report to the counters: relative movement only. Absolute reports (tablets, remote desktop,
// some virtual machines) carry positions, not movement, and are ignored.
void IN_RawInputAdd(const RAWMOUSE *mouse);

// The raw input thread's window procedure: WM_INPUT mouse reports, while the game is the foreground window, go to
// IN_RawInputAdd.
LRESULT WINAPI IN_RawInputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// The movement since the last call, in device counts; the counters restart at zero.
void IN_TakeRawMouse(int *dx, int *dy);

// The game's clock (Sys_Milliseconds, without starting it): false until the game has set it up.
bool IN_GameMilliseconds(int *ms);

// Records one raw key change (the raw input thread, while the game is the foreground window) with the game's time.
void IN_RawKeyAdd(const RAWKEYBOARD *key);

// Subclasses the game's window (once per window) so its messages' events carry times on the game's clock.
void IN_TimeWindowMessages();

// The subclassed window procedure.
LRESULT WINAPI IN_KeyTimeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
