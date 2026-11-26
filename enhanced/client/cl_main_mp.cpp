#ifdef T6_STORE
#include "enhanced/custom_games.h"

void CL_MapLoading_ConfigureDevmapParty(const char *map);

void CL_MapLoading_SetupDevmapParty(const char *map)
{
	// The Custom Games party already owns this map transition, including the preload.
	if (!CustomGames_UsesLocalHost())
		CL_MapLoading_ConfigureDevmapParty(map);
}
#endif
