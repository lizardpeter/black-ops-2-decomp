// universal/mem_largelocal.cpp (CoDMPServer_PC: mem_largelocal.obj)
//
// The Store build inlines all of it into its callers, with no asserts (the thread checks go with them: a thread that
// isn't the main one takes the server thread's side).
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\universal\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "mem_largelocal.cpp"
#define T6_SOURCE "universal/mem_largelocal.cpp"

#include "universal/mem_largelocal.h"

#include "qcommon/threads.h"
#include "t6_assert.h"
#include "t6_static.h"

T6_GLOBAL unsigned char g_largeLocalBuf[0x80000];
T6_STATIC int g_largeLocalPos T6_AT("", "g_largeLocalPos");                       // the main thread's next free byte
T6_STATIC int g_largeLocalRightPos T6_AT("", "g_largeLocalRightPos") T6_INIT(= sizeof(g_largeLocalBuf));  // the
                                                                                  // server thread's lowest used byte

// 0078A110 universal/mem_largelocal.cpp:32
int LargeLocalBegin(int size)
{
	assert_at(35, Sys_IsMainThread());
	const int startPos = g_largeLocalPos;
	g_largeLocalPos += (size + 3) & ~3;
	assert_at(43, g_largeLocalPos <= static_cast< int >( sizeof( g_largeLocalBuf ) ));
	assert_at(45, g_largeLocalPos <= g_largeLocalRightPos);
	return startPos;
}

// 00789FA0 universal/mem_largelocal.cpp:58
void LargeLocalEnd(int startPos)
{
	assert_at(59, Sys_IsMainThread());
	g_largeLocalPos = startPos;
	assert_at(64, g_largeLocalPos <= g_largeLocalRightPos);
}

// 0078A1B0 universal/mem_largelocal.cpp:77
int LargeLocalBeginRight(int size)
{
	assert_at(80, Sys_IsServerThread());
	size = (size + 3) & ~3;
	assert_at(84, g_largeLocalRightPos - size > g_largeLocalPos);
	const int startPos = g_largeLocalRightPos;
	g_largeLocalRightPos -= size;
	return startPos;
}

// 0078A000 universal/mem_largelocal.cpp:99
// (the server passes startPos in a register: link-time code generation)
void LargeLocalEndRight(int startPos)
{
	assert_at(100, Sys_IsServerThread());
	g_largeLocalRightPos = startPos;
	assert_at(104, g_largeLocalRightPos > g_largeLocalPos);
}

// 0078A230 universal/mem_largelocal.cpp:117
void *LargeLocalGetBuf(int startPos, int size)
{
	assert_at(121, Sys_IsMainThread() || Sys_IsServerThread());
	if (Sys_IsMainThread())
		return &g_largeLocalBuf[startPos];
	const int startIndex = startPos - ((size + 3) & ~3);
	assert_at(134, startIndex >= 0);
	return &g_largeLocalBuf[startIndex];
}

// 0078A2C0 universal/mem_largelocal.cpp:147
LargeLocal::LargeLocal(int sizeParam)
{
	assert_at(149, Sys_IsMainThread() || Sys_IsServerThread());
	if (Sys_IsMainThread())
		startPos = LargeLocalBegin(sizeParam);
	else
		startPos = LargeLocalBeginRight(sizeParam);
	size = sizeParam;
}

// 0078A060 universal/mem_largelocal.cpp:173
LargeLocal::~LargeLocal()
{
	assert_at(175, Sys_IsMainThread() || Sys_IsServerThread());
	if (Sys_IsMainThread())
		LargeLocalEnd(startPos);
	else
		LargeLocalEndRight(startPos);
}

// 0078A340 universal/mem_largelocal.cpp:197
void *LargeLocal::GetBuf()
{
	assert_at(199, Sys_IsMainThread() || Sys_IsServerThread());
	return LargeLocalGetBuf(startPos, size);
}

// 0078A0C0 universal/mem_largelocal.cpp:214
void LargeLocalReset()
{
	assert_at(216, Sys_IsMainThread() || Sys_IsServerThread());
	g_largeLocalRightPos = sizeof(g_largeLocalBuf);
	g_largeLocalPos = 0;
}
