// qcommon/common.cpp (CoDMPServer_PC: common.obj)
//
// The engine's common code. So far slow motion's scaling of the view's frame time.
#define T6_SOURCE "qcommon/common.cpp"

#include "t6_static.h"
#include "qcommon/qcommon.h"

T6_GLOBAL SlowMotionCommon g_slowmoCommon;

// 006BF470  qcommon/common.cpp:6125 (inlined in the Store build)
// A frame's time in seconds, scaled by the view's slow-motion timescale, which moves linearly from its start to its
// end value over the change's duration. The result is at least 0.001 (a NaN gives 0.001 too). The Store's /fp:fast
// copy tests the end differently when a timescale is NaN.
float Com_ViewScaleMsec(float sec)
{
	float msec = sec * 1000.0f;
	if (g_slowmoCommon.viewEnable)
	{
		float start = g_slowmoCommon.viewStartTimescale;
		float end = g_slowmoCommon.viewEndTimescale;
		bool done;
		if (g_slowmoCommon.viewEndMsec == g_slowmoCommon.viewStartMsec)
		{
			done = true;
		}
		else
		{
			g_slowmoCommon.viewTimescale += (end - start) * msec /
			                                (float)(g_slowmoCommon.viewEndMsec - g_slowmoCommon.viewStartMsec);
			float cur = g_slowmoCommon.viewTimescale;
#ifdef T6_STORE
			done = (!(start >= end) && cur >= end) || (start > end && !(cur > end)) || (!(start < end) && !(start > end));
#else
			done = (start < end && cur >= end) || (start > end && end >= cur) || start == end;
#endif
		}
		if (done)
		{
			g_slowmoCommon.viewTimescale = end;
			g_slowmoCommon.viewEnable = false;
		}
	}
	float scaled = g_slowmoCommon.viewTimescale * msec * 0.001f;
#ifdef T6_STORE
	return scaled >= 0.001f ? scaled : 0.001f;
#else
	return scaled - 0.001f >= 0.0f ? scaled : 0.001f;
#endif
}

#ifdef T6_STORE
// ---- the frame (the Store build: a client's; the dedicated server's Com_Frame_Try_Block_Function, 006BF540, runs the
// server instead and is not reconstructed yet) --------------------------------------------------------------------
#include "client/client.h"
#include "demo/demo.h"
#include "universal/dvar.h"
#include "universal/win_common.h"
#include "win32/win_gamepad.h"
#include "win32/win_local.h"
#include "win32/win_shared.h"

#include <windows.h>

void *operator new(unsigned long long size);
void operator delete(void *p, unsigned long long size) noexcept;

extern const dvar_t *com_maxfps;
extern const dvar_t *com_dedicated;
extern const dvar_t *com_statmon;
extern const dvar_t *cl_paused;
extern const dvar_t *sv_running;
T6_STATIC int com_lastFrameIndex T6_AT("", "com_lastFrameIndex");
T6_STATIC int com_lastFrameTime[3] T6_AT("", "com_lastFrameTime");
T6_STATIC float com_lastFrameTime_base[3] T6_AT("", "com_lastFrameTime_base");
bool Com_IsDedicatedServer();
void Com_EventLoop();
void Cbuf_Execute(LocalClientNum_t localClientNum, ControllerIndex_t controllerIndex);
void TaskManager2_ProcessTasks(ControllerIndex_t controllerIndex);
void DB_LoadLoadFastfilesForNewContent();
void Demo_Frame(int msec, int scaledMsec);
int Com_TimeScaleMsec(int msec);  // 1403A3970: slow motion, the timescales, fixed time
void Phys_RunToTime(int time);
void SCR_UpdateScreen();

