// ui/ui_motd.cpp (CoDMPServer_PC: ui_motd.obj; Store MP 1404098F0 on)
//
// The message of the day. Treyarch's online file motd-mp-<language>.json (Live's file fetch, which unpacks it with the
// other online files, hands it to MOTD_Parse) holds, in this order:
//
//     { "version": 3, "motdVersion": n, "singleView": b, "expiration": t, "imageCategory": c,   (any order)
//       "data": [ { "vis": "<Lua function>", "motd": { "title": ..., "message": ..., ... },
//                   "banner": { "title": ..., "message": ..., ... } }, ... ],
//       "elite": { "eliteImage": ..., "founderImage": ... } }                                      (optional)
//
// Each entry of "data" whose "vis" function, called with the primary controller, returns true is copied into s_motd
// (so the last such entry wins); the others are skipped. The reference server build keeps only the value parsers (the
// dedicated server never calls the rest, so its linker dropped them): MOTD_ParseDataEntry, MOTD_Parse and
// MOTD_GetImageDescriptors_f are the Store build's, and their names are ours. The Store build has no error messages.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\ui\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "ui_motd.cpp"
#define T6_SOURCE "ui/ui_motd.cpp"
#define T6_CMD_SCOPE "MOTD_Parse"

#include "ui/ui_motd.h"

#include <string.h>
#include <windows.h>

#include "json/json.h"
#include "lui/hks.h"
#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "t6_static.h"
#include "universal/mem_largelocal.h"
#include "universal/q_shared.h"


#ifdef T6_STORE
#define MOTD_FILE_VERSION 3
#else
#define MOTD_FILE_VERSION 2
#endif

#define MOTD_MAX_TOKENS 128

T6_STATIC motd_t s_motd T6_AT("", "s_motd");

// ---- the header ----

// 00719850 ui/ui_motd.cpp:226
// Store MP 1404098F0 (tests/replay/verify_ui_motd.py).
// The file format's version: the server build reads version 2, the Store build 3.
bool MOTD_ParseVersion()
{
	jsmntok_t *token = JSMN_GetToken();
	if (!token || token->type != JSMN_PRIMITIVE || !token->val)
	{
#ifndef T6_STORE
		Com_PrintError(10, "MOTD: Expected version value\n");
#endif
		return false;
	}
	if (token->val->intVal != MOTD_FILE_VERSION)
	{
#ifndef T6_STORE
		Com_PrintError(10, "MOTD: Version mismatch. Expected %d, got %d\n", MOTD_FILE_VERSION, token->val->intVal);
#endif
		return false;
	}
	return true;
}

// A primitive's value into its field. A bool is copied as the byte it is (as the original's code does): "singleView": 2
// leaves 2 there, not 1.
static void MOTD_Assign(int &field, const int &value) { field = value; }
static void MOTD_Assign(bool &field, const bool &value) { memcpy(&field, &value, sizeof(bool)); }

// The parsers of a primitive value
#ifdef T6_STORE
#define MOTD_PRIMITIVE(func, line, field, member, message)        \
	bool func()                                                   \
	{                                                             \
		jsmntok_t *token = JSMN_GetToken();                       \
		if (!token || token->type != JSMN_PRIMITIVE || !token->val) \
			return false;                                         \
		MOTD_Assign(s_motd.field, token->val->member);            \
		return true;                                              \
	}
#else
#define MOTD_PRIMITIVE(func, line, field, member, message)        \
	bool func()                                                   \
	{                                                             \
		jsmntok_t *token = JSMN_GetToken();                       \
		if (!token || token->type != JSMN_PRIMITIVE || !token->val) \
		{                                                         \
			Com_PrintError(10, message);                          \
			return false;                                         \
		}                                                         \
		MOTD_Assign(s_motd.field, token->val->member);            \
		return true;                                              \
	}
#endif

// 007198A0 ui/ui_motd.cpp:247, Store MP 140409920
MOTD_PRIMITIVE(MOTD_ParseMOTDVersion, 247, version, intVal, "MOTD: Expected version value\n")
// 007198E0 ui/ui_motd.cpp:262, Store MP 140409960
MOTD_PRIMITIVE(MOTD_ParseSingleView, 262, singleView, boolVal, "MOTD: Expected singleView value\n")
// 00719920 ui/ui_motd.cpp:277, Store MP 1404099A0
MOTD_PRIMITIVE(MOTD_ParseExpiration, 277, expiration, intVal, "MOTD: Expected expiration value\n")

