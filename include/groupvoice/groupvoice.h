// groupvoice/: the PC voice chat back end (record.cpp, play.cpp, decode.cpp, encode.cpp; DirectSound and Speex).
#pragma once

struct dsound_sample_t;

// record.cpp
int Record_Init(bool, const void *handle);
int Record_Stop(dsound_sample_t *sample);
int Record_DestroySample(dsound_sample_t *sample);
void Record_Shutdown();

// play.cpp
int Sound_Init(const void *handle);
void Sound_Shutdown();
dsound_sample_t *Sound_NewSample();
int Sound_DestroySample(dsound_sample_t *sample);
bool Sound_StopSample(dsound_sample_t *sample);
void Sound_SampleFrame(dsound_sample_t *sample);
void Sound_SetVolume(dsound_sample_t *sample, int volume);
int Sound_UpdateSample(dsound_sample_t *sample, char *data, unsigned int size);
void Sound_Frame();

// decode.cpp
extern int g_decode_frame_size;
bool Decode_Init(int bandwidth);
void Decode_Shutdown();
int Decode_Sample(char *data, int dataSize, short *out, int frameSize);

// encode.cpp
extern int g_current_bandwidth_setting;
bool Encode_Init(int bandwidth);
bool Encode_Shutdown();
