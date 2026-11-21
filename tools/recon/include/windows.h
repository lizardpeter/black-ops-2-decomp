// The part of <windows.h> the reconstruction uses.
//
// Build environment, not game source. Declarations match the Win32 SDK, so calls go through the import
// table as in the original (__imp__Name@N). In the hybrid build those import slots are the original
// binary's, and the differential harness points them at its window model (tests/differential/harness.py).
#pragma once

typedef int BOOL;
typedef unsigned int UINT;
typedef __UINTPTR_TYPE__ WPARAM;  // UINT_PTR
typedef __INTPTR_TYPE__ LPARAM;   // LONG_PTR
typedef long LONG;
typedef unsigned long DWORD;

struct HWND__ { int unused; };
typedef struct HWND__ *HWND;

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
WINUSERAPI BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

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
