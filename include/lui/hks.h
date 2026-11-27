// Havok Script (hks), the Lua 5.1 virtual machine the LUI menus run on, as the game's own code uses it: the Lua C API
// functions it calls and the ones its headers inline (pushing a number or a boolean, popping) on the value stack.
// Only what reconstructed code needs so far, from the Store build (the reference server build has no LUI). The
// function names are Lua's: the Store build has no symbols for them.
#pragma once

enum
{
	LUA_REGISTRYINDEX = -10000,
	LUA_ENVIRONINDEX = -10001,
	LUA_GLOBALSINDEX = -10002,
};

// A value's type tag (the low four bits of HksObject::t)
enum HksObjectType : unsigned int
{
	TNIL = 0,
	TBOOLEAN = 1,
	TLIGHTUSERDATA = 2,
	TNUMBER = 3,  // hks numbers are floats
	TSTRING = 4,
	TTABLE = 5,
	TIFUNCTION = 9,   // a Lua function
	TCFUNCTION = 10,  // a C function
};

struct HksObject
{
	unsigned int t;
	union
	{
		void *ptr;
		float number;
		int boolean;
		unsigned long long bits;
	} v;
};
static_assert(sizeof(HksObject) == 16, "HksObject");

struct lua_State
{
	unsigned char unknown0[0x48];
	HksObject *top;   // the first free stack slot
	HksObject *base;  // the running function's first argument
};
#if __SIZEOF_POINTER__ == 8
static_assert(__builtin_offsetof(lua_State, top) == 0x48, "lua_State::top");
#endif

void lua_getfield(lua_State *s, int index, const char *k);
int lua_isfunction(lua_State *s, int index);
int lua_toboolean(lua_State *s, int index);
int lua_pcall(lua_State *s, int nargs, int nresults, int errfunc);

// Inlined by hks's header: no check that the stack has room (the caller's frame always does).
inline void lua_pushnumber(lua_State *s, float n)
{
	HksObject *top = s->top;
	top->t = TNUMBER;
	top->v.number = n;
	s->top = top + 1;
}

inline void lua_pushboolean(lua_State *s, int b)
{
	HksObject *top = s->top;
	top->v.boolean = b;
	top->t = TBOOLEAN;
	s->top = top + 1;
}

inline void lua_pop(lua_State *s, int n) { s->top -= n; }
