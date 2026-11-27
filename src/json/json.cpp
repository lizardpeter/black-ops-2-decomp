// json/json.cpp (CoDMPServer_PC: json.obj; Store MP 14034C640 on)
//
// jsmn, an early version (tokens marked unused by start = end = -1 and found by a scan from toknext; no parent
// links; non-strict: a primitive is anything printable up to a delimiter), with Treyarch's two additions: ':' turns
// the token before it into JSMN_KEY, and the offset of the character being read is kept in errorPos. Then a reader
// that walks one parsed file's tokens in order, copying each string or primitive into one value buffer.
//
// The server build passes jsmn_parse_primitive's and jsmn_parse_string's parser and input in registers (link-time
// code generation); the Store build inlines them, JSMN_BeginParseSession, JSMN_UnGetToken and JSMN_GetKey, and has no
// asserts or warnings.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\json\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "json.cpp"
#define T6_SOURCE "json/json.cpp"

#include "json/json.h"

#include <stdlib.h>

#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "t6_static.h"
#include "universal/q_shared.h"

const char *UI_SafeTranslateString(const char *reference);

// The unused token from toknext on, or none.
static jsmntok_t *jsmn_get_token(jsmn_parser *parser, jsmntok_t *tokens, unsigned int num_tokens)
{
	for (unsigned int i = parser->toknext; i < num_tokens; i++)
	{
		if (tokens[i].start == -1 && tokens[i].end == -1)
		{
			parser->toknext = i + 1;
			return &tokens[i];
		}
	}
	return nullptr;
}

static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type, int start, int end)
{
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}

// 0068C330 json/json.cpp:36
static int jsmn_parse_primitive(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens)
{
	const int start = parser->pos;
	for (; js[parser->pos]; parser->pos++)
	{
		switch (js[parser->pos])
		{
		case '\t':
		case '\r':
		case '\n':
		case ' ':
		case ',':
		case ':':
		case ']':
		case '}':
			goto found;
		}
		if (js[parser->pos] < 32 || js[parser->pos] >= 127)
		{
			parser->pos = start;
			return JSMN_ERROR_INVAL;
		}
	}
found:
	jsmntok_t *token = jsmn_get_token(parser, tokens, num_tokens);
	if (!token)
	{
		parser->pos = start;
		return JSMN_ERROR_NOMEM;
	}
	jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
	parser->pos--;
	return JSMN_SUCCESS;
}

// 0068C480 json/json.cpp:77
static int jsmn_parse_string(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens)
{
	const int start = parser->pos;
	parser->pos++;
	for (; js[parser->pos]; parser->pos++)
	{
		const char c = js[parser->pos];
		if (c == '"')
		{
			jsmntok_t *token = jsmn_get_token(parser, tokens, num_tokens);
			if (!token)
			{
				parser->pos = start;
				return JSMN_ERROR_NOMEM;
			}
			jsmn_fill_token(token, JSMN_STRING, start + 1, parser->pos);
			return JSMN_SUCCESS;
		}
		if (c == '\\')
		{
			parser->pos++;
			switch (js[parser->pos])
			{
			case '"':
			case '/':
			case '\\':
			case 'b':
			case 'f':
			case 'r':
			case 'n':
			case 't':
			case 'u':
				break;
			default:
				parser->pos = start;
				return JSMN_ERROR_INVAL;
			}
		}
	}
	parser->pos = start;
	return JSMN_ERROR_PART;
}

// 0068CB20 json/json.cpp:124
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens)
{
	int errorPos;
	return jsmn_parse(parser, js, tokens, num_tokens, errorPos);
}

