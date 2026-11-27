// json/json.cpp: jsmn (Serge Zaitsev's minimal JSON tokenizer, an early version, with Treyarch's key tokens and error
// position) and a one-file-at-a-time reader over its tokens (JSMN_BeginParseSession, then JSMN_GetToken in order).
#pragma once

enum jsmntype_t : int
{
	JSMN_PRIMITIVE = 0,  // a number, true, false or null
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_KEY = 3,        // a string followed by ':' (Treyarch's)
	JSMN_STRING = 4,
};

enum jsmnerr_t : int
{
	JSMN_ERROR_NOMEM = -1,  // more tokens than the caller gave room for
	JSMN_ERROR_INVAL = -2,  // a character not allowed there
	JSMN_ERROR_PART = -3,   // the text ends inside a string, object or array
	JSMN_SUCCESS = 0,
};

enum jsmnPrimitiveType_e : int
{
	JSMN_PRIMITIVE_TYPE_INVALID = 0,
	JSMN_PRIMITIVE_TYPE_INT = 1,
	JSMN_PRIMITIVE_TYPE_BOOL = 2,
};

// A token's value as JSMN_GetToken leaves it (one buffer, overwritten by the next token).
union jsmnVal_u
{
	char stringVal[1025];
	int intVal;
	bool boolVal;
};

struct jsmntok_t
{
	jsmntype_t type;
	int start;  // the token's text is [start, end) of the input; -1 while not yet filled in
	int end;
	int size;   // an object's or array's children (an object's keys and values both count)
	jsmnVal_u *val;
	jsmnPrimitiveType_e primType;
};
static_assert(sizeof(jsmntok_t) == (sizeof(void *) == 8 ? 0x20 : 0x18), "jsmntok_t");

struct jsmn_parser
{
	unsigned int pos;  // the input offset
	int toknext;       // the next token to use
	int toksuper;      // the open object or array, -1 for none
};
static_assert(sizeof(jsmn_parser) == 0xC, "jsmn_parser");

int jsmn_parse_primitive(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens);
int jsmn_parse_string(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens);
void jsmn_init(jsmn_parser *parser);
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens, int &errorPos);
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens);

bool JSMN_BeginParseSession(const char *json, jsmntok_t tokens[], int maxTokens);
jsmntok_t *JSMN_GetToken();
void JSMN_UnGetToken();
jsmntok_t *JSMN_GetKey(const char *key);
const char *JSMN_GetLocalizedString(const char *loc);