// 00719960 ui/ui_motd.cpp:292, Store MP 1404099E0
// (the low 16 bits of the value; the message is the original's, copied from the one above)
bool MOTD_ParseImageCategory()
{
	jsmntok_t *token = JSMN_GetToken();
	if (!token || token->type != JSMN_PRIMITIVE || !token->val)
	{
#ifndef T6_STORE
		Com_PrintError(10, "MOTD: Expected expiration value\n");
#endif
		return false;
	}
	s_motd.imageCategory = (unsigned short)token->val->intVal;
	return true;
}

// The parsers of a string value, copied (cut to the field) into s_motd
#ifdef T6_STORE
#define MOTD_STRING(func, field, message)                         \
	bool func()                                                   \
	{                                                             \
		jsmntok_t *token = JSMN_GetToken();                       \
		if (!token || token->type != JSMN_STRING || !token->val)  \
			return false;                                         \
		I_strncpyz(s_motd.field, token->val->stringVal, sizeof(s_motd.field)); \
		return true;                                              \
	}
#else
#define MOTD_STRING(func, field, message)                         \
	bool func()                                                   \
	{                                                             \
		jsmntok_t *token = JSMN_GetToken();                       \
		if (!token || token->type != JSMN_STRING || !token->val)  \
		{                                                         \
			Com_PrintError(10, message);                          \
			return false;                                         \
		}                                                         \
		I_strncpyz(s_motd.field, token->val->stringVal, sizeof(s_motd.field)); \
		return true;                                              \
	}
#endif

// ---- the message (the messages are the original's, some left from the CODTV parser they were copied from) ----

// 007199A0 ui/ui_motd.cpp:307, Store MP 140409A20
MOTD_STRING(MOTD_ParseTitle, title, "MOTD: Expected title\n")
// 007199E0 ui/ui_motd.cpp:322, Store MP 140409A80
MOTD_STRING(MOTD_ParseMessage, message, "MOTD: Expected message\n")
// 00719A20 ui/ui_motd.cpp:337, Store MP 140409AE0
MOTD_STRING(MOTD_ParseImage, image, "CODTV: Expected image name\n")
// 00719A60 ui/ui_motd.cpp:352, Store MP 140409B40
MOTD_STRING(MOTD_ParseAction, action, "CODTV: Expected action\n")
// 00719AA0 ui/ui_motd.cpp:367, Store MP 140409BA0
MOTD_STRING(MOTD_ParseActionContext, actionContext, "MOTD: Expected action context\n")
// 00719AE0 ui/ui_motd.cpp:382, Store MP 140409C00
MOTD_STRING(MOTD_ParseActionString, actionString, "MOTD: Expected action string\n")

// ---- the banner ----

// 00719B20 ui/ui_motd.cpp:398, Store MP 140409C60
MOTD_STRING(MOTD_ParseBannerTitle, bannerTitle, "MOTD: Expected title\n")
// 00719B60 ui/ui_motd.cpp:413, Store MP 140409CC0
MOTD_STRING(MOTD_ParseBannerMessage, bannerMessage, "CODTV: Expected banner message\n")
// 00719BA0 ui/ui_motd.cpp:428, Store MP 140409D20
MOTD_STRING(MOTD_ParseBannerImage, bannerImage, "CODTV: Expected banner image ID\n")
// 00719BE0 ui/ui_motd.cpp:443, Store MP 140409D80
MOTD_STRING(MOTD_ParseBannerAction, bannerAction, "CODTV: Expected action\n")
// 00719C20 ui/ui_motd.cpp:458, Store MP 140409DE0
MOTD_STRING(MOTD_ParseBannerActionContext, bannerActionContext, "MOTD: Expected action context\n")
// 00719C60 ui/ui_motd.cpp:473, Store MP 140409E40
MOTD_STRING(MOTD_ParseBannerActionString, bannerActionString, "MOTD: Expected action string\n")

