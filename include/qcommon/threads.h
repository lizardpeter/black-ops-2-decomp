// qcommon/threads.cpp
#pragma once

// The engine's threads (the PDB's variant with THREAD_CONTEXT_TITLE_SERVER, as threads.obj has; THREAD_CONTEXT_COUNT,
// 17 in it, is in universal/q_shared.h)
enum ThreadContext
{
	THREAD_CONTEXT_MAIN = 0x0,
	THREAD_CONTEXT_BACKEND = 0x1,
	THREAD_CONTEXT_WORKER0 = 0x2,
	THREAD_CONTEXT_SERVER = 0xA,
};

void *Sys_GetValue(int valueIndex);
void Sys_SetValue(int valueIndex, void *data);
bool Sys_IsMainThread();
bool Sys_IsServerThread();
bool Sys_IsRenderThread();
void Sys_SetWin32QuitEvent();
int Sys_WaitRenderer();          // Store only
void Sys_WakeRenderer(void *data);  // Store only (the server's isn't rebuilt yet)
void Sys_RendererRunning();      // Store only
bool Sys_RendererWorkPending();  // Store only
void *Sys_TakeRendererData();    // Store only
