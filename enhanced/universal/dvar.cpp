// universal/dvar.cpp, enhanced (P0018): settings registered without the cheat flag.
//
// A dvar registered with DVAR_CHEAT can be changed only while sv_cheats is on, and goes back to its default whenever
// sv_cheats is off. The settings below only change what the player sees, so the enhanced build registers them without
// that flag; everything else about their registration (type, default, domain, other flags) is the original's.
// Store build only (the build that is played).
#ifdef T6_STORE
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\universal\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "dvar.cpp"
#define T6_SOURCE "universal/dvar.cpp"

#include "universal/dvar.h"

#include "t6_assert.h"

namespace
{
const char *const s_unlockedCheatDvars[] = {
	"cg_fovScale",  // the view's field of view as a multiple of cg_fov
};

// Dvar names compare without case, as the game's own lookup does.
bool SameDvarName(const char *a, const char *b)
{
	for (;; ++a, ++b)
	{
		const char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
		const char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
		if (ca != cb)
			return false;
		if (!ca)
			return true;
	}
}

unsigned int Dvar_EnhancedFlags(const char *dvarName, unsigned int flags)
{
	for (const char *name : s_unlockedCheatDvars)
	{
		if (SameDvarName(dvarName, name))
			return flags & ~DVAR_CHEAT;
	}
	return flags;
}
}  // namespace

// 14046C650 (faithful: src/universal/dvar.cpp)
const dvar_t *_Dvar_RegisterFloat(const char *dvarName, float value, float min, float max, unsigned int flags,
                                  const char *description)
{
	DvarValue dvarValue;
	dvarValue.value = value;
	DvarLimits domain;
	domain.value.min = min;
	domain.value.max = max;
	return Dvar_RegisterVariant(dvarName, DVAR_TYPE_FLOAT, Dvar_EnhancedFlags(dvarName, flags), dvarValue, domain,
	                            description);
}
#endif
