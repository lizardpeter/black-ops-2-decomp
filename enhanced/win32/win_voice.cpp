// win32/win_voice.cpp, enhanced (P0001, B0002): voice chat leaves the Windows recording devices alone.
//
// The original sets every recording device's level to winvoice_mic_reclevel (100% by default) and its mute state to
// winvoice_mic_mute when voice starts, re-applies the level whenever the dvar changes, and restores both only on a
// clean Voice_Shutdown: after a crash the microphone stays at 100%. Here nothing touches the devices:
// - winvoice_mic_mute mutes the game's own capture: the local player doesn't count as talking while it's set.
// - winvoice_mic_reclevel no longer reaches the device; the game's capture gain is winvoice_mic_scaler. Mapping the
//   level onto that gain waits for the capture path (groupvoice/record.cpp) to be reconstructed.
// Everything else is the faithful code.
// The Store build inlines the mixer calls into Voice_Init, Voice_Shutdown and the level re-apply (1401C2950), all of
// which only touch the lines Mixer_InitWaveInLineInfo saved; there the patch is that function finding no line, so no
// device is ever changed (below). The servers' version replaces the callers instead (it was written first).
#ifdef T6_STORE
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\win32\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "win_voice.cpp"
#define T6_SOURCE "win32/win_voice.cpp"

#include "win32/win_voice.h"

#include <string.h>

#include "t6_static.h"

T6_STATIC WaveInLineInfo s_waveInLineInfo[8] T6_AT("", "s_waveInLineInfo");

// 14049F270: the original also finds every mixer's wave-in line and saves its level and mute, so that Voice_Init can
// set them (winvoice_mic_reclevel, 100% by default; winvoice_mic_mute) and Voice_Shutdown restore them. Here the
// table stays empty: the set and restore loops have nothing to do, whatever the dvars say.
void Mixer_InitWaveInLineInfo()
{
	memset(s_waveInLineInfo, 0, sizeof(s_waveInLineInfo));
}
#else
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\win32\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "win_voice.cpp"
#define T6_SOURCE "win32/win_voice.cpp"

#include "win32/win_voice.h"

#include <string.h>
#include <windows.h>

#include "client/client.h"
#include "groupvoice/groupvoice.h"
#include "t6_static.h"
#include "universal/dvar.h"

enum
{
	MAX_CLIENTS = 18,
};

T6_STATIC dsound_sample_t *s_clientSamples[MAX_CLIENTS] T6_AT("", "s_clientSamples");
T6_STATIC int s_clientTalkTime[MAX_CLIENTS] T6_AT("", "s_clientTalkTime");
T6_STATIC bool recording T6_AT("", "recording");
T6_STATIC dsound_sample_t *currentRecordingSample T6_AT("", "currentRecordingSample");

bool Voice_Init()
{
	winvoice_mic_mute = _Dvar_RegisterBool("winvoice_mic_mute", false, 0, "Mute the microphone");
	winvoice_mic_reclevel = _Dvar_RegisterInt("winvoice_mic_reclevel", 0xFFFF, 0, 0xFFFF, 0, "Microphone recording level");
	winvoice_save_voice = _Dvar_RegisterBool("winvoice_save_voice", false, 0, "Write voice data to a file");
	winvoice_mic_scaler = _Dvar_RegisterFloat("winvoice_mic_scaler", 1.0f, 0.25f, 2.0f, 0, "Microphone scaler value");
	winvoice_loopback = _Dvar_RegisterBool("winvoice_loopback", false, 0, "Echo microphone input locally");
	winvoice_voice_level = _Dvar_RegisterInt("winvoice_voice_level", 0xFF, 0, 0xFF, 0, "Voice scaler value");

	g_current_bandwidth_setting = 0;
	HWND handle = GetDesktopWindow();
	if (Record_Init(false, handle))
		g_voice_record_initialized = 1;
	Decode_Init(g_current_bandwidth_setting);
	Encode_Init(g_current_bandwidth_setting);
	memset(s_clientTalkTime, 0, sizeof(s_clientTalkTime));
	if (Sound_Init(handle))
	{
		g_voice_initialized = 1;
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			s_clientSamples[i] = Sound_NewSample();
			Sound_SetVolume(s_clientSamples[i], Dvar_GetInt(winvoice_voice_level));
		}
	}
	return true;
}

void Voice_Shutdown()
{
	if (!g_voice_initialized)
		return;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (s_clientSamples[i])
			Sound_DestroySample(s_clientSamples[i]);
	}
	if (recording)
	{
		Record_Stop(currentRecordingSample);
		Record_DestroySample(currentRecordingSample);
		recording = false;
	}
	Record_Shutdown();
	Encode_Shutdown();
	Decode_Shutdown();
	Sound_Shutdown();
	g_voice_initialized = 0;
	g_voice_record_initialized = 0;
}

bool Voice_IsLocalClientTalking(SessionData *session, LocalClientNum_t localClientNum)
{
	return g_voice_record_initialized && g_voice_initialized && !Dvar_GetBool(winvoice_mic_mute) && IN_IsTalkKeyHeld();
}

void Voice_Playback()
{
	if (!g_voice_initialized)
		return;
	for (int i = 0; i < MAX_CLIENTS; ++i)
		Sound_SampleFrame(s_clientSamples[i]);
	Sound_Frame();
}
#endif
