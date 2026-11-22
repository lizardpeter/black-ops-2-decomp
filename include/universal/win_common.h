// universal/win_common.h: the engine's critical sections.
#pragma once

enum CriticalSection
{
	CRITSECT_SYS_EVENT_QUEUE = 0x21,
	CRITSECT_DXCONTEXT = 0x25,
	CRITSECT_AUDIO_PHYSICS = 0x2B,  // the server's name for 0x2B; the Store's Com_Frame guards a per-controller flag with it
	CRITSECT_COUNT = 0x4E,
};

void Sys_EnterCriticalSection(CriticalSection critSect);
void Sys_LeaveCriticalSection(CriticalSection critSect);
bool Sys_TryEnterCriticalSection(CriticalSection critSect);