// The Store build's additions to the frame (names made up; what most of them do is not read yet)
struct StoreFrameServerInfo { unsigned char unknown0[0x1C54]; int clientFpsLimit; };  // the server's sv_clientFpsLimit
extern StoreFrameServerInfo *store_140E53B70;
extern int store_14226A120;
extern int store_1427F1B58;        // the frame's end waits until Sys_Milliseconds reaches this (set to the averaged msec)
extern int store_140E5C804;        // the time since the previous frame by com_frameTime, 1 to 200
extern int store_140E5CB94;
extern int store_143C0A758;        // set after the first frame
extern unsigned int store_142161684;
extern int store_140E61798;
extern bool store_140A5AAAB;
extern bool store_1443C561D[][0xB0];  // a flag per controller, set once under CRITSECT_AUDIO_PHYSICS
extern bool store_1443C56DD;
extern unsigned long long store_143ED9A18[];
extern int store_142155E08;
extern int store_1444BA284;        // com_statmon: the previous frame's time
// a queue of calls run on the main thread, at most 10 ms of them a frame
struct StoreQueuedCall { bool pending; void (*func)(void *); void *arg; };
extern bool store_14413FEED;
extern volatile long store_14413FEE0;
extern volatile int store_14413FEE4;
extern int store_14413FEE8;
extern StoreQueuedCall store_1441AFEF0[];
// an online task started 30 s after 143ED03D4 (once)
struct StoreFrameTask { void *context; void *unknown8; void (*func)(); void *unknown18[4]; };
extern void *store_143ED0528;
extern bool store_143E039FF;
extern int store_143ED03D4;
extern void *store_141EF8518;
void sub_14034E880();
int sub_14082AEF0(void *queue, StoreFrameTask *task);
void sub_14034E990();
extern bool store_143ED0578;
extern void *store_143ED0570;
extern bool store_143E33FED;
void *dwGetTwitchService(int controller);
void *bdLobbyService_getMessaging(void *service);
void sub_1405B7C10(int controller, void *arg);
bool sub_1407893BC(void *context, int arg1, int arg2);
void sub_1403A2FF0(LocalClientNum_t localClientNum);
void sub_1403CE3A0();
int sub_1403D8DA0(int unused, int scaledMsec);  // returns scaledMsec; the caller leaves its first argument unset
void sub_1403A3D80();
extern int store_142121190;        // the time Phys_RunToTime is given on a dedicated server
void sub_1405B83D0(int arg);
void sub_1405B5DB0(ControllerIndex_t controllerIndex, int msec);
void sub_140473AB0();
void sub_1404797C0();
void sub_1401AE430(LocalClientNum_t localClientNum, int msec);
int sub_140430860();
void sub_1404071C0(LocalClientNum_t localClientNum, int value);
void sub_1400C7720(int msec);
void sub_140498A50(ControllerIndex_t controllerIndex);
void sub_1403B6FA0(int arg, int msec, const char *name);

// The frame's frame-rate cap: com_maxfps, lowered to the server's sv_clientFpsLimit when that is lower; 0 is none.
// (This and the two pieces below are Com_Frame_Try_Block_Function's own code, split out so the enhanced build can
// replace only the wait: enhanced/qcommon/common.cpp. They are not functions of the original.)
int Com_FrameMaxFps()
{
	int maxFps = Dvar_GetInt(com_maxfps);
	int clientFpsLimit = store_140E53B70->clientFpsLimit;
	if (clientFpsLimit > 0 && (maxFps == 0 || maxFps > clientFpsLimit))
		maxFps = clientFpsLimit;
	return maxFps;
}

// The frame wait: whole milliseconds, Sleep(1) between event loops, at most 50 times. Sets com_frameTime; returns the
// frame's msec (at least 1).
int Com_FrameWait(int minMsec)
{
	int now;
	int msec;
	for (int spin = 0; spin < 50; spin++)
	{
		Com_EventLoop();
		now = Sys_Milliseconds();
		if (now - com_frameTime < 0)
			com_frameTime = now;
		msec = now - com_frameTime;
		if (msec >= minMsec)
			break;
		Sleep(1);
	}
	com_frameTime = now;
	if (msec < 1)
		msec = 1;
	return msec;
}

