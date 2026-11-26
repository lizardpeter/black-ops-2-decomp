#pragma once
struct dvar_t {unsigned char pad[24];unsigned type;unsigned pad2;union {bool enabled;const char *string;} current;};
bool Dvar_GetBool(const dvar_t *);
