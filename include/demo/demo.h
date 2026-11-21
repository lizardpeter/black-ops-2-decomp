// demo: demo recording and playback (demo_common.cpp and the files around it).
#pragma once

enum demoState
{
	DEMO_STATE_IDLE = 0,
	DEMO_STATE_RECORDING = 1,
	DEMO_STATE_PLAYING = 2,
	DEMO_STATE_COUNT = 3,
};

// The demo system's state (0xB0700 bytes in the server's PDB); only the fields reconstructed code reads.
struct demoMain
{
	char demoName[128];
	int demoFileHandle;
	demoState state;
};

extern demoMain demo;  // demo_common.cpp

bool Demo_IsPlaying();