// Everything after the wait.
void Com_FrameAfterWait(LocalClientNum_t localClientNum, int minMsec, int index, int msec)
{
	unsigned int maxMsec = !Com_IsDedicatedServer() && Dvar_GetBool(sv_running) ? 100 : 5000;
	if ((unsigned int)msec > maxMsec)
		msec = maxMsec;

	// the average of the last three frames
	float frameSec = (float)msec * 0.001f;
	if ((unsigned int)msec <= 40)
	{
		com_lastFrameTime[0] += msec;
		com_lastFrameTime[1] += msec;
		com_lastFrameTime[2] += msec;
		com_lastFrameTime_base[0] += frameSec;
		com_lastFrameTime_base[1] += frameSec;
		com_lastFrameTime_base[2] += frameSec;
		int lastFrameTime = com_lastFrameTime[index];
		if (lastFrameTime < minMsec)
			lastFrameTime = minMsec;
		com_lastFrameTime[index] =
			(com_lastFrameTime[1] + com_lastFrameTime[0] + com_lastFrameTime[2] - 2 * lastFrameTime) * 2 / 6;
		msec = (index + lastFrameTime) / 3;
		if (!msec)
			msec = 1;
		float minSec = (float)minMsec * 0.001f;
		float lastFrameTime_base = com_lastFrameTime_base[index];
		if (!(lastFrameTime_base - minSec >= 0.0f))
			lastFrameTime_base = minSec;
		float sum = com_lastFrameTime_base[1] + com_lastFrameTime_base[0];
		frameSec = lastFrameTime_base * 0.33333334f;
		sum = sum + com_lastFrameTime_base[2];
		com_lastFrameTime_base[index] = (sum + sum - lastFrameTime_base * 4.0f) * 0.16666667f;
	}
	else
	{
		com_lastFrameTime_base[0] += 0.040000003f;
		com_lastFrameTime[0] += 40;
		com_lastFrameTime_base[1] += 0.040000003f;
		com_lastFrameTime[1] += 40;
		com_lastFrameTime[2] += 40;
		com_lastFrameTime[index] = 0;
		com_lastFrameTime_base[2] += 0.040000003f;
		com_lastFrameTime_base[index] = 0.0f;
	}

	TaskManager2_ProcessTasks(CONTROLLER_INDEX_0);
	DB_LoadLoadFastfilesForNewContent();
	if (store_14413FEED)
	{
		int start = Sys_Milliseconds();
		InterlockedIncrement(&store_14413FEE0);
		while (store_14413FEE4)
			Sleep(0);
		for (int i = 0; i < store_14413FEE8; i++)
		{
			StoreQueuedCall *call = &store_1441AFEF0[i];
			if (call->pending)
			{
				call->func(call->arg);
				call->pending = false;
			}
			if (Sys_Milliseconds() - start >= 10)
				break;
		}
		InterlockedDecrement(&store_14413FEE0);
	}
	Cbuf_Execute(localClientNum, Com_LocalClient_GetControllerIndex(localClientNum));
	if (store_143ED0528 && !store_143E039FF && Sys_Milliseconds() - store_143ED03D4 >= 30000)
	{
		store_143E039FF = true;
		StoreFrameTask *task = (StoreFrameTask *)operator new(sizeof(StoreFrameTask));
		task->unknown8 = 0;
		task->unknown18[0] = 0;
		task->unknown18[1] = 0;
		task->unknown18[2] = 0;
		task->unknown18[3] = 0;
		task->context = store_141EF8518;
		task->func = sub_14034E880;
		if (sub_14082AEF0(store_143ED0528, task) < 0)
		{
			operator delete(task, sizeof(StoreFrameTask));
			sub_14034E990();
		}
	}
	if (store_143ED0578)
	{
		void *service = dwGetTwitchService(0);
		if (service && bdLobbyService_getMessaging(service) && store_143E33FED)
		{
			sub_1405B7C10(0, store_143ED0570);
			store_143ED0578 = false;
		}
	}
	while (sub_1407893BC(store_141EF8518, 1, 0))
		;

	if (Demo_IsPlaying() && msec > 200)
		msec = 200;
	int scaledMsec = Com_TimeScaleMsec(msec);
	Demo_Frame(msec, scaledMsec);
	sub_1403CE3A0();
	msec = sub_1403D8DA0(0, scaledMsec);
	sub_1403A3D80();
	if (Dvar_GetInt(com_dedicated))
	{
		if (Dvar_GetInt(com_dedicated))
			Phys_RunToTime(store_142121190);
		return;
	}

	// the client
	store_1427F1B58 = com_lastFrameTime[index];
	ControllerIndex_t controllerIndex = Com_LocalClient_GetControllerIndex(localClientNum);
	if (store_143C0A758)
	{
		sub_1405B83D0(0);
		sub_1405B5DB0(controllerIndex, msec);
	}
	else
	{
		store_143C0A758 = 1;
	}
	if (!cls.inputUpdatedPrevFrame)
		IN_Frame();
	cls.realtime += msec;
	int sinceLast = com_frameTime - store_140E5CB94;
	cls.frametime_base = frameSec;
	cls.inputUpdatedPrevFrame = false;
	cls.realFrametime = msec;
	cls.frametime = msec;
	if (!sinceLast)
		store_140E5C804 = 1;
	else
		store_140E5C804 = (unsigned int)sinceLast > 200 ? 200 : sinceLast;
	store_140E5CB94 = com_frameTime;
	Com_EventLoop();
	Cbuf_Execute(LOCAL_CLIENT_0, Com_LocalClient_GetControllerIndex(LOCAL_CLIENT_0));
	sub_140473AB0();
	sub_1404797C0();
	sub_1401AE430(LOCAL_CLIENT_0, msec);
	store_142161684 &= ~2u;
	LocalClientNum_t primary = Com_LocalClients_GetPrimary();
	if (!(store_140E61798 & ~2))
	{
		int value = sub_140430860();
		if (value < 0)
			value = 1;
		if (Sys_TryEnterCriticalSection(CRITSECT_AUDIO_PHYSICS))
		{
			ControllerIndex_t controller = Com_LocalClient_GetControllerIndex(primary);
			bool *done = store_140A5AAAB ? &store_1443C56DD : &store_1443C561D[controller][0];
			if (!*done)
			{
				sub_1404071C0(primary, value);
				if (value == 16)
					store_143ED9A18[controller] |= 2;
				*done = true;
			}
			Sys_LeaveCriticalSection(CRITSECT_AUDIO_PHYSICS);
		}
	}
	sub_1400C7720(msec);
	SCR_UpdateScreen();
	int paused = Dvar_GetInt(cl_paused);
	ControllerIndex_t controller0 = Com_LocalClient_GetControllerIndex(LOCAL_CLIENT_0);
	if (controller0 != INVALID_CONTROLLER_PORT)
	{
		if (store_140E61798 == 10 && !paused)
			sub_140498A50(controller0);
		else
			GPad_StopRumbles(controller0);
	}
	if (Dvar_GetBool(com_statmon))
	{
		if (store_142155E08)
		{
			sub_1403B6FA0(1, 3000, "code_warning_file");
			store_142155E08 = 0;
		}
		int previous = store_1444BA284;
		store_1444BA284 = Sys_Milliseconds();
		if (store_1444BA284 - previous > 33 && previous)
			sub_1403B6FA0(0, 3000, "code_warning_fps");
	}
	while (Sys_Milliseconds() - store_1427F1B58 < 0)
		Sleep(1);
}

// 1403A4010 (Store; the server's is qcommon/common.cpp:6460)
// One frame. Waits (Com_EventLoop, then Sleep(1), up to 50 times) until 1000 / com_maxfps milliseconds have passed
// since the last frame (the server's sv_clientFpsLimit caps it too); the frame's msec is then averaged over the last
// three frames (up to 40 ms each) and clamped. Then the Store's queued calls and online tasks, the command buffer,
// time scaling, demo and local server frames, and the client: its input (IN_Frame), the second event loop, the
// command buffer again, the client frame and the screen (SCR_UpdateScreen, where the commands are built). A dedicated
// server stops after the server frame.
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
	int msec = Com_FrameWait(minMsec);
	Com_FrameAfterWait(localClientNum, minMsec, index, msec);
}
#endif
