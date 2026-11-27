// qcommon/cmd.cpp (CoDMPServer_PC: cmd.obj)
//
// Console commands. So far registering one (the Store build inlines both functions into each caller, without the
// asserts and the "already defined" message).
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\qcommon\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "cmd.cpp"
#define T6_SOURCE "qcommon/cmd.cpp"

#include <string.h>

#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "t6_static.h"

T6_STATIC cmd_function_s *cmd_functions T6_AT("", "cmd_functions");

// 006B2990 qcommon/cmd.cpp:1322
// The command named cmdName (case matters), or null.
cmd_function_s *Cmd_FindCommand(const char *cmdName)
{
	for (cmd_function_s *cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!strcmp(cmdName, cmd->name))
			return cmd;
	}
	return nullptr;
}

// 006B29E0 qcommon/cmd.cpp:1335
// Adds allocedCmd to the front of the list as cmdName, unless a command of that name is there already (which must be
// this same record).
void Cmd_AddCommandInternal(const char *cmdName, void (*function)(), cmd_function_s *allocedCmd)
{
	assert_at(1338, cmdName);
	cmd_function_s *cmd = Cmd_FindCommand(cmdName);
	if (cmd)
	{
		assert_at(1343, cmd == allocedCmd);
#ifndef T6_STORE
		if (function)
			Com_Printf(10, "Cmd_AddCommand: %s already defined\n", cmdName);
#endif
		return;
	}
	allocedCmd->name = cmdName;
	allocedCmd->function = function;
	allocedCmd->next = cmd_functions;
	cmd_functions = allocedCmd;
}
