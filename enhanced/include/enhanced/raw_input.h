// enhanced/raw_input.h: raw mouse input (enhanced/win32/win_rawinput.cpp, P0002).
#pragma once

#include <windows.h>

// Starts the raw input thread once; later calls return its state. False if the mouse couldn't be registered for raw
// input: the mouse then doesn't turn the view (raw input is the only mode).
bool IN_StartRawInput();

// Adds one raw mouse report to the counters: relative movement only. Absolute reports (tablets, remote desktop,
// some virtual machines) carry positions, not movement, and are ignored.
void IN_RawInputAdd(const RAWMOUSE *mouse);

// The raw input thread's window procedure: WM_INPUT mouse reports, while the game is the foreground window, go to
// IN_RawInputAdd.
LRESULT WINAPI IN_RawInputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// The movement since the last call, in device counts; the counters restart at zero.
void IN_TakeRawMouse(int *dx, int *dy);
