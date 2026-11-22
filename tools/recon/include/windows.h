// The part of <windows.h> the reconstruction uses.
//
// Build environment, not game source. Declarations match the Win32 SDK, so calls go through the import
// table as in the original (__imp__Name@N). In the hybrid build those import slots are the original
// binary's, and the differential harness points them at its window model (tests/differential/harness.py).
#pragma once

typedef int BOOL;
#define FALSE 0
#define TRUE 1
typedef unsigned int UINT;
typedef __UINTPTR_TYPE__ WPARAM;  // UINT_PTR
typedef __INTPTR_TYPE__ LPARAM;   // LONG_PTR
typedef long LONG;
typedef unsigned long DWORD;

struct HWND__ { int unused; };
typedef struct HWND__ *HWND;
struct HINSTANCE__ { int unused; };
typedef struct HINSTANCE__ *HINSTANCE;
struct HHOOK__ { int unused; };
typedef struct HHOOK__ *HHOOK;

typedef struct tagPOINT
{
	LONG x;
	LONG y;
} POINT, *LPPOINT;

typedef struct tagRECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *LPRECT;

#define WINAPI __stdcall
#define WINUSERAPI extern "C" __declspec(dllimport)

#define WM_LBUTTONDOWN 0x0201
#define MK_LBUTTON 0x0001

WINUSERAPI BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect);
WINUSERAPI BOOL WINAPI GetCursorPos(LPPOINT lpPoint);
WINUSERAPI BOOL WINAPI SetCursorPos(int X, int Y);
WINUSERAPI HWND WINAPI GetForegroundWindow(void);
WINUSERAPI BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint);
WINUSERAPI BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint);
WINUSERAPI int WINAPI ShowCursor(BOOL bShow);
WINUSERAPI BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow);
#define SW_MINIMIZE 6
#define PM_NOREMOVE 0x0000
WINUSERAPI BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// Threads, message windows and raw input, as the enhanced build's raw mouse thread uses them
// (enhanced/win32/win_rawinput.cpp). RegisterRawInputDevices and GetRawInputData aren't imported by the Store exe:
// they're real imports of the enhanced DLL (tools/recon/lib/user32.def).
typedef void *HANDLE;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef __INTPTR_TYPE__ LRESULT;  // LONG_PTR
typedef void *HRAWINPUT;
typedef LRESULT(WINAPI *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef DWORD(WINAPI *LPTHREAD_START_ROUTINE)(void *lpThreadParameter);

typedef struct tagWNDCLASSA
{
	UINT style;
	WNDPROC lpfnWndProc;
	int cbClsExtra;
	int cbWndExtra;
	HINSTANCE hInstance;
	HANDLE hIcon;
	HANDLE hCursor;
	HANDLE hbrBackground;
	const char *lpszMenuName;
	const char *lpszClassName;
} WNDCLASSA;

typedef struct tagMSG
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
	DWORD lPrivate;
} MSG;

typedef struct tagRAWINPUTDEVICE
{
	USHORT usUsagePage;
	USHORT usUsage;
	DWORD dwFlags;
	HWND hwndTarget;
} RAWINPUTDEVICE;

typedef struct tagRAWINPUTHEADER
{
	DWORD dwType;
	DWORD dwSize;
	HANDLE hDevice;
	WPARAM wParam;
} RAWINPUTHEADER;

typedef struct tagRAWMOUSE
{
	USHORT usFlags;
	union
	{
		ULONG ulButtons;
		struct
		{
			USHORT usButtonFlags;
			USHORT usButtonData;
		};
	};
	ULONG ulRawButtons;
	LONG lLastX;
	LONG lLastY;
	ULONG ulExtraInformation;
} RAWMOUSE;

typedef struct tagRAWKEYBOARD
{
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	USHORT VKey;
	UINT Message;
	ULONG ExtraInformation;
} RAWKEYBOARD;

typedef struct tagRAWINPUT
{
	RAWINPUTHEADER header;
	union
	{
		RAWMOUSE mouse;
		RAWKEYBOARD keyboard;
		unsigned char other[24];  // RAWHID: not used
	} data;
} RAWINPUT;

#define WM_INPUT 0x00FF
#define RID_INPUT 0x10000003
#define RIM_TYPEMOUSE 0
#define RIM_TYPEKEYBOARD 1
#define RI_KEY_BREAK 0x01
#define HID_USAGE_GENERIC_KEYBOARD ((USHORT)0x06)
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define GWLP_WNDPROC (-4)
#define ProcessPowerThrottling 4
#define PROCESS_POWER_THROTTLING_CURRENT_VERSION 1
#define PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION 0x4
typedef struct _PROCESS_POWER_THROTTLING_STATE
{
	ULONG Version;
	ULONG ControlMask;
	ULONG StateMask;
} PROCESS_POWER_THROTTLING_STATE;
#define RIDEV_INPUTSINK 0x00000100
#define MOUSE_MOVE_ABSOLUTE 0x01
#define HID_USAGE_PAGE_GENERIC ((USHORT)0x01)
#define HID_USAGE_GENERIC_MOUSE ((USHORT)0x02)
#define HWND_MESSAGE ((HWND)(__INTPTR_TYPE__)-3)
#define INFINITE 0xFFFFFFFF
#define THREAD_PRIORITY_HIGHEST 2
#define WINBASEAPI extern "C" __declspec(dllimport)

