// Replay probe: universal/dvar.cpp's _Dvar_RegisterFloat (Dvar_RegisterVariant inlined) against the Store MP
// original, with Dvar_FindMalleableVar, Dvar_RegisterNew and Dvar_Reregister recorded. verify_dvar_register.py.
#include "universal/dvar.h"

#include <cstdint>
#include <cstring>

#define MS __attribute__((ms_abi))
using U = std::uint64_t;

namespace
{
struct Event
{
	U kind, name, type, flags, value, min, max, desc, dvar;
};
struct Capture
{
	U count;
	Event events[4];
	U result;
} cap;
bool exists;
dvar_t token;

U Bits(float f)
{
	std::uint32_t u;
	std::memcpy(&u, &f, 4);
	return u;
}

Event &Emit(U kind, const char *name)
{
	Event &e = cap.events[cap.count++ & 3];
	e = {};
	e.kind = kind;
	e.name = (U)name;
	return e;
}
}  // namespace

// What the function calls, for the reconstruction (C++ ABI, values by value) and, through the hooks, the original
// (Win64: the value and the domain by pointer). Only the lanes a float registration fills are recorded: the others
// are whatever the stack held.
dvar_t *Dvar_FindMalleableVar(const char *name)
{
	Emit(1, name);
	return exists ? &token : nullptr;
}

static void Record(U kind, const char *name, dvarType_t type, unsigned flags, const DvarValue &v, const DvarLimits &d,
                   const char *desc, const dvar_t *dvar)
{
	Event &e = Emit(kind, name);
	e.type = (U)type, e.flags = flags, e.value = Bits(v.value), e.min = Bits(d.value.min), e.max = Bits(d.value.max);
	e.desc = (U)desc, e.dvar = (U)dvar;
}

const dvar_t *Dvar_RegisterNew(const char *name, dvarType_t type, unsigned flags, DvarValue v, DvarLimits d,
                               const char *desc)
{
	Record(2, name, type, flags, v, d, desc, nullptr);
	return &token;
}

void Dvar_Reregister(dvar_t *dvar, const char *name, dvarType_t type, unsigned flags, DvarValue v, DvarLimits d,
                     const char *desc)
{
	Record(3, name, type, flags, v, d, desc, dvar);
}

static dvar_t *MS HookFind(const char *n) { return Dvar_FindMalleableVar(n); }
static const dvar_t *MS HookNew(const char *n, dvarType_t t, unsigned f, const DvarValue *v, const DvarLimits *d,
                                const char *s)
{
	return Dvar_RegisterNew(n, t, f, *v, *d, s);
}
static void MS HookRe(dvar_t *p, const char *n, dvarType_t t, unsigned f, const DvarValue *v, const DvarLimits *d,
                      const char *s)
{
	Dvar_Reregister(p, n, t, f, *v, *d, s);
}

extern "C" void *DvarRegisterHook(unsigned i)
{
	void *h[] = {(void *)HookFind, (void *)HookNew, (void *)HookRe};
	return h[i];
}

extern "C" void DvarRegisterRun(unsigned char *image, unsigned seed, float value, float min, float max, void *out)
{
	cap = {};
	exists = seed & 1;
	static const char *const names[] = {"cg_fovScale", "winvoice_mic_scaler", "", "x"};
	static const char *const descs[] = {"", "Microphone scaler value"};
	const char *name = names[(seed >> 1) & 3], *desc = descs[(seed >> 3) & 1];
	const unsigned flags = (seed >> 4) * 2654435761u;
	const dvar_t *r = image
		? reinterpret_cast<const dvar_t *(MS *)(const char *, float, float, float, unsigned, const char *)>(image + 0x46C650)(
			  name, value, min, max, flags, desc)
		: _Dvar_RegisterFloat(name, value, min, max, flags, desc);
	cap.result = (U)r;
	std::memcpy(out, &cap, sizeof(cap));
}

extern "C" unsigned DvarRegisterSize() { return sizeof(Capture); }