// ---- Elite (the Call of Duty Elite service's images) ----

// 00719CA0 ui/ui_motd.cpp:487, Store MP 140409EA0
MOTD_STRING(MOTD_ParseEliteImage, eliteImage, "CODTV: Expected elite image\n")
// 00719CE0 ui/ui_motd.cpp:502, Store MP 140409F00
MOTD_STRING(MOTD_ParseFounderImage, founderImage, "CODTV: Expected elite image\n")

// The keys of each object (server 00DB260C on; Store MP 140A6AF70 on)
static const motdKeyHandler_t s_motdVersionHandlers[] = {
	{"version", MOTD_ParseVersion},       {"motdVersion", MOTD_ParseMOTDVersion}, {"singleView", MOTD_ParseSingleView},
	{"expiration", MOTD_ParseExpiration}, {"imageCategory", MOTD_ParseImageCategory},
};
static const motdKeyHandler_t s_motdBodyHandlers[] = {
	{"title", MOTD_ParseTitle},   {"message", MOTD_ParseMessage},           {"image", MOTD_ParseImage},
	{"action", MOTD_ParseAction}, {"actionContext", MOTD_ParseActionContext}, {"actionString", MOTD_ParseActionString},
};
static const motdKeyHandler_t s_motdBannerHandlers[] = {
	{"title", MOTD_ParseBannerTitle},
	{"message", MOTD_ParseBannerMessage},
	{"image", MOTD_ParseBannerImage},
	{"bannerAction", MOTD_ParseBannerAction},
	{"bannerActionContext", MOTD_ParseBannerActionContext},
	{"bannerActionString", MOTD_ParseBannerActionString},
};
static const motdKeyHandler_t s_motdEliteHandlers[] = {
	{"eliteImage", MOTD_ParseEliteImage},
	{"founderImage", MOTD_ParseFounderImage},
};

// The next token as a key of `handlers`, its value parsed by that key's handler: false for anything else, or when the
// handler fails.
template <int N> static bool MOTD_ParseKey(const motdKeyHandler_t (&handlers)[N])
{
	jsmntok_t *token = JSMN_GetToken();
	if (!token || token->type != JSMN_KEY || !token->val)
		return false;
	for (const motdKeyHandler_t &h : handlers)
	{
		if (!I_stricmp(h.key, token->val->stringVal))
			return h.handler();
	}
	return false;
}

#ifdef T6_STORE  // the rest isn't in the server build (see the top)
LocalClientNum_t Com_LocalClients_GetPrimary();
ControllerIndex_t Com_LocalClient_GetControllerIndex(LocalClientNum_t localClientNum);

// LUI's Lua state and the lock around it (names ours)
T6_GLOBAL lua_State *LUI_luaVM;
T6_GLOBAL CRITICAL_SECTION g_luiCriticalSection;

// The next token as a key named `key` (without case).
static bool MOTD_IsKey(const char *key)
{
	jsmntok_t *token = JSMN_GetToken();
	return token && token->type == JSMN_KEY && token->val && !I_stricmp(token->val->stringVal, key);
}

// Calls the Lua global function `name` with the primary controller: whether it returned true. False when name is
// empty or not a function.
static bool MOTD_IsVisible(const char *name)
{
	const int controller = Com_LocalClient_GetControllerIndex(Com_LocalClients_GetPrimary());
	if (!*name)
		return false;
	EnterCriticalSection(&g_luiCriticalSection);
	lua_getfield(LUI_luaVM, LUA_GLOBALSINDEX, name);
	if (!lua_isfunction(LUI_luaVM, -1))
	{
		lua_pop(LUI_luaVM, 1);
		LeaveCriticalSection(&g_luiCriticalSection);
		return false;
	}
	lua_pushnumber(LUI_luaVM, (float)controller);
	lua_pcall(LUI_luaVM, 1, 1, 0);
	const int visible = lua_toboolean(LUI_luaVM, -1);
	lua_pop(LUI_luaVM, 1);
	LeaveCriticalSection(&g_luiCriticalSection);
	return visible > 0;
}

