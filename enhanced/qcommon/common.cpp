// enhanced/qcommon/common.cpp: the frame cap to the microsecond, waited for precisely (P0006; B0026).
//
// Replaces Com_Frame_Try_Block_Function's wait (src: Com_FrameWait). The original waits for 1000 / com_maxfps whole
// milliseconds since the last frame, polling with Sleep(1): 144 runs at 166 fps, 240 at 250, and each frame can end up
// to a millisecond or more late. Here:
// - frames start on a cadence of exactly 1 / com_maxfps seconds on the performance counter (the server's
//   sv_clientFpsLimit still caps it): 144 is 144. A frame that falls more than a frame behind starts the cadence again
//   from now instead of hurrying to catch up;
// - the wait sleeps on a high-resolution waitable timer and finishes with a short spin, so it ends within a few
//   microseconds of its deadline, without a busy CPU for most of it;
// - the event loop still runs at least once a millisecond while waiting, as before, and once more when it ends;
// - a frame still lasts at least until the game's millisecond clock moves (the engine counts whole milliseconds), so
//   the frame rate stays at most 1000.
// Everything else is the original's (src/qcommon/common.cpp: Com_FrameAfterWait). Store build only.
#ifdef T6_STORE
#define T6_SOURCE "qcommon/common.cpp"

#include "client/client.h"
#include "qcommon/qcommon.h"
#include "t6_static.h"
#include "win32/win_shared.h"

#include <windows.h>

T6_STATIC int com_lastFrameIndex T6_AT("", "com_lastFrameIndex");
extern int store_14226A120;
void sub_1403A2FF0(LocalClientNum_t localClientNum);
void Com_EventLoop();
int Com_FrameMaxFps();
void Com_FrameAfterWait(LocalClientNum_t localClientNum, int minMsec, int index, int msec);

static long long s_qpcFrequency;    // performance counter ticks per second (0 until the first frame)
static long long s_nextFrame;       // the counter when the next frame may start
static HANDLE s_frameTimer;         // a high-resolution waitable timer, or 0 (then Sleep)

static long long Com_Qpc()
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return now.QuadPart;
}

// Sleeps until about `ticks` from now: on the timer for all but the last half millisecond, which is spun through.
// Without the timer (before Windows 10 1803), Sleep(1) while a millisecond or more is left beyond that.
static void Com_SleepTicks(long long ticks)
{
	long long spin = s_qpcFrequency / 2000;
	if (ticks > spin)
	{
		long long sleep100ns = (ticks - spin) * 10000000 / s_qpcFrequency;
		if (s_frameTimer)
		{
			LARGE_INTEGER due;
			due.QuadPart = -sleep100ns;  // relative
			if (SetWaitableTimer(s_frameTimer, &due, 0, 0, 0, FALSE))
			{
				WaitForSingleObject(s_frameTimer, INFINITE);
				return;
			}
		}
		if (sleep100ns >= 10000)
		{
			Sleep(1);
			return;
		}
	}
	long long end = Com_Qpc() + ticks;
	while (Com_Qpc() < end)
		;
}

// src: Com_FrameWait. Enhanced: the cadence and the precise wait (see the top of this file). Returns the frame's msec
// on the game's millisecond clock (at least 1) and sets com_frameTime, as the original.
int Com_FrameWaitPrecise(int maxFps)
{
	if (!s_qpcFrequency)
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		s_qpcFrequency = frequency.QuadPart;
		s_frameTimer = CreateWaitableTimerExW(0, 0, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	}
	long long period = maxFps > 0 ? s_qpcFrequency / maxFps : 0;
	long long millisecond = s_qpcFrequency / 1000;
	for (;;)
	{
		Com_EventLoop();
		int now = Sys_Milliseconds();
		if (now - com_frameTime < 0)
			com_frameTime = now;
		int msec = now - com_frameTime;
		long long qpc = Com_Qpc();
		if (msec >= 1 && qpc >= s_nextFrame)
		{
			// the next frame one period after this one was due, unless this one is more than a period late
			s_nextFrame = period && qpc - s_nextFrame < period ? s_nextFrame + period : qpc + period;
			com_frameTime = now;
			return msec;
		}
		long long wait = msec < 1 ? millisecond / 4 : s_nextFrame - qpc;  // the clock's next tick, or the deadline
		if (wait > millisecond)
			wait = millisecond;  // the event loop at least once a millisecond
		Com_SleepTicks(wait);
	}
}

// src: Store 1403A4010 Com_Frame_Try_Block_Function. Enhanced: the wait (Com_FrameWaitPrecise).
void Com_Frame_Try_Block_Function()
{
	LocalClientNum_t localClientNum = Com_LocalClients_GetPrimary();
	sub_1403A2FF0(localClientNum);
	store_14226A120 = 0;
	int minMsec = 1;
	int maxFps = Com_FrameMaxFps();
	if (maxFps > 0)
	{
		minMsec = 1000u / (unsigned int)maxFps;
		if (!minMsec)
			minMsec = 1;
	}
	int index = com_lastFrameIndex % 3;
	com_lastFrameIndex++;
	int msec = Com_FrameWaitPrecise(maxFps);
	Com_FrameAfterWait(localClientNum, minMsec, index, msec);
}
#endif