// 0068C5D0 json/json.cpp:133
// Store MP 14034C640 (tests/replay/verify_json.py).
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens, int &errorPos)
{
	int i;
	// the tokens not used yet are marked unused (the caller may have grown the array)
	for (i = parser->toknext; i < (int)num_tokens; i++)
		jsmn_fill_token(&tokens[i], JSMN_PRIMITIVE, -1, -1);

	for (; js[parser->pos]; parser->pos++)
	{
		errorPos = parser->pos;
		const char c = js[parser->pos];
		switch (c)
		{
		case '{':
		case '[':
		{
			jsmntok_t *token = jsmn_get_token(parser, tokens, num_tokens);
			if (!token)
				return JSMN_ERROR_NOMEM;
			if (parser->toksuper != -1)
				tokens[parser->toksuper].size++;
			token->type = c == '{' ? JSMN_OBJECT : JSMN_ARRAY;
			token->start = parser->pos;
			parser->toksuper = parser->toknext - 1;
			break;
		}
		case '}':
		case ']':
		{
			const jsmntype_t type = c == '}' ? JSMN_OBJECT : JSMN_ARRAY;
			for (i = parser->toknext - 1; i >= 0; i--)
			{
				jsmntok_t *token = &tokens[i];
				if (token->start != -1 && token->end == -1)
				{
					if (token->type != type)
						return JSMN_ERROR_INVAL;
					parser->toksuper = -1;
					token->end = parser->pos + 1;
					break;
				}
			}
			if (i == -1)  // nothing open to close
				return JSMN_ERROR_INVAL;
			for (; i >= 0; i--)
			{
				if (tokens[i].start != -1 && tokens[i].end == -1)
				{
					parser->toksuper = i;
					break;
				}
			}
			break;
		}
		case '"':
		{
			const int r = jsmn_parse_string(parser, js, tokens, num_tokens);
			if (r < 0)
				return (jsmnerr_t)r;
			if (parser->toksuper != -1)
				tokens[parser->toksuper].size++;
			break;
		}
		case ':':
			tokens[parser->toknext - 1].type = JSMN_KEY;
			break;
		case '\t':
		case '\r':
		case '\n':
		case ',':
		case ' ':
			break;
		default:
		{
			const int r = jsmn_parse_primitive(parser, js, tokens, num_tokens);
			if (r < 0)
				return (jsmnerr_t)r;
			if (parser->toksuper != -1)
				tokens[parser->toksuper].size++;
			break;
		}
		}
	}

	for (i = parser->toknext - 1; i >= 0; i--)
	{
		if (tokens[i].start != -1 && tokens[i].end == -1)  // an object or array never closed
			return JSMN_ERROR_PART;
	}
	return JSMN_SUCCESS;
}

// 0068C850 json/json.cpp:263
void jsmn_init(jsmn_parser *parser)
{
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}

// The session: one file's text, its tokens, and the next to read.
T6_STATIC const char *s_json T6_AT("", "s_json");
T6_STATIC jsmntok_t *s_tokens T6_AT("", "s_tokens");
T6_STATIC int s_currToken T6_AT("", "s_currToken");
T6_STATIC int s_numTokens T6_AT("", "s_numTokens");
T6_STATIC jsmn_parser s_parser T6_AT("", "s_parser");
T6_STATIC jsmnVal_u s_val T6_AT("", "s_val");

// 0068CB50 json/json.cpp:286 (inlined in the Store build)
bool JSMN_BeginParseSession(const char *json, jsmntok_t tokens[], int maxTokens)
{
	s_currToken = 0;
	jsmn_init(&s_parser);
	s_json = json;
	s_tokens = tokens;
	int errorPos;
	const jsmnerr_t r = jsmn_parse(&s_parser, json, tokens, maxTokens, errorPos);
	s_numTokens = s_parser.toknext;
	switch (r)
	{
	case JSMN_ERROR_PART:
		Com_PrintError(10, "The string is not a full JSON packet, more bytes expected.\n");
		return false;
	case JSMN_ERROR_INVAL:
		Com_PrintError(10, "Found invalid characters while parsing and bailed out.\n");
		return false;
	case JSMN_ERROR_NOMEM:
		Com_PrintError(10, "Too many tokens in the file. Exceeded current limit of %d\n", maxTokens);
		return false;
	default:
		return true;
	}
}

