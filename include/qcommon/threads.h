// qcommon/threads.cpp
#pragma once

void *Sys_GetValue(int valueIndex);
void Sys_SetValue(int valueIndex, void *data);
bool Sys_IsMainThread();
bool Sys_IsServerThread();
bool Sys_IsRenderThread();
