// Replay probe: ui/ui_motd.cpp's MOTD_Parse (with MOTD_ParseDataEntry, the value parsers, and the inlined
// LargeLocal, JSMN session and Cmd_AddCommand) against the Store MP original. verify_ui_motd.py drives it.
//
// Both sides call the same fake Lua (a value stack in a fake lua_State; the "vis" functions are named in the file and
// return true, false, a number or nothing) and record everything into one buffer: the result, s_motd, the Lua calls
// and the stack, the command list, the large-local positions, the token buffer and the session.
#include "json/json.h"
#include "lui/hks.h"
#include "qcommon/qcommon.h"
#include "ui/ui_motd.h"

#include <windows.h>

#include <asm/prctl.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/syscall.h>
#include <unistd.h>

#define MS __attribute__((ms_abi))

// ---- the reconstruction's globals and statics ----
const char *s_json;
jsmntok_t *s_tokens;
int s_currToken;
int s_numTokens;
jsmn_parser s_parser;
jsmnVal_u s_val;
motd_t s_motd;
cmd_function_s *cmd_functions;
cmd_function_s MOTD_GetImageDescriptors_f_VAR;
unsigned char g_largeLocalBuf[0x80000];
int g_largeLocalPos;
int g_largeLocalRightPos;
lua_State *LUI_luaVM;
CRITICAL_SECTION g_luiCriticalSection;
ClientGameState clientGameStates[MAX_LOCAL_CLIENTS];
LocalClientNum_t primaryLocalClient;

namespace
{
// the original's, as offsets into the image
constexpr unsigned S_MOTD = 0x40ABD20, CMD_VAR = 0x44D70E0, CMD_FUNCTIONS = 0x3F00478, LUA_VM = 0x265E878,
                   BUF = 0x21616C0, POS = 0x41B24F8, RIGHT = 0xA63A58, THREAD_ID = 0x406FBF0, PRIMARY = 0xA53D88,
                   CLIENTS = 0x1EF91E0, CURR = 0x3ECFFA0, NUM = 0x3ECFFA4;
constexpr unsigned MAIN_ID = 1234;

unsigned state;
unsigned Next() { return state = state * 1664525u + 1013904223u; }
unsigned Pick(unsigned n) { return Next() % n; }

bool g_main;
unsigned g_last;
unsigned char *g_image;  // the side running: the image, or null for the reconstruction

struct Out
{
	unsigned char bytes[0x6000];
	unsigned n;
	void Put(const void *p, unsigned len)
	{
		if (n + len <= sizeof(bytes))
			std::memcpy(bytes + n, p, len);
		n += len;
	}
	template <class T> void Put(T v) { Put(&v, sizeof(v)); }
	void Str(const char *s) { Put(s, (unsigned)std::strlen(s) + 1); }
} *g_out;

// ---- the fake Lua ----
alignas(16) unsigned char g_state[0x100];
HksObject g_stack[64];
lua_State *L() { return reinterpret_cast<lua_State *>(g_state); }

// What a global named in the file is (by a hash of the name): mostly a function returning true; or one returning
// false, a number or nothing; or not a function at all.
unsigned Hash(const char *name)
{
	unsigned h = 7;
	for (; *name; ++name)
		h = h * 31 + (unsigned char)*name;
	return h;
}
unsigned Kind(const char *name) { return Hash(name) % 8; }  // 0-3 true, 4 false, 5 a number, 6 nil, 7 not a function

HksObject *Slot(int index) { return index < 0 ? L()->top + index : L()->base + index - 1; }
}  // namespace

void lua_getfield(lua_State *s, int index, const char *k)
{
	g_out->Put('g'), g_out->Put(index), g_out->Str(k), g_out->Put((long long)(s->top - g_stack));
	const unsigned kind = Kind(k);
	*s->top = {kind == 7 ? (Hash(k) & 8 ? TNUMBER : TNIL) : (Hash(k) & 16 ? TCFUNCTION : TIFUNCTION), {}};
	s->top->v.bits = kind;
	s->top++;
}

int lua_isfunction(lua_State *, int index)
{
	g_out->Put('f'), g_out->Put(index);
	const unsigned t = Slot(index)->t & 0xF;
	return t == TIFUNCTION || t == TCFUNCTION;  // (an int, as the original returns)
}

