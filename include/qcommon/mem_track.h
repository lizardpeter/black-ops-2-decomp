// qcommon/mem_track.cpp
#pragma once

enum TrackType
{
	TRACK_MISC = 12,
};

extern "C" void track_static_alloc_internal(void *ptr, int size, const char *name, int type);