// An object's `pairs` key/value pairs: parsed with `handlers` when `parse`, else skipped one token each (which
// assumes the values are strings or primitives).
template <int N> static bool MOTD_ParseObject(const motdKeyHandler_t (&handlers)[N], int size, bool parse)
{
	if (parse)
	{
		for (int i = 0; i < size / 2; i++)
		{
			if (!MOTD_ParseKey(handlers))
				return false;
		}
		return true;
	}
	for (int i = 0; i < size; i++)
	{
		if (!JSMN_GetToken())
			return false;
	}
	return true;
}

// Store MP 140409F60 (tests/replay/verify_ui_motd.py).
// One entry of "data": {"vis": <Lua function>, "motd": {...}, then optionally "banner": {...}}; copied into s_motd
// when the function says the entry is for this player.
bool MOTD_ParseDataEntry()
{
	jsmntok_t *token = JSMN_GetToken();
	if (!token || token->type != JSMN_OBJECT)
		return false;
	if (!MOTD_IsKey("vis"))
		return false;
	token = JSMN_GetToken();
	if (!token || token->type != JSMN_STRING || !token->val)
		return false;
	const bool visible = MOTD_IsVisible(token->val->stringVal);

	if (!MOTD_IsKey("motd"))
		return false;
	token = JSMN_GetToken();
	if (!token || token->type != JSMN_OBJECT)
		return false;
	if (!MOTD_ParseObject(s_motdBodyHandlers, token->size, visible))
		return false;

	if (!MOTD_IsKey("banner"))
	{
		JSMN_UnGetToken();
		return true;
	}
	token = JSMN_GetToken();
	if (!token || token->type != JSMN_OBJECT)
		return false;
	return MOTD_ParseObject(s_motdBannerHandlers, token->size, visible);
}

// Store MP 14040A860 (tests/replay/verify_ui_motd.py).
// The downloaded file (UTF-8, UTF-16 or UTF-32 byte order marks skipped; the text itself must be 8-bit) into s_motd.
// The command that fetches the images' descriptors is registered once the file has parsed.
bool MOTD_Parse(const char *buffer)
{
	LargeLocal tokensBuf(MOTD_MAX_TOKENS * sizeof(jsmntok_t));
	jsmntok_t *tokens = static_cast<jsmntok_t *>(tokensBuf.GetBuf());
	if (!tokens)
		return false;

	// (the original reads the first four bytes whatever the file's length: the buffer is always larger)
	unsigned int first4;
	unsigned short first2;
	memcpy(&first4, buffer, sizeof(first4));
	memcpy(&first2, buffer, sizeof(first2));
	int bom = 0;
	if (first4 == 0xFFFE0000 || first4 == 0xFEFF)
		bom = 4;
	else if (first2 == 0xFFFE || first2 == 0xFEFF)
		bom = 2;
	else if (first2 == 0xBBEF && (unsigned char)buffer[2] == 0xBF)
		bom = 3;
	JSMN_BeginParseSession(buffer + bom, tokens, MOTD_MAX_TOKENS);
	memset(&s_motd, 0, sizeof(s_motd));

	jsmntok_t *token = JSMN_GetToken();
	if (!token || token->type != JSMN_OBJECT)
		return false;
	for (int i = 0; i < 5; i++)
	{
		if (!MOTD_ParseKey(s_motdVersionHandlers))
			return false;
	}

	if (!MOTD_IsKey("data"))
		return false;
	jsmntok_t *data = JSMN_GetToken();
	if (!data || data->type != JSMN_ARRAY)
		return false;
	for (int i = 0; i < data->size; i++)
	{
		if (!MOTD_ParseDataEntry())
			return false;
	}

	if (MOTD_IsKey("elite"))
	{
		token = JSMN_GetToken();
		if (token && token->type == JSMN_OBJECT)
		{
			const int pairs = token->size / 2;
			for (int i = 0; i < pairs && MOTD_ParseKey(s_motdEliteHandlers); i++)
			{
			}
		}
	}
	else
		JSMN_UnGetToken();

	Cmd_AddCommand("motdGetImageDescriptors", MOTD_GetImageDescriptors_f);
	if (s_motd.message[0])
		s_motd.isValid = true;
	return true;
}
#endif  // T6_STORE
