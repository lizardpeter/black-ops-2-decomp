// win32/win_voice.cpp: PC voice chat on top of groupvoice/, and the Windows mixer settings it changes.
#pragma once

#include "qcommon/qcommon.h"

struct SessionData;
struct ClientVoicePacket_s;
struct dvar_t;

struct WaveInLineInfo
{
	int isValid;
	unsigned int mixerID;
	unsigned int dstWaveInLineID;
	int oldRecLevel;
	int oldMuteState;
};

extern const dvar_t *winvoice_mic_mute;
extern const dvar_t *winvoice_mic_reclevel;
extern const dvar_t *winvoice_save_voice;
extern const dvar_t *winvoice_mic_scaler;
extern const dvar_t *winvoice_loopback;
extern const dvar_t *winvoice_voice_level;
extern int g_voice_initialized;
extern int g_voice_record_initialized;

bool Voice_Init();
void Voice_Shutdown();
void Voice_StopClientSamples();
int Voice_GetLocalVoiceData(SessionData *session, LocalClientNum_t localClientNum, ClientVoicePacket_s *packet);
bool Voice_IsLocalClientTalking(SessionData *session, LocalClientNum_t localClientNum);
bool Voice_IsHeadsetPresent(SessionData *session, ControllerIndex_t localControllerIndex);
void Voice_Playback();
int Voice_IncomingVoiceData_Platform(SessionData *session, ClientNum_t talker, unsigned char *data, int packetDataSize);
bool Voice_IsXuidTalking(SessionData *session, ControllerIndex_t localControllerIndex, unsigned __int64 xuid);
bool Voice_IsClientTalking(ClientNum_t clientNum);

void Mixer_InitWaveInLineInfo();
void Mixer_SetWaveInRecordLevels(int newLevel);
void Mixer_RestoreWaveInRecordLevels();
int Mixer_GetRecordLevel(unsigned long mixerID, unsigned long waveInLineID);
void Mixer_SetWaveInMuteStates(int muteState);
void Mixer_RestoreWaveInMuteStates();
int Mixer_GetMuteState(unsigned long mixerID, unsigned long waveInLineID);