// 0068C870 json/json.cpp:315
// Store MP 14034CBB0 (tests/replay/verify_json.py).
// The next token, its text copied into s_val for a key, string or primitive (a primitive also read as a bool, from
// its first letter, or an int); objects and arrays get no value. Null at the end.
jsmntok_t *JSMN_GetToken()
{
	assert_at(316, s_json);
	assert_at(317, s_tokens);
	if (!s_json || !s_tokens || s_currToken == s_numTokens)
		return nullptr;

	jsmntok_t *token = &s_tokens[s_currToken++];
	token->primType = JSMN_PRIMITIVE_TYPE_INVALID;
	if (token->type != JSMN_KEY && token->type != JSMN_STRING && token->type != JSMN_PRIMITIVE)
	{
		token->val = nullptr;
		return token;
	}

	const int len = token->end - token->start + 1;
	token->val = &s_val;
	const int destsize = len < (int)sizeof(s_val.stringVal) ? len : (int)sizeof(s_val.stringVal);
	I_strncpyz(s_val.stringVal, s_json + token->start, destsize);
#ifndef T6_STORE
	if (destsize < len)
		Com_PrintWarning(10, "JSON: Primitive truncated to '%s'\n", s_val.stringVal);
#endif
	if (token->type == JSMN_PRIMITIVE)
	{
		if (!s_val.stringVal[0])
		{
			token->val = nullptr;
			return token;
		}
		if (s_val.stringVal[0] == 't')
		{
			s_val.boolVal = true;
			token->primType = JSMN_PRIMITIVE_TYPE_BOOL;
			return token;
		}
		if (s_val.stringVal[0] == 'f')
		{
			s_val.boolVal = false;
			token->primType = JSMN_PRIMITIVE_TYPE_BOOL;
			return token;
		}
		s_val.intVal = atoi(s_val.stringVal);
		token->primType = JSMN_PRIMITIVE_TYPE_INT;
	}
	return token;
}

// 0068C9E0 json/json.cpp:392 (inlined in the Store build)
void JSMN_UnGetToken()
{
	if (s_currToken)
		s_currToken--;
}

// 0068C9F0 json/json.cpp:403 (inlined in the Store build)
// The next token if it is the key `key` (without case), else null (the token is read either way).
jsmntok_t *JSMN_GetKey(const char *key)
{
	jsmntok_t *token = JSMN_GetToken();
	if (token && token->type == JSMN_KEY && token->val && !I_stricmp(token->val->stringVal, key))
		return token;
	return nullptr;
}

// 0068CA30 json/json.cpp:415
// Store MP 14034CD10 (tests/replay/verify_json.py).
// An object of translations: the value for the language `loc` or "noloc", or the translation of the localized
// string reference under "ref", whichever comes first; "" when none does or the object is malformed.
const char *JSMN_GetLocalizedString(const char *loc)
{
	jsmntok_t *token = JSMN_GetToken();
	if (!token || token->type != JSMN_OBJECT)
		return "";
	const int pairs = token->size / 2;
	for (int i = 0; i < pairs; i++)
	{
		token = JSMN_GetToken();
		if (!token || token->type != JSMN_KEY || !token->val)
			return "";
		if ((loc && !I_stricmp(token->val->stringVal, loc)) || !I_stricmp(token->val->stringVal, "noloc"))
		{
			token = JSMN_GetToken();
			if (token && token->type == JSMN_STRING && token->val)
				return token->val->stringVal;
			return "";
		}
		if (!I_stricmp(token->val->stringVal, "ref"))
		{
			token = JSMN_GetToken();
			if (token && token->type == JSMN_STRING && token->val)
				return UI_SafeTranslateString(token->val->stringVal);
			return "";
		}
		if (!JSMN_GetToken())
			return "";
	}
	return "";
}
