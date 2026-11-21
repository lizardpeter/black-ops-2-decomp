// universal/dvar.cpp
#pragma once

struct dvar_t;

// The Dvar_Register* macros call these (the leading underscore is in the original names).
const dvar_t *_Dvar_RegisterBool(const char *dvarName, bool value, unsigned int flags, const char *description);
const dvar_t *_Dvar_RegisterInt(const char *dvarName, int value, int min, int max, unsigned int flags,
                                const char *description);
const dvar_t *_Dvar_RegisterFloat(const char *dvarName, float value, float min, float max, unsigned int flags,
                                  const char *description);
bool Dvar_GetBool(const dvar_t *dvar);
int Dvar_GetInt(const dvar_t *dvar);
