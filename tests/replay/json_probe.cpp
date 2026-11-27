// Replay probe: json/json.cpp's jsmn_parse, JSMN_GetToken and JSMN_GetLocalizedString against the Store MP
// original (JSMN_GetKey and JSMN_UnGetToken are inlined there). verify_json.py drives it.
#include "json/json.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define MS __attribute__((ms_abi))

// The reconstruction's session (the original's is at 143ECFFA0 on)
const char *s_json;
jsmntok_t *s_tokens;
int s_currToken;
int s_numTokens;
jsmn_parser s_parser;
jsmnVal_u s_val;

namespace
{
constexpr unsigned CURR = 0x3ECFFA0, NUM = 0x3ECFFA4, TOKENS = 0x3ECFFA8, JSON = 0x3ECFFB0, VAL = 0x3ECFFC0;
unsigned state;
unsigned Next() { return state = state * 1664525u + 1013904223u; }

char text[4096];
jsmntok_t tokens[160];
char translated[64];

struct Out
{
	unsigned char bytes[0x2000];
	unsigned n;
	void Put(const void *p, unsigned len)
	{
		if (n + len <= sizeof(bytes))
			std::memcpy(bytes + n, p, len);
		n += len;
	}
	template <class T> void Put(T v) { Put(&v, sizeof(v)); }
};

const char *Translate(const char *ref)
{
	std::strcpy(translated, "tr:");
	std::strncat(translated, ref, 40);
	return translated;
}

// Random JSON-like text: mostly well formed objects and arrays with every token kind, some of it broken.
void Gen(int depth, unsigned &at)
{
	auto put = [&](const char *s) {
		while (*s && at < sizeof(text) - 64)
			text[at++] = *s++;
	};
	static const char *const keys[] = {"version", "title", "loc", "english", "noloc", "ref", "ENGLISH", "x", "",
	                                   "a\\\"b", "vis", "data"};
	static const char *const prims[] = {"1", "-17", "0", "true", "false", "null", "tru", "f", "3.5", "99999999999",
	                                    "\x7f", "1e5"};
	static const char *const junk[] = {"}", "]", ":", ",", "\"", "\\", "{", "[", "\x01", " ", "\t\r\n", "\"\\q\""};
	const unsigned r = Next() % 16;
	if (r < 2 || depth > 3)
	{
		put(prims[Next() % 12]);
		return;
	}
	if (r < 5)
	{
		put("\"");
		const unsigned len = Next() % 6 ? Next() % 12 : 1100 + Next() % 200;
		for (unsigned i = 0; i < len; ++i)
			put(i % 7 == 3 && Next() % 4 == 0 ? "\\n" : "y");
		put("\"");
		return;
	}
	if (r == 5)
	{
		put(junk[Next() % 12]);
		return;
	}
	const bool object = r < 11;
	put(object ? "{" : "[");
	const unsigned count = Next() % 5;
	for (unsigned i = 0; i < count; ++i)
	{
		if (i)
			put(Next() % 9 ? "," : " , ");
		if (object)
		{
			put("\"");
			put(keys[Next() % 12]);
			put(Next() % 7 ? "\":" : "\" : ");
		}
		Gen(depth + 1, at);
	}
	if (Next() % 12)
		put(object ? "}" : "]");
}

unsigned MakeText()
{
	unsigned at = 0;
	Gen(0, at);
	if (Next() % 8 == 0)
		Gen(0, at);
	text[at] = 0;
	return at;
}

void FillJunk(jsmntok_t *t, unsigned n)
{
	for (unsigned i = 0; i < n; ++i)
		t[i] = {(jsmntype_t)(Next() % 5), Next() % 3 ? -1 : (int)(Next() % 40), Next() % 3 ? -1 : (int)(Next() % 40),
		        (int)(Next() % 9), nullptr, (jsmnPrimitiveType_e)(Next() % 3)};
}

void PutTokens(Out &o, const jsmntok_t *t, unsigned n, const void *val)
{
	for (unsigned i = 0; i < n; ++i)
	{
		o.Put(t[i].type), o.Put(t[i].start), o.Put(t[i].end), o.Put(t[i].size), o.Put(t[i].primType);
		o.Put((unsigned char)(t[i].val == nullptr ? 0 : t[i].val == val ? 1 : 2));
	}
}

// Parse with the reconstruction (for the reader's input: both sides read the same tokens).
unsigned Tokenize()
{
	jsmn_parser p;
	jsmn_init(&p);
	int err;
	FillJunk(tokens, 160);
	jsmn_parse(&p, text, tokens, 160, err);
	return p.toknext;
}
}  // namespace

