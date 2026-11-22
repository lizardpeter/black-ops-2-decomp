// qcommon/threads.cpp
#pragma once

void *Sys_GetValue(int valueIndex);
void Sys_SetValue(int valueIndex, void *data);
bool Sys_IsMainThread();
bool Sys_IsServerThread();
bool Sys_IsRenderThread();
void Sys_SetWin32QuitEvent();
void Sys_RendererRunning();      // Store only
bool Sys_RendererWorkPending();  // Store only
void *Sys_TakeRendererData();    // Store only
