// universal/dvar.cpp
#pragma once

#include "universal/q_shared.h"

enum dvarType_t
{
	DVAR_TYPE_INVALID = 0,
	DVAR_TYPE_BOOL = 1,
	DVAR_TYPE_FLOAT = 2,
	DVAR_TYPE_FLOAT_2 = 3,
	DVAR_TYPE_FLOAT_3 = 4,
	DVAR_TYPE_FLOAT_4 = 5,
	DVAR_TYPE_INT = 6,
	DVAR_TYPE_ENUM = 7,
	DVAR_TYPE_STRING = 8,
	DVAR_TYPE_COLOR = 9,
	DVAR_TYPE_INT64 = 10,
	DVAR_TYPE_LINEAR_COLOR_RGB = 11,
	DVAR_TYPE_COLOR_XYZ = 12,
	DVAR_TYPE_COUNT = 13,
};

union DvarValue
{
	bool enabled;
	int integer;
	unsigned int unsignedInt;
	__int64 integer64;
	unsigned __int64 unsignedInt64;
	float value;
	float vector[4];
	const char *string;
	unsigned char color[4];
};

union DvarLimits
{
	struct
	{
		int stringCount;
		const char **strings;
	} enumeration;
	struct
	{
		int min;
		int max;
	} integer;
	struct
	{
		__int64 min;
		__int64 max;
	} integer64;
	struct
	{
		float min;
		float max;
	} value;
	struct
	{
		float min;
		float max;
	} vector;
};

struct dvar_t
{
	const char *name;
	const char *description;
	int hash;
	unsigned int flags;
	dvarType_t type;
	bool modified;
	DvarValue current;
	DvarValue latched;
	DvarValue reset;
	DvarLimits domain;
	dvar_t *hashNext;
};
#ifdef T6_STORE
// t6mp_msstore: Dvar_GetInt 14046B950 reads type at 0x18 and the value at 0x20
static_assert(__builtin_offsetof(dvar_t, type) == 0x18 && __builtin_offsetof(dvar_t, current) == 0x20, "dvar_t");
#else
static_assert(sizeof(dvar_t) == 0x60 && __builtin_offsetof(dvar_t, current) == 0x18, "dvar_t");
#endif

enum
{
	DVAR_CHEAT = 1 << 7,      // changes only while sv_cheats is on, and goes back to its default when it's off
	DVAR_EXTERNAL = 1 << 14,  // a string dvar a mod or the UI set, read through the typed getters (descriptive name)
};

enum DvarSetSource
{
	DVAR_SOURCE_INTERNAL = 0,
	DVAR_SOURCE_EXTERNAL = 1,
	DVAR_SOURCE_SCRIPT = 2,
	DVAR_SOURCE_DEVGUI = 3,
};

// dvar.cpp
int Dvar_ValuesEqual(dvarType_t type, DvarValue val0, DvarValue val1);
void Dvar_SetVariant(dvar_t *dvar, DvarValue value, DvarSetSource source);
bool Dvar_HasLatchedValue(const dvar_t *dvar);
void Dvar_SetBoolFromSource(const dvar_t *dvar, bool value, DvarSetSource source);
void Dvar_SetIntFromSource(const dvar_t *dvar, int value, DvarSetSource source);
void Dvar_SetBool(const dvar_t *dvar, bool value);
void Dvar_SetInt(const dvar_t *dvar, int value);
void Dvar_MakeLatchedValueCurrent(const dvar_t *dvar);
bool Dvar_StringToBool(const char *string);
int Dvar_StringToInt(const char *string);
float Dvar_StringToFloat(const char *string);

// Registration: an existing dvar (from the config or the command line) is re-registered with the code's type,
// flags, default and domain; otherwise a new one is made.
dvar_t *Dvar_FindMalleableVar(const char *dvarName);
const dvar_t *Dvar_RegisterNew(const char *dvarName, dvarType_t type, unsigned int flags, DvarValue value,
                               DvarLimits domain, const char *description);
void Dvar_Reregister(dvar_t *dvar, const char *dvarName, dvarType_t type, unsigned int flags, DvarValue resetValue,
                     DvarLimits domain, const char *description);
const dvar_t *Dvar_RegisterVariant(const char *dvarName, dvarType_t type, unsigned int flags, DvarValue value,
                                   DvarLimits domain, const char *description);
int CanKeepStringPointer(const char *string);  // universal/q_shared.cpp

// The Dvar_Register* macros call these (the leading underscore is in the original names).
const dvar_t *_Dvar_RegisterBool(const char *dvarName, bool value, unsigned int flags, const char *description);
const dvar_t *_Dvar_RegisterInt(const char *dvarName, int value, int min, int max, unsigned int flags,
                                const char *description);
const dvar_t *_Dvar_RegisterFloat(const char *dvarName, float value, float min, float max, unsigned int flags,
                                  const char *description);
bool Dvar_GetBool(const dvar_t *dvar);
int Dvar_GetInt(const dvar_t *dvar);
float Dvar_GetFloat(const dvar_t *dvar);
