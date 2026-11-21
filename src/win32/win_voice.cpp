// win32/win_voice.cpp (CoDMPServer_PC: win_voice.obj)
//
// PC voice chat: playback of other players' voices through groupvoice/, and the Windows recording-device settings
// the game takes over while voice is up (B0002: every recording device's level and mute state, restored only by
// Voice_Shutdown).
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\win32\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "win_voice.cpp"
#define T6_SOURCE "win32/win_voice.cpp"

#include "win32/win_voice.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "client/client.h"
#include "groupvoice/groupvoice.h"
#include "live/live.h"
#include "t6_assert.h"
#include "t6_static.h"
#include "universal/dvar.h"
#include "win32/win_shared.h"

enum
{
	MAX_CLIENTS = 18,
	MAX_WAVEIN_LINES = 8,
	VOICE_TALK_TIME_MS = 300,  // a client counts as talking this long after its last voice packet
};

T6_GLOBAL const dvar_t *winvoice_mic_mute;
T6_GLOBAL const dvar_t *winvoice_mic_reclevel;
T6_GLOBAL const dvar_t *winvoice_save_voice;
T6_GLOBAL const dvar_t *winvoice_mic_scaler;
T6_GLOBAL const dvar_t *winvoice_loopback;
T6_GLOBAL const dvar_t *winvoice_voice_level;
T6_GLOBAL int g_voice_initialized;
T6_GLOBAL int g_voice_record_initialized;

T6_STATIC dsound_sample_t *s_clientSamples[MAX_CLIENTS] T6_AT("", "s_clientSamples");
T6_STATIC int s_clientTalkTime[MAX_CLIENTS] T6_AT("", "s_clientTalkTime");
T6_STATIC WaveInLineInfo s_waveInLineInfo[MAX_WAVEIN_LINES] T6_AT("", "s_waveInLineInfo");
T6_STATIC int wavein_current_reclevel T6_AT("", "wavein_current_reclevel");
T6_STATIC bool recording T6_AT("", "recording");
T6_STATIC bool playing T6_AT("", "playing");
T6_STATIC dsound_sample_t *currentRecordingSample T6_AT("", "currentRecordingSample");

// 0079EBD0  win32/win_voice.cpp:108
bool Voice_Init()
{
	winvoice_mic_mute = _Dvar_RegisterBool("winvoice_mic_mute", false, 0, "Mute the microphone");
	winvoice_mic_reclevel = _Dvar_RegisterInt("winvoice_mic_reclevel", 0xFFFF, 0, 0xFFFF, 0, "Microphone recording level");
	winvoice_save_voice = _Dvar_RegisterBool("winvoice_save_voice", false, 0, "Write voice data to a file");
	winvoice_mic_scaler = _Dvar_RegisterFloat("winvoice_mic_scaler", 1.0f, 0.25f, 2.0f, 0, "Microphone scaler value");
	winvoice_loopback = _Dvar_RegisterBool("winvoice_loopback", false, 0, "Echo microphone input locally");
	winvoice_voice_level = _Dvar_RegisterInt("winvoice_voice_level", 0xFF, 0, 0xFF, 0, "Voice scaler value");

	// B0002: takes over every recording device (full level by default), saving the old settings
	wavein_current_reclevel = Dvar_GetInt(winvoice_mic_reclevel);
	Mixer_InitWaveInLineInfo();
	Mixer_SetWaveInRecordLevels(wavein_current_reclevel);
	Mixer_SetWaveInMuteStates(Dvar_GetBool(winvoice_mic_mute));

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

// 0079ED80  win32/win_voice.cpp:153
void Voice_Shutdown()
{
	if (!g_voice_initialized)
		return;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (s_clientSamples[i])
			Sound_DestroySample(s_clientSamples[i]);
	}
	Mixer_RestoreWaveInRecordLevels();
	Mixer_RestoreWaveInMuteStates();
	memset(s_waveInLineInfo, 0, sizeof(s_waveInLineInfo));
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

// 0079DF60  win32/win_voice.cpp:181
void Voice_StopClientSamples()
{
	if (!g_voice_initialized)
		return;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (s_clientSamples[i])
			Sound_StopSample(s_clientSamples[i]);
	}
}

