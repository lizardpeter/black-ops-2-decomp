#pragma once
enum CriticalSection {CRITSECT_DXCONTEXT=0x25};
void Sys_EnterCriticalSection(CriticalSection);
void Sys_LeaveCriticalSection(CriticalSection);