// q_shared.cpp's helpers (verified on their own) for the reconstruction: the original's, from the mapped image.
static int(MS *s_stricmp)(const char *, const char *);
static void(MS *s_strncpyz)(char *, const char *, int);
int I_stricmp(const char *a, const char *b) { return s_stricmp(a, b); }
void I_strncpyz(char *dest, const char *src, int destsize) { s_strncpyz(dest, src, destsize); }
extern "C" void JsonInit(unsigned char *image)
{
	s_stricmp = reinterpret_cast<int(MS *)(const char *, const char *)>(image + 0x471C80);
	s_strncpyz = reinterpret_cast<void(MS *)(char *, const char *, int)>(image + 0x471B00);
}

const char *UI_SafeTranslateString(const char *ref) { return Translate(ref); }
static const char *MS HookTranslate(const char *ref) { return Translate(ref); }
static int MS HookAtoi(const char *s) { return std::atoi(s); }
extern "C" void *JsonHook(unsigned i) { return i ? (void *)HookAtoi : (void *)HookTranslate; }

// op 0 jsmn_parse, 1 JSMN_GetToken to the end, 2 JSMN_GetLocalizedString; image null: the reconstruction.
extern "C" unsigned JsonRun(unsigned char *image, unsigned op, unsigned seed, Out *o)
{
	state = seed + 1;
	o->n = 0;
	if (op == 2 && Next() % 8)
	{
		// a translations object, read from its own token (index 2) on most inputs
		static const char *const keys[] = {"english", "french", "noloc", "ref", "ENGLISH", "x", "Noloc", "REF"};
		static const char *const values[] = {"\"Hello\"", "\"\"", "1", "true", "{}", "\"MENU_X\""};
		unsigned at = (unsigned)std::strlen(std::strcpy(text, "{\"loc\":{"));
		const unsigned pairs = Next() % 5;
		for (unsigned i = 0; i < pairs; ++i)
			at += std::sprintf(text + at, "%s\"%s\":%s", i ? "," : "", keys[Next() % 8], values[Next() % 6]);
		std::strcpy(text + at, Next() % 6 ? "}}" : "}");
	}
	else
		MakeText();
	if (op == 0)
	{
		const unsigned n = Next() % 4 ? 1 + Next() % 120 : Next() % 4;
		jsmn_parser p = {Next() % 3 ? 0u : Next() % 8, (int)(Next() % 3 ? 0 : Next() % 4), Next() % 3 ? -1 : 0};
		if (p.toknext > (int)n)
			p.toknext = n;
		FillJunk(tokens, 160);
		int err = 0x5A5A;
		const jsmnerr_t r = image ? reinterpret_cast<jsmnerr_t(MS *)(jsmn_parser *, const char *, jsmntok_t *, unsigned,
		                                                            int *)>(image + 0x34C640)(&p, text, tokens, n, &err)
		                          : jsmn_parse(&p, text, tokens, n, err);
		o->Put(r), o->Put(p), o->Put(err);
		PutTokens(*o, tokens, 160, nullptr);
		return o->n;
	}

	const unsigned count = Tokenize();
	const int curr = Next() % 4 ? (op == 1 ? 0 : (count > 2 ? 2 : 0)) : (int)(Next() % (count + 1));
	const bool noJson = Next() % 32 == 0, noTokens = Next() % 32 == 0;
	void *val;
	if (image)
	{
		*reinterpret_cast<int *>(image + CURR) = curr;
		*reinterpret_cast<int *>(image + NUM) = count;
		*reinterpret_cast<jsmntok_t **>(image + TOKENS) = noTokens ? nullptr : tokens;
		*reinterpret_cast<const char **>(image + JSON) = noJson ? nullptr : text;
		std::memset(image + VAL, 0xCC, sizeof(jsmnVal_u));
		val = image + VAL;
	}
	else
	{
		s_currToken = curr, s_numTokens = count, s_tokens = noTokens ? nullptr : tokens;
		s_json = noJson ? nullptr : text;
		std::memset(&s_val, 0xCC, sizeof(s_val));
		val = &s_val;
	}
	auto session = [&] {
		o->Put(image ? *reinterpret_cast<int *>(image + CURR) : s_currToken);
		o->Put(val, sizeof(jsmnVal_u));
	};
	if (op == 1)
	{
		for (int i = 0; i < 170; ++i)
		{
			jsmntok_t *t = image ? reinterpret_cast<jsmntok_t *(MS *)()>(image + 0x34CBB0)() : JSMN_GetToken();
			o->Put((long long)(t ? t - tokens : -1));
			session();
			if (!t)
				break;
		}
		PutTokens(*o, tokens, 160, val);
		return o->n;
	}
	static const char *const locs[] = {"english", "french", "", "ENGLISH", nullptr};
	const char *loc = locs[Next() % 5];
	const char *s = image ? reinterpret_cast<const char *(MS *)(const char *)>(image + 0x34CD10)(loc)
	                      : JSMN_GetLocalizedString(loc);
	o->Put((unsigned char)(s == val ? 1 : s == translated ? 2 : 0));
	o->Put(s, std::strlen(s) + 1);
	session();
	PutTokens(*o, tokens, 160, val);
	return o->n;
}

extern "C" unsigned JsonOutSize() { return sizeof(Out); }