// 0055DF60  win32/win_voice.cpp (folded with every other `return 0` function: the dedicated server sends no voice
// of its own)
int Voice_GetLocalVoiceData(SessionData *session, LocalClientNum_t localClientNum, ClientVoicePacket_s *packet)
{
	return 0;
}

// 0079E400  win32/win_voice.cpp:232
// Push to talk.
bool Voice_IsLocalClientTalking(SessionData *session, LocalClientNum_t localClientNum)
{
	return g_voice_record_initialized && g_voice_initialized && IN_IsTalkKeyHeld();
}

// 0079E430  win32/win_voice.cpp:237
bool Voice_IsHeadsetPresent(SessionData *session, ControllerIndex_t localControllerIndex)
{
	return g_voice_initialized != 0;
}

// 0079EE20  win32/win_voice.cpp:242
void Voice_Playback()
{
	if (!g_voice_initialized)
		return;
	int newLevel = Dvar_GetInt(winvoice_mic_reclevel);
	if (wavein_current_reclevel != newLevel)
	{
		wavein_current_reclevel = newLevel;
		Mixer_SetWaveInRecordLevels(newLevel);  // B0002
	}
	for (int i = 0; i < MAX_CLIENTS; ++i)
		Sound_SampleFrame(s_clientSamples[i]);
	Sound_Frame();
}

// 0079DF90  win32/win_voice.cpp:289
// Decodes a talker's voice packet frame by frame into their sample. talker indexes s_clientSamples before the range
// assert (B0010). winvoice_save_voice appends only the last decoded frame to voice.wav.
int Voice_IncomingVoiceData_Platform(SessionData *session, ClientNum_t talker, unsigned char *data, int packetDataSize)
{
	short decodedData[4096];

	if (!g_voice_initialized || !s_clientSamples[talker])
		return 0;
	if (!playing)
		playing = true;
	int size = 0;
	int offset = 0;
	assertf_at(308, (talker >= 0 && talker < 18), "(talker) = %i", talker);
	s_clientTalkTime[talker] = Sys_Milliseconds();
	while (offset < packetDataSize)
	{
		int frameSize = g_decode_frame_size;
		if (packetDataSize - offset < g_decode_frame_size)
			frameSize = packetDataSize - offset;
		size = Decode_Sample((char *)&data[offset], frameSize, decodedData, g_decode_frame_size);
		if (size > 0)
			Sound_UpdateSample(s_clientSamples[talker], (char *)decodedData, size);
		offset += frameSize;
	}
	if (Dvar_GetBool(winvoice_save_voice))
	{
		FILE *file = fopen("voice.wav", "ab");
		if (file)
		{
			fwrite(decodedData, size, 1, file);
			fclose(file);
		}
	}
	return packetDataSize;
}

// 0079E440  win32/win_voice.cpp:338
bool Voice_IsXuidTalking(SessionData *session, ControllerIndex_t localControllerIndex, unsigned __int64 xuid)
{
	if (!Live_Base_IsConnected(localControllerIndex))
		return false;
	if (Live_XUIDIsLocalPlayer(xuid))
		return Voice_IsLocalClientTalking(session, Com_ControllerIndex_GetLocalClientNum(localControllerIndex));
	ClientNum_t clientNum = Session_FindRegisteredUser(session, xuid);
	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
		return Voice_IsClientTalking(clientNum);
	return false;
}

// 0079E0F0  win32/win_voice.cpp:358
bool Voice_IsClientTalking(ClientNum_t clientNum)
{
	assertf_at(359, (clientNum >= 0 && clientNum < 18), "(clientNum) = %i", clientNum);
	return Sys_Milliseconds() - s_clientTalkTime[clientNum] < VOICE_TALK_TIME_MS;
}