WINUSERAPI USHORT WINAPI RegisterClassA(const WNDCLASSA *lpWndClass);
WINUSERAPI HWND WINAPI CreateWindowExA(DWORD dwExStyle, const char *lpClassName, const char *lpWindowName,
                                       DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent,
                                       HANDLE hMenu, HINSTANCE hInstance, void *lpParam);
WINUSERAPI LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
WINUSERAPI BOOL WINAPI GetMessageA(MSG *lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
WINUSERAPI LRESULT WINAPI DispatchMessageA(const MSG *lpMsg);
WINUSERAPI BOOL WINAPI PeekMessageA(MSG *lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
WINUSERAPI BOOL WINAPI TranslateMessage(const MSG *lpMsg);

// Critical sections (universal/win_common.cpp): the SDK's layout (0x18 bytes on x86, 0x28 on x64)
typedef struct _RTL_CRITICAL_SECTION
{
	void *DebugInfo;
	LONG LockCount;
	LONG RecursionCount;
	HANDLE OwningThread;
	HANDLE LockSemaphore;
	__UINTPTR_TYPE__ SpinCount;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;
WINBASEAPI void WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
WINBASEAPI void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
WINUSERAPI BOOL WINAPI RegisterRawInputDevices(const RAWINPUTDEVICE *pRawInputDevices, UINT uiNumDevices,
                                               UINT cbSize);
WINUSERAPI UINT WINAPI GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, void *pData, UINT *pcbSize,
                                       UINT cbSizeHeader);
WINBASEAPI HINSTANCE WINAPI GetModuleHandleA(const char *lpModuleName);
WINBASEAPI HANDLE WINAPI CreateThread(void *lpThreadAttributes, __SIZE_TYPE__ dwStackSize,
                                      LPTHREAD_START_ROUTINE lpStartAddress, void *lpParameter,
                                      DWORD dwCreationFlags, DWORD *lpThreadId);
WINBASEAPI HANDLE WINAPI CreateEventA(void *lpEventAttributes, BOOL bManualReset, BOOL bInitialState,
                                      const char *lpName);
WINBASEAPI BOOL WINAPI SetEvent(HANDLE hEvent);
WINBASEAPI DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
WINBASEAPI BOOL WINAPI CloseHandle(HANDLE hObject);
WINBASEAPI BOOL WINAPI SetThreadPriority(HANDLE hThread, int nPriority);
// key timing (enhanced/win32/win_keytime.cpp): SetWindowLongPtrA, GetMessageTime and SetProcessInformation are real
// imports of the enhanced DLL (tools/recon/lib/user32.def, kernel32.def)
WINUSERAPI __INTPTR_TYPE__ WINAPI SetWindowLongPtrA(HWND hWnd, int nIndex, __INTPTR_TYPE__ dwNewLong);
WINUSERAPI LRESULT WINAPI CallWindowProcA(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
WINUSERAPI LONG WINAPI GetMessageTime(void);
WINBASEAPI DWORD WINAPI GetTickCount(void);
WINBASEAPI HANDLE WINAPI GetCurrentProcess(void);
WINBASEAPI BOOL WINAPI SetProcessInformation(HANDLE hProcess, int ProcessInformationClass,
                                             void *ProcessInformation, DWORD ProcessInformationSize);
extern "C" __declspec(dllimport) DWORD WINAPI timeGetTime(void);

// <mmeapi.h>: the audio mixer API (winmm.dll), as win32/win_voice.cpp uses it. The SDK packs these structures to
// one byte (pshpack1.h).
typedef UINT MMRESULT;
typedef __UINTPTR_TYPE__ UINT_PTR;
typedef __UINTPTR_TYPE__ DWORD_PTR;
typedef unsigned short WORD;
typedef char CHAR;
typedef long LONG;
typedef UINT MMVERSION;
struct HMIXER__ { int unused; };
typedef struct HMIXER__ *HMIXER, **LPHMIXER;
struct HMIXEROBJ__ { int unused; };
typedef struct HMIXEROBJ__ *HMIXEROBJ;

#define MMSYSERR_NOERROR 0
#define MAXPNAMELEN 32
#define MIXER_SHORT_NAME_CHARS 16
#define MIXER_LONG_NAME_CHARS 64
#define MIXERLINE_COMPONENTTYPE_DST_WAVEIN 7
#define MIXER_GETLINEINFOF_LINEID 0x00000002
#define MIXER_GETLINEINFOF_COMPONENTTYPE 0x00000003
#define MIXER_GETLINECONTROLSF_ONEBYTYPE 0x00000002
#define MIXER_GETCONTROLDETAILSF_VALUE 0x00000000
#define MIXER_SETCONTROLDETAILSF_VALUE 0x00000000
#define MIXERCONTROL_CONTROLTYPE_VOLUME 0x50030001
#define MIXERCONTROL_CONTROLTYPE_MUTE 0x20010002

#pragma pack(push, 1)
typedef struct tagMIXERCAPSA
{
	WORD wMid;
	WORD wPid;
	MMVERSION vDriverVersion;
	CHAR szPname[MAXPNAMELEN];
	DWORD fdwSupport;
	DWORD cDestinations;
} MIXERCAPSA, *LPMIXERCAPSA;

typedef struct tagMIXERLINEA
{
	DWORD cbStruct;
	DWORD dwDestination;
	DWORD dwSource;
	DWORD dwLineID;
	DWORD fdwLine;
	DWORD_PTR dwUser;
	DWORD dwComponentType;
	DWORD cChannels;
	DWORD cConnections;
	DWORD cControls;
	CHAR szShortName[MIXER_SHORT_NAME_CHARS];
	CHAR szName[MIXER_LONG_NAME_CHARS];
	struct
	{
		DWORD dwType;
		DWORD dwDeviceID;
		WORD wMid;
		WORD wPid;
		MMVERSION vDriverVersion;
		CHAR szPname[MAXPNAMELEN];
	} Target;
} MIXERLINEA, *LPMIXERLINEA;

typedef struct tagMIXERCONTROLA
{
	DWORD cbStruct;
	DWORD dwControlID;
	DWORD dwControlType;
	DWORD fdwControl;
	DWORD cMultipleItems;
	CHAR szShortName[MIXER_SHORT_NAME_CHARS];
	CHAR szName[MIXER_LONG_NAME_CHARS];
	union
	{
		struct
		{
			LONG lMinimum;
			LONG lMaximum;
		};
		struct
		{
			DWORD dwMinimum;
			DWORD dwMaximum;
		};
		DWORD dwReserved[6];
	} Bounds;
	union
	{
		DWORD cSteps;
		DWORD cbCustomData;
		DWORD dwReserved[6];
	} Metrics;
} MIXERCONTROLA, *LPMIXERCONTROLA;

typedef struct tagMIXERLINECONTROLSA
{
	DWORD cbStruct;
	DWORD dwLineID;
	union
	{
		DWORD dwControlID;
		DWORD dwControlType;
	};
	DWORD cControls;
	DWORD cbmxctrl;
	LPMIXERCONTROLA pamxctrl;
} MIXERLINECONTROLSA, *LPMIXERLINECONTROLSA;

typedef struct tMIXERCONTROLDETAILS
{
	DWORD cbStruct;
	DWORD dwControlID;
	DWORD cChannels;
	union
	{
		HWND hwndOwner;
		DWORD cMultipleItems;
	};
	DWORD cbDetails;
	void *paDetails;
} MIXERCONTROLDETAILS, *LPMIXERCONTROLDETAILS;

typedef struct tMIXERCONTROLDETAILS_UNSIGNED
{
	DWORD dwValue;
} MIXERCONTROLDETAILS_UNSIGNED;

typedef struct tMIXERCONTROLDETAILS_BOOLEAN
{
	LONG fValue;
} MIXERCONTROLDETAILS_BOOLEAN;
#pragma pack(pop)

#define WINMMAPI extern "C" __declspec(dllimport)

WINMMAPI UINT WINAPI mixerGetNumDevs(void);
WINMMAPI UINT WINAPI waveInGetNumDevs(void);
WINMMAPI MMRESULT WINAPI mixerGetDevCapsA(UINT_PTR uMxId, LPMIXERCAPSA pmxcaps, UINT cbmxcaps);
WINMMAPI MMRESULT WINAPI mixerOpen(LPHMIXER phmx, UINT uMxId, DWORD_PTR dwCallback, DWORD_PTR dwInstance,
                                   DWORD fdwOpen);
WINMMAPI MMRESULT WINAPI mixerClose(HMIXER hmx);
WINMMAPI MMRESULT WINAPI mixerGetLineInfoA(HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo);
WINMMAPI MMRESULT WINAPI mixerGetLineControlsA(HMIXEROBJ hmxobj, LPMIXERLINECONTROLSA pmxlc, DWORD fdwControls);
WINMMAPI MMRESULT WINAPI mixerGetControlDetailsA(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
WINMMAPI MMRESULT WINAPI mixerSetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);

WINUSERAPI HWND WINAPI GetDesktopWindow(void);

// Monitors (gfx_d3d/r_init.cpp: the native resolution)
struct HMONITOR__ { int unused; };
typedef struct HMONITOR__ *HMONITOR;
typedef struct tagMONITORINFO
{
	DWORD cbSize;
	RECT rcMonitor;
	RECT rcWork;
	DWORD dwFlags;
} MONITORINFO, *LPMONITORINFO;
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
WINUSERAPI int WINAPI GetSystemMetrics(int nIndex);
WINUSERAPI BOOL WINAPI GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi);

typedef struct _GUID
{
	unsigned long Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char Data4[8];
} GUID, IID;
