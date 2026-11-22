// client/splitscreen.cpp (CoDMPServer_PC: splitscreen.obj)
//
// Local clients for split screen. One local client in both builds: so far the checks the input code makes.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\client\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "splitscreen.cpp"
#define T6_SOURCE "client/splitscreen.cpp"

#include "t6_assert.h"
#include "client/client.h"

// 00982EE0  client/splitscreen.cpp:53 (inlined in the Store build)
bool CL_LocalClient_IsActive(LocalClientNum_t localClientNum)
{
	assertf_at(54, (localClientNum >= 0 && localClientNum < 1), "(localClientNum) = %i", localClientNum);
	return clientUIActives[localClientNum].flags & 1;
}

// 0080BBD0  client/splitscreen.cpp:101 (folded with an empty physics function on the server; inlined in the Store
// build): split screen is off in both
bool CL_SplitscreenEnabled()
{
	return false;
}

// 009831B0  client/splitscreen.cpp:308 (inlined in the Store build)
// Whether no local client before this one is active.
bool CL_IsFirstActiveLocalClient(LocalClientNum_t localClientNum)
{
	bcassert_at(311, localClientNum, MAX_LOCAL_CLIENTS);
	for (int i = 0; i < localClientNum; i++)
	{
		if (CL_LocalClient_IsActive((LocalClientNum_t)i))
			return false;
	}
	return true;
}