// 0079E4D0  win32/win_voice.cpp:402
// Finds each mixer's wave-in destination line and saves its level and mute state. No bound on the table: a ninth
// such mixer writes past s_waveInLineInfo (B0011).
void Mixer_InitWaveInLineInfo()
{
	memset(s_waveInLineInfo, 0, sizeof(s_waveInLineInfo));
	UINT mixerDevCount = mixerGetNumDevs();
	if (!mixerDevCount || !waveInGetNumDevs())
		return;
	WaveInLineInfo *info = s_waveInLineInfo;
	for (UINT uMxId = 0; uMxId < mixerDevCount; ++uMxId)
	{
		MIXERCAPSA mixcaps;
		HMIXER phmx;
		if (mixerGetDevCapsA(uMxId, &mixcaps, sizeof(mixcaps)) != MMSYSERR_NOERROR)
			continue;
		if (mixerOpen(&phmx, uMxId, 0, 0, 0) != MMSYSERR_NOERROR)
			continue;
		MIXERLINEA mixerline;
		memset(&mixerline, 0, sizeof(mixerline));
		mixerline.cbStruct = sizeof(mixerline);
		mixerline.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
		if (mixerGetLineInfoA((HMIXEROBJ)phmx, &mixerline, MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
		{
			info->mixerID = uMxId;
			info->dstWaveInLineID = mixerline.dwLineID;
			info->oldRecLevel = Mixer_GetRecordLevel(uMxId, mixerline.dwLineID);
			info->oldMuteState = Mixer_GetMuteState(uMxId, mixerline.dwLineID);
			info->isValid = 1;
			++info;
		}
		mixerClose(phmx);
	}
}

// Sets one control (volume or mute) of a saved line. Inlined into the four Mixer_*WaveIn* functions below; not in
// the symbols, so the name is descriptive.
static void Mixer_SetLineControl(unsigned int mixerID, unsigned int lineID, DWORD controlType, int value)
{
	HMIXER phmx;
	MIXERLINEA mixerline;
	MIXERLINECONTROLSA mxlc;
	MIXERCONTROLA mxc;
	MIXERCONTROLDETAILS mxcd;
	int newSetting;

	if (mixerOpen(&phmx, mixerID, 0, 0, 0) != MMSYSERR_NOERROR)
		return;
	mixerline.cbStruct = sizeof(mixerline);
	mixerline.dwLineID = lineID;
	if (mixerGetLineInfoA((HMIXEROBJ)phmx, &mixerline, MIXER_GETLINEINFOF_LINEID) == MMSYSERR_NOERROR)
	{
		mxlc.cbStruct = sizeof(mxlc);
		mxlc.dwLineID = mixerline.dwLineID;
		mxlc.dwControlType = controlType;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(mxc);
		mxlc.pamxctrl = &mxc;
		if (mixerGetLineControlsA((HMIXEROBJ)phmx, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
		{
			mxcd.cbStruct = sizeof(mxcd);
			mxcd.dwControlID = mxc.dwControlID;
			mxcd.cChannels = 1;
			mxcd.cMultipleItems = 0;
			mxcd.cbDetails = sizeof(newSetting);
			mxcd.paDetails = &newSetting;
			if (mixerGetControlDetailsA((HMIXEROBJ)phmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
			{
				newSetting = value;
				mixerSetControlDetails((HMIXEROBJ)phmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);
			}
		}
	}
	mixerClose(phmx);
}

// 0079E610  win32/win_voice.cpp:454
// B0002: sets the recording level of every saved line.
void Mixer_SetWaveInRecordLevels(int newLevel)
{
	for (int i = 0; i < MAX_WAVEIN_LINES; ++i)
	{
		if (s_waveInLineInfo[i].isValid)
			Mixer_SetLineControl(s_waveInLineInfo[i].mixerID, s_waveInLineInfo[i].dstWaveInLineID,
			                     MIXERCONTROL_CONTROLTYPE_VOLUME, newLevel);
	}
}

// 0079E780  win32/win_voice.cpp:468
void Mixer_RestoreWaveInRecordLevels()
{
	for (int i = 0; i < MAX_WAVEIN_LINES; ++i)
	{
		if (s_waveInLineInfo[i].isValid)
			Mixer_SetLineControl(s_waveInLineInfo[i].mixerID, s_waveInLineInfo[i].dstWaveInLineID,
			                     MIXERCONTROL_CONTROLTYPE_VOLUME, s_waveInLineInfo[i].oldRecLevel);
	}
}

// Reads one control (volume or mute) of a line, or `failed`. Inlined into Mixer_GetRecordLevel and Mixer_GetMuteState;
// a descriptive name, as above.
static int Mixer_GetLineControl(unsigned long mixerID, unsigned long lineID, DWORD controlType, int failed)
{
	HMIXER phmx;
	MIXERLINEA mixerline;
	MIXERLINECONTROLSA mxlc;
	MIXERCONTROLA mxc;
	MIXERCONTROLDETAILS mxcd;
	MIXERCONTROLDETAILS_UNSIGNED newSetting;

	if (mixerOpen(&phmx, mixerID, 0, 0, 0) != MMSYSERR_NOERROR)
		return failed;
	mixerline.cbStruct = sizeof(mixerline);
	mixerline.dwLineID = lineID;
	if (mixerGetLineInfoA((HMIXEROBJ)phmx, &mixerline, MIXER_GETLINEINFOF_LINEID) == MMSYSERR_NOERROR)
	{
		mxlc.cbStruct = sizeof(mxlc);
		mxlc.dwLineID = mixerline.dwLineID;
		mxlc.dwControlType = controlType;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(mxc);
		mxlc.pamxctrl = &mxc;
		if (mixerGetLineControlsA((HMIXEROBJ)phmx, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
		{
			mxcd.cbStruct = sizeof(mxcd);
			mxcd.dwControlID = mxc.dwControlID;
			mxcd.cChannels = 1;
			mxcd.hwndOwner = 0;
			mxcd.cbDetails = sizeof(newSetting);
			mxcd.paDetails = &newSetting;
			if (mixerGetControlDetailsA((HMIXEROBJ)phmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
			{
				mixerClose(phmx);
				return newSetting.dwValue;
			}
		}
	}
	mixerClose(phmx);
	return failed;
}

// 0079E140  win32/win_voice.cpp:482
int Mixer_GetRecordLevel(unsigned long mixerID, unsigned long waveInLineID)
{
	return Mixer_GetLineControl(mixerID, waveInLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, -1);
}

// 0079E8F0  win32/win_voice.cpp:568
// B0002: sets the mute state of every saved line.
void Mixer_SetWaveInMuteStates(int muteState)
{
	for (int i = 0; i < MAX_WAVEIN_LINES; ++i)
	{
		if (s_waveInLineInfo[i].isValid)
			Mixer_SetLineControl(s_waveInLineInfo[i].mixerID, s_waveInLineInfo[i].dstWaveInLineID,
			                     MIXERCONTROL_CONTROLTYPE_MUTE, muteState);
	}
}

// 0079EA60  win32/win_voice.cpp:582
void Mixer_RestoreWaveInMuteStates()
{
	for (int i = 0; i < MAX_WAVEIN_LINES; ++i)
	{
		if (s_waveInLineInfo[i].isValid)
			Mixer_SetLineControl(s_waveInLineInfo[i].mixerID, s_waveInLineInfo[i].dstWaveInLineID,
			                     MIXERCONTROL_CONTROLTYPE_MUTE, s_waveInLineInfo[i].oldMuteState);
	}
}

// 0079E2A0  win32/win_voice.cpp:637
int Mixer_GetMuteState(unsigned long mixerID, unsigned long waveInLineID)
{
	return Mixer_GetLineControl(mixerID, waveInLineID, MIXERCONTROL_CONTROLTYPE_MUTE, 0);
}