int lua_pcall(lua_State *s, int nargs, int nresults, int errfunc)
{
	g_out->Put('p'), g_out->Put(nargs), g_out->Put(nresults), g_out->Put(errfunc);
	for (int i = 0; i < nargs; ++i)
		g_out->Put(s->top[-1 - i].t), g_out->Put(s->top[-1 - i].v.bits);
	const unsigned kind = (unsigned)s->top[-1 - nargs].v.bits;
	s->top -= nargs + 1;
	*s->top = {kind < 5 ? TBOOLEAN : kind == 5 ? TNUMBER : TNIL, {}};
	s->top->v.boolean = kind < 4;
	s->top++;
	return 0;
}

int lua_toboolean(lua_State *, int index)
{
	g_out->Put('b'), g_out->Put(index);
	const HksObject *o = Slot(index);
	return (o->t & 0xF) != TNIL && ((o->t & 0xF) != TBOOLEAN || o->v.boolean);
}

void WINAPI EnterCriticalSection(LPCRITICAL_SECTION) { g_out->Put('E'); }
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION) { g_out->Put('L'); }
bool Sys_IsMainThread() { return g_main; }
bool Sys_IsServerThread() { return !g_main; }
void MOTD_GetImageDescriptors_f() {}

// q_shared.cpp's helpers (verified on their own) for the reconstruction: the original's, from the mapped image
static int(MS *s_stricmp)(const char *, const char *);
static void(MS *s_strncpyz)(char *, const char *, int);
int I_stricmp(const char *a, const char *b) { return s_stricmp(a, b); }
void I_strncpyz(char *dest, const char *src, int destsize) { s_strncpyz(dest, src, destsize); }

