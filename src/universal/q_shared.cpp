// universal/q_shared.cpp (CoDMPServer_PC: q_shared.obj)
//
// Every function the server's MAP lists for q_shared.obj, in original line order. Not reconstructed here: the
// blowfish helpers (s_blowfishKey, s_blowfishSalt) and whatever used difficulty_strings, which the server's
// link-time code generation inlined into their callers in other files and never emitted on their own. They come
// back with those callers.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\universal\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "q_shared.cpp"
#define T6_SOURCE "universal/q_shared.cpp"

#include "universal/q_shared.h"

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clientscript/cscr_stringlist.h"
#include "gfx_d3d/r_model.h"
#include "physics/physpreset_load_obj.h"
#include "qcommon/mem_track.h"
#include "qcommon/qcommon.h"
#include "qcommon/threads.h"
#include "sound/snd_utils.h"
#include "t6_assert.h"
#include "t6_compiler.h"
#include "t6_static.h"
#include "tracers/tracer_load_obj.h"
#include "universal/com_math.h"

T6_GLOBAL const vec4_t colorBlack T6_INIT(= { 0.0f, 0.0f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorRed T6_INIT(= { 1.0f, 0.0f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorLtRed T6_INIT(= { 0.75f, 0.0f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorMdRed T6_INIT(= { 0.5f, 0.0f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorDkRed T6_INIT(= { 0.25f, 0.0f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorGreen T6_INIT(= { 0.0f, 1.0f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorLtGreen T6_INIT(= { 0.0f, 0.75f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorMdGreen T6_INIT(= { 0.0f, 0.5f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorDkGreen T6_INIT(= { 0.0f, 0.25f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorBlue T6_INIT(= { 0.0f, 0.0f, 1.0f, 1.0f });
T6_GLOBAL const vec4_t colorLtBlue T6_INIT(= { 0.5f, 0.5f, 1.0f, 1.0f });
T6_GLOBAL const vec4_t colorMdBlue T6_INIT(= { 0.0f, 0.0f, 0.5f, 1.0f });
T6_GLOBAL const vec4_t colorYellow T6_INIT(= { 1.0f, 1.0f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorBOYellow T6_INIT(= { 1.0f, 0.94f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorDkYellow T6_INIT(= { 0.25f, 0.25f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorMdYellow T6_INIT(= { 0.5f, 0.5f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorLtYellow T6_INIT(= { 0.75f, 0.75f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorMagenta T6_INIT(= { 1.0f, 0.0f, 1.0f, 1.0f });
T6_GLOBAL const vec4_t colorPurple T6_INIT(= { 0.88f, 0.0f, 0.88f, 1.0f });
T6_GLOBAL const vec4_t colorLtPurple T6_INIT(= { 0.66f, 0.0f, 0.66f, 1.0f });
T6_GLOBAL const vec4_t colorMdPurple T6_INIT(= { 0.44f, 0.0f, 0.44f, 1.0f });
T6_GLOBAL const vec4_t colorDkPurple T6_INIT(= { 0.22f, 0.0f, 0.22f, 1.0f });
T6_GLOBAL const vec4_t colorPink T6_INIT(= { 0.88f, 0.44f, 0.88f, 1.0f });
T6_GLOBAL const vec4_t colorCyan T6_INIT(= { 0.0f, 1.0f, 1.0f, 1.0f });
T6_GLOBAL const vec4_t colorLtCyan T6_INIT(= { 0.0f, 0.75f, 0.75f, 1.0f });
T6_GLOBAL const vec4_t colorMdCyan T6_INIT(= { 0.0f, 0.5f, 0.5f, 1.0f });
T6_GLOBAL const vec4_t colorDkCyan T6_INIT(= { 0.0f, 0.25f, 0.25f, 1.0f });
T6_GLOBAL const vec4_t colorWhite T6_INIT(= { 1.0f, 1.0f, 1.0f, 1.0f });
T6_GLOBAL const vec4_t colorLtGrey T6_INIT(= { 0.75f, 0.75f, 0.75f, 1.0f });
T6_GLOBAL const vec4_t colorMdGrey T6_INIT(= { 0.5f, 0.5f, 0.5f, 1.0f });
T6_GLOBAL const vec4_t colorDkGrey T6_INIT(= { 0.25f, 0.25f, 0.25f, 1.0f });
T6_GLOBAL const vec4_t colorOrange T6_INIT(= { 1.0f, 0.7f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorLtOrange T6_INIT(= { 0.75f, 0.525f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorMdOrange T6_INIT(= { 0.5f, 0.35f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorDkOrange T6_INIT(= { 0.25f, 0.175f, 0.0f, 1.0f });
T6_GLOBAL const vec4_t colorWhiteFaded T6_INIT(= { 1.0f, 1.0f, 1.0f, 0.75f });
T6_GLOBAL const vec4_t colorGreenFaded T6_INIT(= { 0.0f, 1.0f, 0.0f, 0.75f });
T6_GLOBAL const vec4_t colorRedFaded T6_INIT(= { 0.75f, 0.25f, 0.0f, 0.75f });
T6_GLOBAL const vec4_t colorBlackBlank T6_INIT(= { 0.0f, 0.0f, 0.0f, 0.0f });

T6_STATIC va_info_t va_info[THREAD_CONTEXT_COUNT] T6_AT("", "va_info");
T6_STATIC jmp_buf g_com_error[THREAD_CONTEXT_COUNT] T6_AT("", "g_com_error");
T6_GLOBAL TraceThreadInfo g_traceThreadInfo[THREAD_CONTEXT_COUNT];
T6_STATIC char value1[3][2][BIG_INFO_VALUE] T6_AT("", "value1");  // Info_ValueForKey's results

// 0078DB00  universal/q_shared.cpp:139
void TRACK_q_shared()
{
	track_static_alloc_internal(va_info, sizeof(va_info), "va_info", TRACK_MISC);
	track_static_alloc_internal(g_com_error, sizeof(g_com_error), "g_com_error", TRACK_MISC);
	track_static_alloc_internal(g_traceThreadInfo, sizeof(g_traceThreadInfo), "g_traceThreadInfo", TRACK_MISC);
	track_static_alloc_internal(value1, sizeof(value1), "value1", TRACK_MISC);
}

// 0078DB60  universal/q_shared.cpp:157
unsigned char ColorIndex(unsigned char c)
{
	unsigned char index = c - '0';
	if (index < 10)
		return index;
	return 7;
}

// 0078DB80  universal/q_shared.cpp:166
const char *Com_GetFilenameSubString(const char *pathname)
{
	const char *last = pathname;
	while (*pathname)
	{
		if (*pathname == '/' || *pathname == '\\')
			last = pathname + 1;
		++pathname;
	}
	return last;
}

// 0078DBB0  universal/q_shared.cpp:215
const char *Com_GetExtensionSubString(const char *filename)
{
	assert_at(218, filename);
	const char *substr = 0;
	while (*filename)
	{
		if (*filename == '.')
			substr = filename;
		else if (*filename == '/' || *filename == '\\')
			substr = 0;
		++filename;
	}
	if (!substr)
		substr = filename;
	return substr;
}

// 0078DC10  universal/q_shared.cpp:241
void Com_StripExtension(const char *in, char *out)
{
	const char *extension = Com_GetExtensionSubString(in);
	while (in != extension)
		*out++ = *in++;
	*out = 0;
}

// 007902C0  universal/q_shared.cpp:274
// An empty path reads path[-1].
void Com_DefaultExtension(char *path, int maxSize, const char *extension)
{
	char oldPath[64];
	const char *src = path + strlen(path) - 1;
	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;  // it has an extension
		--src;
	}
	I_strncpyz(oldPath, path, sizeof(oldPath));
	Com_sprintf(path, maxSize, "%s%s", oldPath, extension);
}

// Byte order: Swap_Init points these at the swapping or the plain versions.
T6_STATIC short (*_BigShort)(short) T6_AT("", "_BigShort");
T6_STATIC short (*_LittleShort)(short) T6_AT("", "_LittleShort");
T6_STATIC int (*_BigLong)(int) T6_AT("", "_BigLong");
T6_STATIC int (*_LittleLong)(int) T6_AT("", "_LittleLong");
T6_STATIC unsigned __int64 (*_BigLong64)(unsigned __int64) T6_AT("", "_BigLong64");
T6_STATIC unsigned __int64 (*_LittleLong64)(unsigned __int64) T6_AT("", "_LittleLong64");
T6_STATIC float (*_LittleFloatRead)(int) T6_AT("", "_LittleFloatRead");
T6_STATIC int (*_LittleFloatWrite)(float) T6_AT("", "_LittleFloatWrite");

// 0078DC40  universal/q_shared.cpp:318
short BigShort(short l)
{
	return _BigShort(l);
}

// 0078DC50  universal/q_shared.cpp:324
int BigLong(int l)
{
	return _BigLong(l);
}

// 0078DC60  universal/q_shared.cpp:335
unsigned __int64 BigLong64(unsigned __int64 l)
{
	return _BigLong64(l);
}

// 0078DC80  universal/q_shared.cpp:344
short LittleShort(short l)
{
	return _LittleShort(l);
}

// 0078DC90  universal/q_shared.cpp:350
int LittleLong(int l)
{
	return _LittleLong(l);
}

// 0078DCA0  universal/q_shared.cpp:361
int LittleFloatWrite(float l)
{
	return _LittleFloatWrite(l);
}

// 0078DCC0  universal/q_shared.cpp:381
short ShortSwap(short l)
{
	unsigned char b1 = l & 255;
	unsigned char b2 = (l >> 8) & 255;
	return (b1 << 8) + b2;
}

// 0078DCE0  universal/q_shared.cpp:392
short ShortNoSwap(short l)
{
	return l;
}

// 0078DCF0  universal/q_shared.cpp:398
int LongSwap(int l)
{
	unsigned char b1 = l & 255;
	unsigned char b2 = (l >> 8) & 255;
	unsigned char b3 = (l >> 16) & 255;
	unsigned char b4 = (l >> 24) & 255;
	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

// 00834010  universal/q_shared.cpp (folded: the linker merged it with every other `return the argument`
// function, so the MAP lists it at node_pos's address in cscr_parsetree.obj)
int LongNoSwap(int l)
{
	return l;
}

// 0078DD20  universal/q_shared.cpp:417
unsigned __int64 Long64Swap(unsigned __int64 ll)
{
	unsigned char b1 = ll & 255;
	unsigned char b2 = (ll >> 8) & 255;
	unsigned char b3 = (ll >> 16) & 255;
	unsigned char b4 = (ll >> 24) & 255;
	unsigned char b5 = (ll >> 32) & 255;
	unsigned char b6 = (ll >> 40) & 255;
	unsigned char b7 = (ll >> 48) & 255;
	unsigned char b8 = (ll >> 56) & 255;
	return ((unsigned __int64)b1 << 56) + ((unsigned __int64)b2 << 48) + ((unsigned __int64)b3 << 40) +
	       ((unsigned __int64)b4 << 32) + ((unsigned __int64)b5 << 24) + ((unsigned __int64)b6 << 16) +
	       ((unsigned __int64)b7 << 8) + b8;
}

// 0078DDD0  universal/q_shared.cpp:434
unsigned __int64 Long64NoSwap(unsigned __int64 ll)
{
	return ll;
}

// 00834010  universal/q_shared.cpp (folded, as LongNoSwap)
int FloatWriteNoSwap(float f)
{
	union
	{
		float f;
		int i;
	} dat;
	dat.f = f;
	return dat.i;
}

// 0078DDE0  universal/q_shared.cpp:458
float FloatReadNoSwap(int n)
{
	union
	{
		float f;
		int i;
	} dat;
	dat.i = n;
	return dat.f;
}

// 0078DDF0  universal/q_shared.cpp:531
// The data is in the consoles' byte order (big endian): a console client swaps "big" values, everything else
// reads them as they are.
void Swap_Init()
{
	bool isConsole = Com_IsClientConsole();
	_LittleFloatRead = FloatReadNoSwap;
	_LittleFloatWrite = FloatWriteNoSwap;
	if (isConsole)
	{
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_LittleLong64 = Long64NoSwap;
		_BigLong64 = Long64Swap;
	}
	else
	{
		_BigShort = ShortNoSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongNoSwap;
		_LittleLong64 = Long64NoSwap;
		_BigLong64 = Long64NoSwap;
	}
}

// 0078DE80  universal/q_shared.cpp:569
// Any non-digit gives 0; overflow gives UINT_MAX.
unsigned int I_atoui(const char *str)
{
	if (!str || !*str)
		return 0;
	int len = strlen(str);
	unsigned int value = 0;
	for (int i = 0; i < len; ++i)
	{
		if (str[i] < '0' || str[i] > '9')
			return 0;
		unsigned int next = 10 * value + (unsigned int)(str[i] - '0');
		if (next < value)
			return 0xFFFFFFFF;
		value = next;
	}
	return value;
}

// 0078DEF0  universal/q_shared.cpp:600
__int64 I_atoi64(const char *str)
{
	return _atoi64(str);
}

// 0078DF00  universal/q_shared.cpp:616
bool I_islower(int c)
{
	return c >= 'a' && c <= 'z';
}

// 0078DF20  universal/q_shared.cpp:622
bool I_isupper(int c)
{
	return c >= 'A' && c <= 'Z';
}

// 0078DF40  universal/q_shared.cpp:634
bool I_isdigit(int c)
{
	return c >= '0' && c <= '9';
}

// 0078DF60  universal/q_shared.cpp:640
bool I_isalnum(int c)
{
	return I_islower(c) || I_isupper(c) || I_isdigit(c);
}

// 0078DF90  universal/q_shared.cpp:646
bool I_isforfilename(int c)
{
	return I_isalnum(c) || c == '_' || c == '-';
}

// 0078DFD0  universal/q_shared.cpp:764
// Copies at most destsize characters, then terminates at dest[destsize - 1]. A negative destsize copies to the
// end of src and writes before dest.
void I_strncpyz(char *dest, const char *src, int destsize)
{
	assert_at(765, src);
	assert_at(766, dest);
	assertf_at(767, (destsize >= 1), "(destsize) = %i", destsize);
	if (!dest)
		return;
	if (!src)
	{
		*dest = 0;
		return;
	}
	int count = destsize;
	if (count)
	{
		char *d = dest;
		const char *s = src;
		do
		{
			if (!(*d++ = *s++))
				break;
		} while (--count);
	}
	dest[destsize - 1] = 0;
}

// 0078E090  universal/q_shared.cpp:825
// Compares as unsigned characters; reads one character of each string past the n-th.
int I_strnicmp(const char *s0, const char *s1, int n)
{
	assert_at(828, s0);
	assert_at(829, s1);
	if (!s0 || !s1)
		return s1 - s0;
	int c0;
	do
	{
		c0 = (unsigned char)*s0++;
		int c1 = (unsigned char)*s1++;
		if (!n--)
			return 0;
		if (c0 != c1)
		{
			if (I_isupper(c0))
				c0 += 'a' - 'A';
			if (I_isupper(c1))
				c1 += 'a' - 'A';
			if (c0 != c1)
				return c0 < c1 ? -1 : 1;
		}
	} while (c0);
	return 0;
}

// 0078E150  universal/q_shared.cpp:861
// Compares as signed characters, unlike I_strnicmp.
int I_strncmp(const char *s0, const char *s1, int n)
{
	assert_at(864, s0);
	assert_at(865, s1);
	if (!s0 || !s1)
		return s1 - s0;
	char c0;
	do
	{
		c0 = *s0++;
		char c1 = *s1++;
		if (!n--)
			return 0;
		if (c0 != c1)
			return c0 < c1 ? -1 : 1;
	} while (c0);
	return 0;
}

// 0078E1F0  universal/q_shared.cpp:887
const char *I_stristr(const char *s0, const char *substr)
{
	assert_at(891, s0);
	assert_at(892, substr);
	if (!s0 || !substr)
		return 0;
	for (int s0Char = 0; s0[s0Char]; ++s0Char)
	{
		for (int i = 0;; ++i)
		{
			if (!substr[i])
				return &s0[s0Char];
			if (tolower(s0[s0Char + i]) != tolower(substr[i]))
				break;
		}
	}
	return 0;
}

// 0078E2C0  universal/q_shared.cpp:914
int I_stricmp(const char *s0, const char *s1)
{
	assert_at(915, s0);
	assert_at(916, s1);
	return I_strnicmp(s0, s1, 0x7FFFFFFF);
}

// 0078E330  universal/q_shared.cpp:923
int I_strcmp(const char *s0, const char *s1)
{
	assert_at(924, s0);
	assert_at(925, s1);
	return I_strncmp(s0, s1, 0x7FFFFFFF);
}

// 0078E3A0  universal/q_shared.cpp:931
// '*' matches any run of characters, '?' any one character. A '?' facing the end of s steps past it and reads on.
int I_stricmpwild(const char *wild, const char *s)
{
	assert_at(936, wild);
	assert_at(937, s);
	for (;;)
	{
		char charWild = *wild++;
		if (charWild == '*')
		{
			if (!*wild)
				return 0;  // a trailing '*' matches the rest
			if (!*s)
				continue;
			if (!I_stricmpwild(wild - 1, s + 1))
				return 0;
		}
		else
		{
			char charRef = *s++;
			if (charWild != charRef && charWild != '?')
			{
				int lowerRef = tolower(charRef);
				int lowerWild = tolower(charWild);
				if (lowerWild != lowerRef)
					return lowerWild - lowerRef >= 0 ? 1 : -1;
			}
			if (!charWild)
				return 0;
		}
	}
}

// 0078E470  universal/q_shared.cpp:969
char *I_strlwr(char *s)
{
	char *start = s;
	while (*s)
	{
		if (I_isupper(*s))
			*s += 'a' - 'A';
		++s;
	}
	return start;
}

// 0078E4A0  universal/q_shared.cpp:984
char *I_strupr(char *s)
{
	char *start = s;
	while (*s)
	{
		if (I_islower(*s))
			*s -= 'a' - 'A';
		++s;
	}
	return start;
}

// 0078E4D0  universal/q_shared.cpp:999
int I_iscsym(int c)
{
	return I_isalnum(c) || c == '_';
}

// 0078E510  universal/q_shared.cpp:1011
void I_strncat(char *dest, int size, const char *src)
{
	assert_at(1014, size != sizeof( char * ));
	int destLen = strlen(dest);
	if (destLen >= size)
		Com_Error(ERR_FATAL, "\x15" "I_strncat: already overflowed");
	I_strncpyz(dest + destLen, src, size - destLen);
}

// A colour code: '^' and a digit.
static bool I_IsColorString(const char *p)
{
	return *p == '^' && p[1] && p[1] != '^' && p[1] >= '0' && p[1] <= '9';
}

// 0078E590  universal/q_shared.cpp:1025
// The length as drawn: without colour codes.
int I_DrawStrlen(const char *str)
{
	int count = 0;
	while (*str)
	{
		if (I_IsColorString(str))
		{
			str += 2;
		}
		else
		{
			++count;
			++str;
		}
	}
	return count;
}

// 0078E5D0  universal/q_shared.cpp:1046
// '^' and any character, or "[{".
bool I_IsSpecialToken(const char *buf)
{
	if (!buf)
		return false;
	return (*buf == '^' && buf[1]) || (*buf == '[' && buf[1] == '{');
}

// 0078E600  universal/q_shared.cpp:1078
// How far a special token reaches through a run of '^' and '[' (0: none).
int I_IsSpecialTokenRecursive(const char *buf)
{
	if (!buf)
		return 0;
	if (I_IsSpecialToken(buf))
		return 2;
	if (*buf == '^' || *buf == '[')
	{
		int length = I_IsSpecialTokenRecursive(buf + 1);
		if (length)
			return length + 1;
	}
	return 0;
}

// 0078E600  universal/q_shared.cpp:1078 (folded with I_IsSpecialTokenRecursive: inlining it made them identical)
int I_IsSpecialTokenString(const char *buf)
{
	return I_IsSpecialTokenRecursive(buf);
}

// 0078E650  universal/q_shared.cpp:1131
// Drops special tokens and anything outside printable ASCII, until a pass finds no more tokens.
char *I_CleanChat(char *string)
{
	bool modified;
	do
	{
		modified = false;
		char *d = string;
		for (const char *s = string; *s; ++s)
		{
			if (I_IsSpecialToken(s))
			{
				++s;
				modified = true;
			}
			else if (*s >= ' ' && *s <= '~')
			{
				*d++ = *s;
			}
		}
		*d = 0;
	} while (modified);
	return string;
}

// 0078E6B0  universal/q_shared.cpp:1162
// I_CleanChat with colour codes in place of special tokens.
char *I_CleanStr(char *string)
{
	bool modified;
	do
	{
		modified = false;
		char *d = string;
		for (const char *s = string; *s; ++s)
		{
			if (I_IsColorString(s))
			{
				++s;
				modified = true;
			}
			else if (*s >= ' ' && *s <= '~')
			{
				*d++ = *s;
			}
		}
		*d = 0;
	} while (modified);
	return string;
}

// 0078E720  universal/q_shared.cpp:1195
// Windows-1252's right single quotation mark becomes an apostrophe.
char I_CleanChar(char character)
{
	if (character == (char)0x92)
		return '\'';
	return character;
}

// 0078E730  universal/q_shared.cpp:1206
int Com_sprintf(char *dest, int size, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	int len = _vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);
	dest[size - 1] = 0;
	return len;
}

// 0078E760  universal/q_shared.cpp:1221
// Appends at *destPos and advances it; a full buffer leaves *destPos at destSize - 1.
int Com_sprintfPos(char *dest, int destSize, int *destPos, const char *fmt, ...)
{
	if (*destPos >= destSize - 1)
		return -1;
	char *destWrite = dest + *destPos;
	int destRemaining = destSize - *destPos;
	va_list argptr;
	va_start(argptr, fmt);
	int len = _vsnprintf(destWrite, destRemaining, fmt, argptr);
	va_end(argptr);
	destWrite[destRemaining - 1] = 0;
	if (len == destRemaining || len == -1)
		*destPos = destSize - 1;
	else
		*destPos += len;
	return len;
}

// 0078E7C0  universal/q_shared.cpp:1248
// False for a string on the stack (up to 8 KB above this frame) or in this thread's va() buffers: those don't
// live long enough to keep a pointer to.
int CanKeepStringPointer(const char *string)
{
	char stack;
	if (string >= &stack && string < &stack + 0x2000)
		return 0;
	const va_info_t *info = (const va_info_t *)Sys_GetValue(1);
	if (string >= info->va_string[0] && string <= &info->va_string[3][1023])
		return 0;
	return 1;
}

// 0078E810  universal/q_shared.cpp:1286
char *I_itoa(int value, char *buf, int bufsize)
{
	assert_at(1289, bufsize >= 12);
	if (bufsize < 12)
	{
		buf[0] = '0';
		buf[1] = 0;
		return buf;
	}
	unsigned int magnitude = value < 0 ? 0u - (unsigned int)value : (unsigned int)value;
	char *end = buf;
	do
	{
		*end++ = '0' + magnitude % 10;
		magnitude /= 10;
	} while (magnitude);
	if (value < 0)
		*end++ = '-';
	*end = 0;
	char *start = buf;
	for (--end; start < end; ++start, --end)
	{
		char c = *end;
		*end = *start;
		*start = c;
	}
	return buf;
}

// 0078E8C0  universal/q_shared.cpp:1403
// Formats into the next of this thread's four 1 KB buffers.
char *va(const char *format, ...)
{
	va_info_t *info = (va_info_t *)Sys_GetValue(1);
	int index = info->index;
	info->index = (info->index + 1) % 4;
	char *buf = info->va_string[index];
	va_list argptr;
	va_start(argptr, format);
	int len = _vsnprintf(buf, sizeof(info->va_string[0]), format, argptr);
	va_end(argptr);
	buf[sizeof(info->va_string[0]) - 1] = 0;
	if (len < 0 || len >= (int)sizeof(info->va_string[0]))
	{
		buf[sizeof(info->va_string[0]) - 1] = 0;
		Com_Error(ERR_DROP, "\x15" "Attempted to overrun string in call to va(): '%s'", buf);
	}
	return buf;
}

// 0078E940  universal/q_shared.cpp:1437
void Com_InitThreadData(int threadContext)
{
	Sys_SetValue(1, &va_info[threadContext]);
	Sys_SetValue(2, &g_com_error[threadContext]);
	Sys_SetValue(3, &g_traceThreadInfo[threadContext]);
	if (threadContext == 1)
		Sys_SetValue(4, &g_cmd_args[1]);
	else
		Sys_SetValue(4, &g_cmd_args[0]);
}

// 0078E9B0  universal/q_shared.cpp:1523
// Returns the value for key (case-insensitive) in a "\key\value\key\value" string, or "". The result lives in one
// of two buffers per thread (main, render, server), alternating.
const char *Info_ValueForKey(const char *s, const char *key)
{
	T6_STATIC int valueindex T6_AT("Info_ValueForKey", "valueindex");
	char pkey[BIG_INFO_KEY];

	if (!s || !key)
		return "";
	valueindex = (valueindex + 1) % 2;
	if (*s == '\\')
		++s;
	for (;;)
	{
		char *o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
			if (o - pkey >= BIG_INFO_KEY)
				Com_Error(ERR_DROP, "\x15" "Info_ValueForKey: oversize key %d", o - pkey);
		}
		*o = 0;
		++s;

		char *value;
		if (Sys_IsMainThread())
		{
			value = value1[0][valueindex];
		}
		else if (Sys_IsServerThread())
		{
			value = value1[2][valueindex];
		}
		else
		{
			assert_at(1572, Sys_IsRenderThread());
			value = value1[1][valueindex];
		}
		o = value;
		while (*s != '\\' && *s)
		{
			*o++ = *s++;
			if (o - value >= BIG_INFO_VALUE)
				Com_Error(ERR_DROP, "\x15" "Info_ValueForKey: oversize key %d", o - value);  // sic: "key"
		}
		*o = 0;

		if (!I_stricmp(key, pkey))
			return value;
		if (!*s)
			return "";
		++s;
	}
}

// 0078EB60  universal/q_shared.cpp:1615
// Splits off the next key and value and advances *head. No length limits.
void Info_NextPair(const char **head, char *key, char *value)
{
	const char *s = *head;
	if (*s == '\\')
		++s;
	key[0] = 0;
	value[0] = 0;
	char *o = key;
	while (*s != '\\')
	{
		if (!*s)
		{
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	++s;
	o = value;
	while (*s != '\\' && *s)
		*o++ = *s++;
	*o = 0;
	*head = s;
}

// 0078EBC0  universal/q_shared.cpp:1659
// Removes key (case-sensitive) by copying the rest of the string down over it: an overlapping strcpy that works
// because VC100 expands it into a forward byte copy.
//
// The original's source also copies each value into a local buffer it never reads (as Quake III's does); the
// compiler removed that copy, so values of any length are only skipped. Written that way here: a copy would
// overflow the buffer in Info_RemoveKey_Big, where the original never writes it.
void Info_RemoveKey(char *s, const char *key)
{
	char pkey[MAX_INFO_STRING];

	if (strlen(s) >= MAX_INFO_STRING)
		Com_Error(ERR_DROP, "\x15" "Info_RemoveKey: oversize infostring");
	if (strchr(key, '\\'))
		return;
	for (;;)
	{
		char *start = s;
		if (*s == '\\')
			++s;
		char *o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		++s;
		while (*s != '\\' && *s)  // the value (see above)
			++s;

		if (!strcmp(key, pkey))
		{
			strcpy(start, s);
			return;
		}
		if (!*s)
			return;
	}
}

// 0078ECD0  universal/q_shared.cpp:1717
// Info_RemoveKey for strings up to BIG_INFO_STRING, through a key buffer of only BIG_INFO_KEY (B0005).
void Info_RemoveKey_Big(char *s, const char *key)
{
	char pkey[BIG_INFO_KEY];

	if (strlen(s) >= BIG_INFO_STRING)
		Com_Error(ERR_DROP, "\x15" "Info_RemoveKey_Big: oversize infostring");
	if (strchr(key, '\\'))
		return;
	for (;;)
	{
		char *start = s;
		if (*s == '\\')
			++s;
		char *o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		++s;
		while (*s != '\\' && *s)  // the value (see above)
			++s;

		if (!strcmp(key, pkey))
		{
			strcpy(start, s);
			return;
		}
		if (!*s)
			return;
	}
}

// 0078EDF0  universal/q_shared.cpp:1778
// Info strings can't hold '"' or ';'.
int Info_Validate(const char *s)
{
	if (strchr(s, '"'))
		return 0;
	if (strchr(s, ';'))
		return 0;
	return 1;
}

// 0078EE20  universal/q_shared.cpp:1799
// Sets key (removing any old value) with '\\', ';' and '"' dropped from the value. An empty value only removes it.
// The length check lets s grow to MAX_INFO_STRING characters plus the terminator (B0004).
void Info_SetValueForKey(char *s, const char *key, const char *value)
{
	char newi[MAX_INFO_STRING];
	char cleanValue[MAX_INFO_STRING];

	assert_at(1807, value);
	if (strlen(s) >= MAX_INFO_STRING)
	{
		assertfail_at(1811, "Info_SetValueForKey: oversize infostring");
		Com_Printf(10, "\x15" "Info_SetValueForKey: oversize infostring");
		return;
	}

	int j = 0;
	for (int i = 0; i < MAX_INFO_STRING - 1; ++i)
	{
		if (!value[i])
			break;
		if (value[i] != '\\' && value[i] != ';' && value[i] != '"')
		{
			assert_at(1824, j < 1024);
			cleanValue[j++] = value[i];
		}
	}
	assert_at(1829, j < 1024);
	cleanValue[j] = 0;

	if (strchr(key, '\\'))
	{
		assertfail_at(1834, va("Can't use keys with a \\\nkey: '%s'\nvalue: '%s'", key, value));
		Com_Printf(10, "\x15" "Can't use keys with a \\\nkey: '%s'\nvalue: '%s'", key, value);
		return;
	}
	if (strchr(key, ';'))
	{
		assertfail_at(1841, va("Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'", key, value));
		Com_Printf(10, "\x15" "Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'", key, value);
		return;
	}
	if (strchr(key, '"'))
	{
		assertfail_at(1848, va("Can't use keys with a \"\nkey: '%s'\nvalue: '%s'", key, value));
		Com_Printf(10, "\x15" "Can't use keys with a \"\nkey: '%s'\nvalue: '%s'", key, value);
		return;
	}

	Info_RemoveKey(s, key);
	if (!cleanValue[0])
		return;
	if (Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, cleanValue) <= 0)
	{
		assertfail_at(1861, "Info buffer length exceeded, not including key/value pair in response\n");
		Com_Printf(10, "\x15" "Info buffer length exceeded, not including key/value pair in response\n");
		return;
	}
	if (strlen(newi) + strlen(s) > MAX_INFO_STRING)
	{
		assertfail_at(1868, va("Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string:\n%s\n", key,
		                       value, s));
		Com_Printf(10, "\x15" "Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string:\n%s\n", key, value,
		           s);
		return;
	}
	strcat(s, newi);
}

// 0078F1E0  universal/q_shared.cpp:1885
// Info_SetValueForKey for BIG_INFO_STRING, printing instead of asserting (same off-by-one: B0004).
void Info_SetValueForKey_Big(char *s, const char *key, const char *value)
{
	char newi[BIG_INFO_STRING];
	char cleanValue[BIG_INFO_STRING];

	assert_at(1893, value);
	if (strlen(s) >= BIG_INFO_STRING)
	{
		Com_Printf(10, "\x15" "Info_SetValueForKey: oversize infostring");
		return;
	}

	int j = 0;
	for (int i = 0; i < BIG_INFO_STRING - 1; ++i)
	{
		if (!value[i])
			break;
		if (value[i] != '\\' && value[i] != ';' && value[i] != '"')
		{
			assert_at(1909, j < 20480);
			cleanValue[j++] = value[i];
		}
	}
	assert_at(1914, j < 20480);
	cleanValue[j] = 0;

	if (strchr(key, '\\'))
	{
		Com_Printf(10, "\x15" "Can't use keys with a \\\nkey: '%s'\nvalue: '%s'", key, value);
		return;
	}
	if (strchr(key, ';'))
	{
		Com_Printf(10, "\x15" "Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'", key, value);
		return;
	}
	if (strchr(key, '"'))
	{
		Com_Printf(10, "\x15" "Can't use keys with a \"\nkey: '%s'\nvalue: '%s'", key, value);
		return;
	}

	Info_RemoveKey_Big(s, key);
	if (!cleanValue[0])
		return;
	if (Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, cleanValue) <= 0)
	{
		Com_Printf(10, "\x15" "Info buffer length exceeded, not including key/value pair in response\n");
		return;
	}
	if (strlen(newi) + strlen(s) > BIG_INFO_STRING)
	{
		Com_Printf(10, "\x15" "Big info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string:\n%s\n", key,
		           value, s);
		return;
	}
	strcat(s, newi);
}

// 0078F4B0  universal/q_shared.cpp:1994
// The server's link-time code generation gave this function a custom register convention (pField in ESI,
// pStruct in EDI, parseStrCpy in ECX, iMaxFieldTypes in EDX), so it is tested through its callers.
int KeyValueToField(unsigned char *pStruct, const cspField_t *pField, const char *pszKeyValue, int iMaxFieldTypes,
                    int (*parseSpecialFieldType)(unsigned char *, const char *, int, int),
                    void (*parseStrCpy)(unsigned char *, const char *))
{
	char assetName[8192];
	char presetName[245];

	if (pField->iFieldType < CSPFT_NUM_BASE_FIELD_TYPES)
	{
		switch (pField->iFieldType)
		{
		case CSPFT_STRING:
			parseStrCpy(&pStruct[pField->iOffset], pszKeyValue);
			break;
		case CSPFT_STRING_MAX_STRING_CHARS:
			I_strncpyz((char *)&pStruct[pField->iOffset], pszKeyValue, MAX_STRING_CHARS);
			break;
		case CSPFT_STRING_MAX_QPATH:
			I_strncpyz((char *)&pStruct[pField->iOffset], pszKeyValue, MAX_QPATH);
			break;
		case CSPFT_STRING_MAX_OSPATH:
			I_strncpyz((char *)&pStruct[pField->iOffset], pszKeyValue, MAX_OSPATH);
			break;
		case CSPFT_INT:
			*(int *)&pStruct[pField->iOffset] = atoi(pszKeyValue);
			break;
		case CSPFT_UINT:
			*(unsigned int *)&pStruct[pField->iOffset] = I_atoui(pszKeyValue);
			break;
		case CSPFT_BOOL:
			*(bool *)&pStruct[pField->iOffset] = atoi(pszKeyValue) != 0;
			break;
		case CSPFT_QBOOLEAN:
			*(int *)&pStruct[pField->iOffset] = atoi(pszKeyValue) != 0;
			break;
		case CSPFT_FLOAT:
			*(float *)&pStruct[pField->iOffset] = (float)atof(pszKeyValue);
			break;
		case CSPFT_MILLISECONDS:
			*(int *)&pStruct[pField->iOffset] = (int)(atof(pszKeyValue) * 1000.0);
			break;
		case CSPFT_FX:
		case CSPFT_MATERIAL:
		case CSPFT_MATERIAL_STREAM:
			break;
		case CSPFT_XMODEL:
			I_strncpyz(assetName, pszKeyValue, sizeof(assetName));
			*(XModel **)&pStruct[pField->iOffset] = R_RegisterModel(assetName);
			if (!*(XModel **)&pStruct[pField->iOffset])
				return 0;
			break;
		case CSPFT_PHYS_PRESET:
			I_strncpyz(presetName, pszKeyValue, sizeof(presetName));
			*(PhysPreset **)&pStruct[pField->iOffset] = PhysPreset_Register(presetName);
			break;
		case CSPFT_SCRIPT_STRING:
			*(unsigned short *)&pStruct[pField->iOffset] = SL_GetLowercaseString(pszKeyValue, 0);
			break;
		case CSPFT_TRACER:
			I_strncpyz(assetName, pszKeyValue, sizeof(assetName));
			*(TracerDef **)&pStruct[pField->iOffset] = Tracer_Register(assetName);
			break;
		case CSPFT_SOUND_ALIAS_ID:
			*(unsigned int *)&pStruct[pField->iOffset] = SND_HashName(pszKeyValue);
			break;
		default:
			if (pField->iFieldType < 0)
				assertfail_at(2083, va("Negative field type %i given to ParseConfigStringToStruct\n",
				                       pField->iFieldType));
			else
				assertfail_at(2085,
				              "ParseConfigStringToStruct is out of sync with the csParseFieldType_t enum list\n");
			break;
		}
		return 1;
	}
	if (iMaxFieldTypes <= 0 || pField->iFieldType >= iMaxFieldTypes)
	{
		assertfail_at(2098, va("Bad field type %i\n", pField->iFieldType));
		Com_Error(ERR_DROP, "\x15" "Bad field type %i\n", pField->iFieldType);
		return 0;
	}
	assert_at(2092, parseSpecialFieldType != 0);
	if (!parseSpecialFieldType(pStruct, pszKeyValue, pField->iFieldType, pField->iOffset))
		return 0;
	return 1;
}

// 0078F8E0  universal/q_shared.cpp:2115
// Fills the fields named in an info string; false if any field failed.
int ParseConfigStringToStruct(unsigned char *pStruct, const cspField_t *pFieldList, int iNumFields,
                              const char *pszBuffer, int iMaxFieldTypes,
                              int (*parseSpecialFieldType)(unsigned char *, const char *, int, int),
                              void (*parseStrCpy)(unsigned char *, const char *))
{
	bool error = false;
	const cspField_t *pField = pFieldList;
	for (int fieldNum = 0; fieldNum < iNumFields; ++fieldNum, ++pField)
	{
		const char *pszKeyValue = Info_ValueForKey(pszBuffer, pField->szName);
		if (*pszKeyValue)
			error |= !KeyValueToField(pStruct, pField, pszKeyValue, iMaxFieldTypes, parseSpecialFieldType,
			                          parseStrCpy);
	}
	return !error;
}

// 0078F950  universal/q_shared.cpp:2142
// Three-way merge of a field from pszBuffer[0] (base), [1] (left) and [2] (right): a side that changed wins,
// both changing differently goes to parseMergeSpecialCase. The merged value goes into pszMergedBuffer and the
// struct.
int ParseConfigStringToStructMerged(unsigned char *pStruct, const cspField_t *pFieldList, int iNumFields,
                                    const char *mergedName, const char *pszBuffer[], const char *sourceName[],
                                    char *pszMergedBuffer, int iMaxFieldTypes,
                                    int (*parseSpecialFieldType)(unsigned char *, const char *, int, int),
                                    void (*parseStrCpy)(unsigned char *, const char *),
                                    int (*parseMergeSpecialCase)(const char *, char *[], char *, int))
{
	char value0[8192];
	char value1[8192];
	char value2[8192];
	char *value[3];
	value[0] = value0;
	value[1] = value1;
	value[2] = value2;

	bool error = false;
	const cspField_t *pField = pFieldList;
	for (int fieldNum = 0; fieldNum < iNumFields; ++fieldNum, ++pField)
	{
		for (int i = 0; i < 3; ++i)
			I_strncpyz(value[i], Info_ValueForKey(pszBuffer[i], pField->szName), 8192);

		const char *merged;
		if (!strcmp(value[1], value[2]))
		{
			merged = value[1];
		}
		else if (!strcmp(value[0], value[2]) && strcmp(value[0], value[1]))
		{
			merged = value[1];
		}
		else if (!strcmp(value[0], value[1]) && strcmp(value[0], value[2]))
		{
			merged = value[2];
		}
		else
		{
			if (!parseMergeSpecialCase(pField->szName, value, value[0], 8192))
			{
				Com_PrintError(1,
				               "ERROR: Could not merge field '%s' for merged asset '%s'.\nBase: %s -> %s\nLeft: %s -> "
				               "%s\nRight: %s -> %s\n",
				               pField->szName, mergedName, sourceName[0], value[0], sourceName[1], value[1],
				               sourceName[2], value[2]);
				error = true;
			}
			merged = value[0];
		}

		if (*merged)
		{
			Info_SetValueForKey_Big(pszMergedBuffer, pField->szName, merged);
			error |= !KeyValueToField(pStruct, pField, merged, iMaxFieldTypes, parseSpecialFieldType, parseStrCpy);
		}
	}
	return !error;
}

// 0078FCB0  universal/q_shared.cpp:2247
// Leaning eases out: 2f - f*f for f in [0, 1], odd in f. The product stays unrounded on the x87 stack.
X87_MATH float GetLeanFraction(float fFrac)
{
	return (2.0f - fabsf(fFrac)) * fFrac;
}

// 0078FCD0  universal/q_shared.cpp:2260
// Not GetLeanFraction's inverse: anything below 1 is first moved down by 0.8 (B0006).
X87_MATH float UnGetLeanFraction(float fFrac)
{
	assert_at(2261, fFrac >= 0);
	assert_at(2262, fFrac <= 1.f);
	if (fFrac < 1.0f)
		fFrac -= 0.8f;
	// The original returns 0 here (fldz). Written as 1 - sqrt(1 - 0), which is the same +0, so the x87 result is
	// computed in the returning block: see CMD_GetAnalogButtonValue.
	if (fFrac <= 0.0f)
		fFrac = 0.0f;
	return 1.0f - sqrtf(1.0f - fFrac);
}

// 0078FD80  universal/q_shared.cpp:2288
void AddLeanToPosition(vec3_t &position, float fViewYaw, float fLeanFrac, float fViewRoll, float fLeanDist)
{
	if (fLeanFrac != 0.0f)
	{
		// GetLeanFraction, inlined: here the arithmetic is SSE, rounded to float at each step
		float fLean = (2.0f - fabsf(fLeanFrac)) * fLeanFrac;
		vec3_t vAng;
		vAng.x = 0.0f;
		vAng.y = fViewYaw;
		vAng.z = fLean * fViewRoll;
		vec3_t vRight;
		AngleVectors(vAng, 0, &vRight, 0);
		float scale = fLean * fLeanDist;
		position.x = vRight.x * scale + position.x;
		position.y = vRight.y * scale + position.y;
		position.z = vRight.z * scale + position.z;
	}
}

// Inlined into OrientationConcatenate; not in the symbols, so the names are descriptive.
// pos transformed into orient's frame (universal/q_shared.cpp:2320)
static void OrientationTransformPos(const vec3_t &pos, const orientation_t *orient, vec3_t &out)
{
	assert_at(2320, &pos != &out);
	out.x = pos.x * orient->axis[0].x + orient->origin.x + pos.y * orient->axis[1].x + pos.z * orient->axis[2].x;
	out.y = pos.x * orient->axis[0].y + orient->origin.y + pos.y * orient->axis[1].y + pos.z * orient->axis[2].y;
	out.z = pos.x * orient->axis[0].z + orient->origin.z + pos.y * orient->axis[1].z + pos.z * orient->axis[2].z;
}

// dir rotated by orient's axes (universal/q_shared.cpp:2329)
static void OrientationTransformDir(const vec3_t &dir, const orientation_t *orient, vec3_t &out)
{
	assert_at(2329, &dir != &out);
	out.x = dir.y * orient->axis[1].x + dir.x * orient->axis[0].x + dir.z * orient->axis[2].x;
	out.y = dir.y * orient->axis[1].y + dir.x * orient->axis[0].y + dir.z * orient->axis[2].y;
	out.z = dir.y * orient->axis[1].z + dir.x * orient->axis[0].z + dir.z * orient->axis[2].z;
}

// 0078FE50  universal/q_shared.cpp:2398
// orFirst expressed in orSecond's frame, then orSecond's frame in the world.
void OrientationConcatenate(const orientation_t *orFirst, const orientation_t *orSecond, orientation_t *out)
{
	assert_at(2399, out != orFirst);
	assert_at(2400, out != orSecond);
	OrientationTransformDir(orFirst->axis[0], orSecond, out->axis[0]);
	OrientationTransformDir(orFirst->axis[1], orSecond, out->axis[1]);
	OrientationTransformDir(orFirst->axis[2], orSecond, out->axis[2]);
	OrientationTransformPos(orFirst->origin, orSecond, out->origin);
}

// 00790180  universal/q_shared.cpp:2410
// The axes are transposed through the x87 stack (fld / fstp), the origin in SSE.
void OrientationInvert(const orientation_t *orient, orientation_t *out)
{
	T6_X87_COPY(out->axis[0].x, orient->axis[0].x);
	T6_X87_COPY(out->axis[0].y, orient->axis[1].x);
	T6_X87_COPY(out->axis[0].z, orient->axis[2].x);
	T6_X87_COPY(out->axis[1].x, orient->axis[0].y);
	T6_X87_COPY(out->axis[1].y, orient->axis[1].y);
	T6_X87_COPY(out->axis[1].z, orient->axis[2].y);
	T6_X87_COPY(out->axis[2].x, orient->axis[0].z);
	T6_X87_COPY(out->axis[2].y, orient->axis[1].z);
	T6_X87_COPY(out->axis[2].z, orient->axis[2].z);
	vec3_t negOrigin;
	negOrigin.x = vec3_origin.x - orient->origin.x;
	negOrigin.y = vec3_origin.y - orient->origin.y;
	negOrigin.z = vec3_origin.z - orient->origin.z;
	out->origin.x = out->axis[0].x * negOrigin.x + negOrigin.y * out->axis[1].x + out->axis[2].x * negOrigin.z;
	out->origin.y = out->axis[0].y * negOrigin.x + out->axis[1].y * negOrigin.y + negOrigin.z * out->axis[2].y;
	out->origin.z = out->axis[0].z * negOrigin.x + out->axis[1].z * negOrigin.y + out->axis[2].z * negOrigin.z;
}

// 00790280  universal/q_shared.cpp:2530
int Com_IsLegacyXModelName(const char *name)
{
	return !I_strnicmp(name, "xmodel", 6) && (name[6] == '/' || name[6] == '\\');
}

// 00790340  universal/q_shared.cpp:2537
// Button 19 is analog: three bits (19-21), as sevenths; any other button is 0 or 1.
//
// Written for the reconstruction's compiler: the original returns 1.0 / 0.0 (fld1 / fldz) for a plain button and
// the analog product unrounded on the x87 stack. clang at -O0 spills an x87 value that crosses a branch to a float
// slot, which would round the product, so both paths end in the one multiplication (value * 1.0 is exact).
X87_MATH float CMD_GetAnalogButtonValue(usercmd_s *cmd, int whichBit)
{
	int value;
	float scale;
	if (whichBit == 19)
	{
		value = 0;
		for (int i = 0; i < 3; ++i)
		{
			if (cmd->button_bits.testBit(19 + i))
				value |= 1 << i;
		}
		scale = 0.14285715f;
	}
	else
	{
		value = cmd->button_bits.testBit(whichBit);
		scale = 1.0f;
	}
	return value * scale;
}
