#ifdef T6_STORE
#include "qcommon/qcommon.h"

extern "C" void StoreRequestDemonwareConnect(ControllerIndex_t controller);

extern "C" void Live_InitiateDemonWareConnect_f()
{
	StoreRequestDemonwareConnect(static_cast<ControllerIndex_t>(0));
}
#endif
