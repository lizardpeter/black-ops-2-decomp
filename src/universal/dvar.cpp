// universal/dvar.cpp (CoDMPServer_PC: dvar.obj)
//
// Console variables. So far the typed getters: each reads its own type and converts from the others (the server
// warns when it has to; the Store revision converts silently).
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\universal\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "dvar.cpp"
#define T6_SOURCE "universal/dvar.cpp"

#include "universal/dvar.h"

#include <stdlib.h>

#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "t6_compiler.h"
#include "t6_static.h"

T6_STATIC const char *s_dvarTypeNames[DVAR_TYPE_COUNT] T6_AT("", "s_dvarTypeNames");

// 007725F0  universal/dvar.cpp:473
bool Dvar_StringToBool(const char *string)
{
	assert_at(474, string);
	return atoi(string) != 0;
}

// 00772630  universal/dvar.cpp:480
int Dvar_StringToInt(const char *string)
{
	assert_at(481, string);
	return atoi(string);
}

// 00772690  universal/dvar.cpp:494
// atof's double comes back unrounded on the x86 server (ST0); the Store rounds it to float.
X87_MATH float Dvar_StringToFloat(const char *string)
{
	assert_at(495, string);
	return atof(string);
}

// 00773C10  universal/dvar.cpp:1209 (inlined in the Store build)
bool Dvar_HasLatchedValue(const dvar_t *dvar)
{
	return !Dvar_ValuesEqual(dvar->type, dvar->current, dvar->latched);
}

// 007767A0  universal/dvar.cpp:1568
bool Dvar_GetBool(const dvar_t *dvar)
{
	if (!dvar)
		return false;
	assertf_at(1576, (dvar->type == DVAR_TYPE_BOOL || (dvar->type == DVAR_TYPE_STRING && (dvar->flags & (1 << 14)))),
	           "(dvar->type) = %i", dvar->type);
	if (dvar->type == DVAR_TYPE_BOOL)
		return dvar->current.enabled;
#ifndef T6_STORE
	Com_PrintWarning(23, "Dvar '%s' silently casting to a different type (%s -> bool)\n", dvar->name,
	                 s_dvarTypeNames[dvar->type]);
#endif
	return Dvar_StringToBool(dvar->current.string);
}

// 00776850  universal/dvar.cpp:1588
int Dvar_GetInt(const dvar_t *dvar)
{
	if (!dvar)
		return 0;
	assertf_at(1594, (dvar->type == DVAR_TYPE_BOOL || dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_FLOAT || dvar->type == DVAR_TYPE_ENUM || (dvar->type == DVAR_TYPE_STRING && (dvar->flags & (1 << 14)))),
	           "(dvar->type) = %i", dvar->type);
	if (dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM)
		return dvar->current.integer;
#ifndef T6_STORE
	Com_PrintWarning(23, "Dvar '%s' silently casting to a different type (%s -> int)\n", dvar->name,
	                 s_dvarTypeNames[dvar->type]);
#endif
	if (dvar->type == DVAR_TYPE_BOOL)
		return dvar->current.enabled;
	if (dvar->type == DVAR_TYPE_FLOAT)
		return (int)dvar->current.value;
	return Dvar_StringToInt(dvar->current.string);
}

// 007769E0  universal/dvar.cpp:1674
X87_MATH float Dvar_GetFloat(const dvar_t *dvar)
{
	if (!dvar)
		return 0.0f;
	assertf_at(1680, (dvar->type == DVAR_TYPE_FLOAT || dvar->type == DVAR_TYPE_INT ||(dvar->type == DVAR_TYPE_STRING && (dvar->flags & (1 << 14)))),
	           "(dvar->type) = %i", dvar->type);
	if (dvar->type == DVAR_TYPE_FLOAT)
		return dvar->current.value;
#ifndef T6_STORE
	Com_PrintWarning(23, "Dvar '%s' silently casting to a different type (%s -> float)\n", dvar->name,
	                 s_dvarTypeNames[dvar->type]);
#endif
	if (dvar->type == DVAR_TYPE_INT)
		return dvar->current.integer;  // fild: unrounded on the x86 server
	return Dvar_StringToFloat(dvar->current.string);
}

// 00776A70  universal/dvar.cpp:2628 (inlined in the Store build)
void Dvar_MakeLatchedValueCurrent(const dvar_t *dvar)
{
	Dvar_SetVariant((dvar_t *)dvar, dvar->latched, DVAR_SOURCE_INTERNAL);
}

// 007776B0  universal/dvar.cpp:2971 (inlined in the Store build)
// A string dvar the UI set is given "1" or "0". The Store revision also does nothing for a null dvar (every inlined
// copy tests it, where the server only asserts).
void Dvar_SetBoolFromSource(const dvar_t *dvar, bool value, DvarSetSource source)
{
#ifdef T6_STORE
	if (!dvar)
		return;
#endif
	assert_at(2974, dvar);
	assert_at(2975, dvar->name);
	assertf_at(2976, (dvar->type == DVAR_TYPE_BOOL || (dvar->type == DVAR_TYPE_STRING && (dvar->flags & (1 << 14)))),
	           "(dvar->name) = %s", dvar->name);
	if (!dvar->name)
		return;
	DvarValue newValue;
	if (dvar->type == DVAR_TYPE_BOOL)
		newValue.enabled = value;
	else
		newValue.string = value ? "1" : "0";
	Dvar_SetVariant((dvar_t *)dvar, newValue, source);
}

// 00778330  universal/dvar.cpp:3223 (inlined in the Store build)
void Dvar_SetBool(const dvar_t *dvar, bool value)
{
	Dvar_SetBoolFromSource(dvar, value, DVAR_SOURCE_INTERNAL);
}

// 00778350  universal/dvar.cpp:3229 (inlined in the Store build)
void Dvar_SetInt(const dvar_t *dvar, int value)
{
	Dvar_SetIntFromSource(dvar, value, DVAR_SOURCE_INTERNAL);
}