namespace
{
void MS HookGetField(lua_State *s, int i, const char *k) { lua_getfield(s, i, k); }
int MS HookIsFunction(lua_State *s, int i) { return lua_isfunction(s, i); }
int MS HookPcall(lua_State *s, int a, int r, int e) { return lua_pcall(s, a, r, e); }
int MS HookToBoolean(lua_State *s, int i) { return lua_toboolean(s, i); }
void MS HookEnter(LPCRITICAL_SECTION c) { EnterCriticalSection(c); }
void MS HookLeave(LPCRITICAL_SECTION c) { LeaveCriticalSection(c); }
unsigned MS HookThreadId() { return MAIN_ID; }
int MS HookAtoi(const char *s) { return std::atoi(s); }
void *MS HookMemset(void *d, int c, size_t n) { return std::memset(d, c, n); }
int MS HookStrcmp(const char *a, const char *b) { return std::strcmp(a, b); }

// The thread's TEB for the original's TLS reads (gs:[0x58] -> slot 0 -> the cached thread id at +0x3C)
unsigned char g_teb[0x100], g_tlsBlock[0x100];
void *g_tlsArray[4];

// ---- the MOTD files ----
char g_text[0x8000];
unsigned g_at;
void Put(const char *s)
{
	while (*s && g_at < sizeof(g_text) - 2)
		g_text[g_at++] = *s++;
	g_text[g_at] = 0;
}
void PutF(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void PutF(const char *fmt, ...)
{
	char b[1400];
	va_list a;
	va_start(a, fmt);
	vsnprintf(b, sizeof(b), fmt, a);
	va_end(a);
	Put(b);
}

void Value(bool stringy)
{
	if (stringy && Pick(30))
	{
		const unsigned len = Pick(5) ? Pick(20) : (Pick(2) ? 60 + Pick(80) : 1000 + Pick(200));
		Put("\"");
		for (unsigned i = 0; i < len; ++i)
			Put(i == 3 ? "\\n" : (i % 5 ? "m" : "M"));
		Put("\"");
		return;
	}
	static const char *const prims[] = {"3", "2", "0", "true", "false", "null", "70000", "-1", "\"3\"", "{}", "[]"};
	Put(prims[Pick(stringy ? 11 : (Pick(4) ? 8 : 11))]);
}

void Object(const char *const *keys, unsigned nkeys, bool stringy)
{
	Put("{");
	const unsigned n = Pick(4) ? 1 + Pick(nkeys) : Pick(nkeys + 2);
	for (unsigned i = 0; i < n; ++i)
	{
		PutF("%s\"%s\":", i ? "," : "", Pick(40) ? keys[Pick(nkeys)] : "unknownKey");
		Value(stringy);
	}
	Put("}");
}

void MakeFile()
{
	g_at = 0;
	static const char *const boms[] = {"", "", "", "\xEF\xBB\xBF", "\xFF\xFE", "\xFE\xFF"};
	Put(boms[Pick(6)]);
	if (Pick(16) == 0)
	{
		Put(Pick(2) ? "[1,2]" : "{\"version\":");
		return;
	}
	static const char *const header[] = {"version", "motdVersion", "singleView", "expiration", "imageCategory"};
	unsigned order[5] = {0, 1, 2, 3, 4};
	for (unsigned i = 4; i > 0; --i)
	{
		const unsigned j = Pick(i + 1), t = order[i];
		order[i] = order[j], order[j] = t;
	}
	Put("{");
	const unsigned nheader = Pick(10) ? 5 : 3 + Pick(4);
	for (unsigned i = 0; i < nheader; ++i)
	{
		const char *key = i < 5 ? header[order[i]] : "extra";
		PutF("%s\"%s\":", i ? "," : "", Pick(30) ? key : "VERSION");
		if (i < 5 && order[i] == 0)
			Put(Pick(8) ? "3" : (Pick(2) ? "2" : "\"3\""));
		else
			Value(false);
	}
	if (Pick(20))
	{
		Put(Pick(25) ? ",\"data\":[" : ",\"Data\":[");
		const unsigned entries = Pick(4);
		static const char *const vis[] = {"IsElite", "IsFounder", "AlwaysShow", "Never", "x", "y", "", "notAFunction",
		                                  "a", "b", "c", "d", "e", "f", "g", "h"};
		static const char *const body[] = {"title", "message", "image", "action", "actionContext", "actionString"};
		static const char *const banner[] = {"title", "message", "image", "bannerAction", "bannerActionContext",
		                                     "bannerActionString"};
		for (unsigned e = 0; e < entries; ++e)
		{
			PutF("%s{\"%s\":\"%s\",\"motd\":", e ? "," : "", Pick(30) ? "vis" : "VIS", vis[Pick(16)]);
			Object(body, 6, true);
			if (Pick(2))
			{
				Put(Pick(20) ? ",\"banner\":" : ",\"other\":");
				Object(banner, 6, true);
			}
			Put("}");
		}
		Put("]");
	}
	if (Pick(2))
	{
		static const char *const elite[] = {"eliteImage", "founderImage"};
		Put(Pick(10) ? ",\"elite\":" : ",\"notElite\":");
		Object(elite, 2, true);
	}
	Put(Pick(12) ? "}" : "");
}

template <class T> T &At(unsigned rva) { return *reinterpret_cast<T *>(g_image + rva); }
}  // namespace

extern "C" void *MotdHook(unsigned i)
{
	void *h[] = {(void *)HookGetField, (void *)HookIsFunction, (void *)HookPcall, (void *)HookToBoolean,
	             (void *)HookEnter,    (void *)HookLeave,      (void *)HookThreadId, (void *)HookAtoi,
	             (void *)HookMemset,   (void *)HookStrcmp};
	return h[i];
}

extern "C" int MotdInit(unsigned char *image)
{
	s_stricmp = reinterpret_cast<int(MS *)(const char *, const char *)>(image + 0x471C80);
	s_strncpyz = reinterpret_cast<void(MS *)(char *, const char *, int)>(image + 0x471B00);
	g_tlsArray[0] = g_tlsBlock;
	*reinterpret_cast<void **>(g_teb + 0x58) = g_tlsArray;
	*reinterpret_cast<unsigned *>(g_tlsBlock + 0x3C) = MAIN_ID;
	return (int)syscall(SYS_arch_prctl, ARCH_SET_GS, (unsigned long)g_teb);
}

extern "C" unsigned MotdRun(unsigned char *image, unsigned seed, Out *o)
{
	state = seed + 1;
	g_out = o, o->n = 0, g_image = image;
	MakeFile();
	g_main = Pick(4) != 0;
	const int pos = 4 * (int)Pick(0x4000), right = 0x80000 - 4 * (int)Pick(0x4000);
	const int primary = 0;  // MAX_LOCAL_CLIENTS is 1
	const bool mapped = Pick(8) != 0;  // the primary client's slot holds its own number
	const int controller = (int)Pick(4);
	const unsigned chain = Pick(3);   // the command list: empty, another command, or this one already in it

	// a fresh Lua stack with two values on it
	std::memset(g_state, 0, sizeof(g_state));
	std::memset(g_stack, 0, sizeof(g_stack));
	L()->base = g_stack;
	L()->top = g_stack + 2;
	static cmd_function_s other = {nullptr, "other", nullptr, nullptr, nullptr};

	motd_t *motd;
	cmd_function_s **head, *var;
	unsigned char *buf;
	int *posp, *rightp;
	void *function;
	if (image)
	{
		At<unsigned>(THREAD_ID) = g_main ? MAIN_ID : MAIN_ID + 1;
		At<lua_State *>(LUA_VM) = L();
		At<int>(PRIMARY) = primary;
		At<int>(CLIENTS + 0x18 * primary + 4) = mapped ? primary : primary + 1;
		At<int>(CLIENTS + 0x18 * primary + 8) = controller;
		motd = &At<motd_t>(S_MOTD), head = &At<cmd_function_s *>(CMD_FUNCTIONS), var = &At<cmd_function_s>(CMD_VAR);
		buf = image + BUF, posp = &At<int>(POS), rightp = &At<int>(RIGHT);
		function = image + 0x40A7A0;
	}
	else
	{
		LUI_luaVM = L();
		primaryLocalClient = (LocalClientNum_t)primary;
		clientGameStates[primary].localClientNum = (LocalClientNum_t)(mapped ? primary : primary + 1);
		clientGameStates[primary].controllerIndex = (ControllerIndex_t)controller;
		motd = &s_motd, head = &cmd_functions, var = &MOTD_GetImageDescriptors_f_VAR;
		buf = g_largeLocalBuf, posp = &g_largeLocalPos, rightp = &g_largeLocalRightPos;
		function = reinterpret_cast<void *>(MOTD_GetImageDescriptors_f);
	}
	std::memset(motd, 0xA5, sizeof(*motd));
	std::memset(var, 0, sizeof(*var));
	other.next = nullptr;
	*head = chain == 0 ? nullptr : &other;
	if (chain == 2)
	{
		var->name = "motdGetImageDescriptors";
		var->next = *head;
		*head = var;
	}
	*posp = pos, *rightp = right;
	const int start = g_main ? pos : right - 0x1000;
	std::memset(buf + start, 0x5C, 0x1000);

	const bool r = image ? reinterpret_cast<bool(MS *)(const char *)>(image + 0x40A860)(g_text) : MOTD_Parse(g_text);

	o->Put('R'), o->Put(r);
	g_last = r | (motd->isValid ? 2 : 0) | (std::strlen(motd->bannerTitle) && motd->bannerTitle[0] != (char)0xA5 ? 4 : 0);
	o->Put(motd, sizeof(*motd));
	o->Put((long long)(L()->top - g_stack));
	for (HksObject *p = g_stack; p < L()->top; ++p)
		o->Put(p->t), o->Put(p->v.bits);
	o->Put((unsigned char)(*head == var ? 1 : *head == &other ? 2 : *head ? 3 : 0));
	o->Put((unsigned char)(var->next == &other ? 2 : var->next ? 3 : 0));
	o->Str(var->name ? var->name : "(null)");
	o->Put((unsigned char)(var->function == function ? 1 : var->function ? 2 : 0));
	o->Put(*posp), o->Put(*rightp);
	// the tokens, with their value pointers made comparable (each side's s_val)
	unsigned char tokens[0x1000];
	std::memcpy(tokens, buf + start, sizeof(tokens));
	const void *val = image ? static_cast<void *>(image + 0x3ECFFC0) : static_cast<void *>(&s_val);
	for (unsigned i = 0; i + 0x20 <= sizeof(tokens); i += 0x20)
		if (*reinterpret_cast<const void **>(tokens + i + 0x10) == val)
			*reinterpret_cast<unsigned long long *>(tokens + i + 0x10) = 1;
	o->Put(tokens, sizeof(tokens));
	o->Put(image ? At<int>(CURR) : s_currToken), o->Put(image ? At<int>(NUM) : s_numTokens);
	return o->n;
}

extern "C" unsigned MotdOutSize() { return sizeof(Out); }
// what the last run gave: 1 parsed, 2 a valid message, 4 a banner title
extern "C" unsigned MotdLast() { return g_last; }
