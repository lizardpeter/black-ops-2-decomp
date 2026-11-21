// universal/q_shared.cpp: types and helpers every module shares.
#pragma once

#include "qcommon/bitarray.h"

union vec2_t
{
	float v[2];
	struct
	{
		float x;
		float y;
	};
};

union vec3_t
{
	struct
	{
		float x;
		float y;
		float z;
	};
	float v[3];
};

union vec4_t
{
	float v[4];
	struct
	{
		float x;
		float y;
		float z;
		float w;
	};
	struct
	{
		float r;
		float g;
		float b;
		float a;
	};
};

struct orientation_t
{
	vec3_t origin;
	vec3_t axis[3];
};

union Weapon
{
	struct
	{
		unsigned int weaponIdx : 8;
		unsigned int attachment1 : 6;
		unsigned int attachment2 : 6;
		unsigned int attachment3 : 6;
		unsigned int padding : 6;
	};
	unsigned int weaponData;
};

struct usercmd_s
{
	int serverTime;
	bitarray<64> button_bits;
	int angles[3];
	Weapon weapon;
	Weapon offHandWeapon;
	Weapon lastWeaponAltModeSwitch;
	char forwardmove;
	char rightmove;
	char upmove;
	char pitchmove;
	char yawmove;
	short gunPitch;
	short gunYaw;
	short wiiumoteAimX;
	short wiiumoteAimY;
	char wiiuControllerType;
	unsigned short meleeChargeEnt;
	unsigned char meleeChargeDist;
	union
	{
		float rollmove;
		short damageKick[2];
	};
	char selectedLocation[2];
	unsigned char selectedYaw;
	unsigned short airburstMarkDistance;
	unsigned short lastInput;
};

enum csParseFieldType_t
{
	CSPFT_STRING = 0,
	CSPFT_STRING_MAX_STRING_CHARS = 1,
	CSPFT_STRING_MAX_QPATH = 2,
	CSPFT_STRING_MAX_OSPATH = 3,
	CSPFT_INT = 4,
	CSPFT_UINT = 5,
	CSPFT_BOOL = 6,
	CSPFT_QBOOLEAN = 7,
	CSPFT_FLOAT = 8,
	CSPFT_MILLISECONDS = 9,
	CSPFT_FX = 10,
	CSPFT_XMODEL = 11,
	CSPFT_MATERIAL = 12,
	CSPFT_MATERIAL_STREAM = 13,
	CSPFT_PHYS_PRESET = 14,
	CSPFT_SCRIPT_STRING = 15,
	CSPFT_TRACER = 16,
	CSPFT_SOUND_ALIAS_ID = 17,
	CSPFT_NUM_BASE_FIELD_TYPES = 18,
};

struct cspField_t
{
	const char *szName;
	int iOffset;
	int iFieldType;
};

struct va_info_t
{
	char va_string[4][1024];
	int index;
};

enum
{
	MAX_STRING_CHARS = 1024,
	MAX_QPATH = 64,
	MAX_OSPATH = 256,
	MAX_INFO_STRING = 1024,
	BIG_INFO_STRING = 20480,
	BIG_INFO_KEY = 8192,
	BIG_INFO_VALUE = 8192,
	THREAD_CONTEXT_COUNT = 17,
};

extern const vec3_t vec3_origin;  // com_vector4_const.cpp

extern const vec4_t colorBlack;
extern const vec4_t colorRed;
extern const vec4_t colorLtRed;
extern const vec4_t colorMdRed;
extern const vec4_t colorDkRed;
extern const vec4_t colorGreen;
extern const vec4_t colorLtGreen;
extern const vec4_t colorMdGreen;
extern const vec4_t colorDkGreen;
extern const vec4_t colorBlue;
extern const vec4_t colorLtBlue;
extern const vec4_t colorMdBlue;
extern const vec4_t colorYellow;
extern const vec4_t colorBOYellow;
extern const vec4_t colorDkYellow;
extern const vec4_t colorMdYellow;
extern const vec4_t colorLtYellow;
extern const vec4_t colorMagenta;
extern const vec4_t colorPurple;
extern const vec4_t colorLtPurple;
extern const vec4_t colorMdPurple;
extern const vec4_t colorDkPurple;
extern const vec4_t colorPink;
extern const vec4_t colorCyan;
extern const vec4_t colorLtCyan;
extern const vec4_t colorMdCyan;
extern const vec4_t colorDkCyan;
extern const vec4_t colorWhite;
extern const vec4_t colorLtGrey;
extern const vec4_t colorMdGrey;
extern const vec4_t colorDkGrey;
extern const vec4_t colorOrange;
extern const vec4_t colorLtOrange;
extern const vec4_t colorMdOrange;
extern const vec4_t colorDkOrange;
extern const vec4_t colorWhiteFaded;
extern const vec4_t colorGreenFaded;
extern const vec4_t colorRedFaded;
extern const vec4_t colorBlackBlank;

struct TraceThreadInfo;
extern TraceThreadInfo g_traceThreadInfo[THREAD_CONTEXT_COUNT];

void TRACK_q_shared();
unsigned char ColorIndex(unsigned char c);
const char *Com_GetFilenameSubString(const char *pathname);
const char *Com_GetExtensionSubString(const char *filename);
void Com_StripExtension(const char *in, char *out);
void Com_DefaultExtension(char *path, int maxSize, const char *extension);

short BigShort(short l);
int BigLong(int l);
unsigned __int64 BigLong64(unsigned __int64 l);
short LittleShort(short l);
int LittleLong(int l);
int LittleFloatWrite(float l);
short ShortSwap(short l);
short ShortNoSwap(short l);
int LongSwap(int l);
int LongNoSwap(int l);
unsigned __int64 Long64Swap(unsigned __int64 ll);
unsigned __int64 Long64NoSwap(unsigned __int64 ll);
int FloatWriteNoSwap(float f);
float FloatReadNoSwap(int n);
void Swap_Init();

unsigned int I_atoui(const char *str);
__int64 I_atoi64(const char *str);
bool I_islower(int c);
bool I_isupper(int c);
bool I_isdigit(int c);
bool I_isalnum(int c);
bool I_isforfilename(int c);
void I_strncpyz(char *dest, const char *src, int destsize);
int I_strnicmp(const char *s0, const char *s1, int n);
int I_strncmp(const char *s0, const char *s1, int n);
const char *I_stristr(const char *s0, const char *substr);
int I_stricmp(const char *s0, const char *s1);
int I_strcmp(const char *s0, const char *s1);
int I_stricmpwild(const char *wild, const char *s);
char *I_strlwr(char *s);
char *I_strupr(char *s);
int I_iscsym(int c);
void I_strncat(char *dest, int size, const char *src);
int I_DrawStrlen(const char *str);
bool I_IsSpecialToken(const char *buf);
int I_IsSpecialTokenRecursive(const char *buf);
int I_IsSpecialTokenString(const char *buf);
char *I_CleanChat(char *string);
char *I_CleanStr(char *string);
char I_CleanChar(char character);
int Com_sprintf(char *dest, int size, const char *fmt, ...);
int Com_sprintfPos(char *dest, int destSize, int *destPos, const char *fmt, ...);
int CanKeepStringPointer(const char *string);
char *I_itoa(int value, char *buf, int bufsize);
char *va(const char *format, ...);
void Com_InitThreadData(int threadContext);

const char *Info_ValueForKey(const char *s, const char *key);
void Info_NextPair(const char **head, char *key, char *value);
void Info_RemoveKey(char *s, const char *key);
void Info_RemoveKey_Big(char *s, const char *key);
int Info_Validate(const char *s);
void Info_SetValueForKey(char *s, const char *key, const char *value);
void Info_SetValueForKey_Big(char *s, const char *key, const char *value);

int KeyValueToField(unsigned char *pStruct, const cspField_t *pField, const char *pszKeyValue, int iMaxFieldTypes,
                    int (*parseSpecialFieldType)(unsigned char *, const char *, int, int),
                    void (*parseStrCpy)(unsigned char *, const char *));
int ParseConfigStringToStruct(unsigned char *pStruct, const cspField_t *pFieldList, int iNumFields,
                              const char *pszBuffer, int iMaxFieldTypes,
                              int (*parseSpecialFieldType)(unsigned char *, const char *, int, int),
                              void (*parseStrCpy)(unsigned char *, const char *));
int ParseConfigStringToStructMerged(unsigned char *pStruct, const cspField_t *pFieldList, int iNumFields,
                                    const char *mergedName, const char *pszBuffer[], const char *sourceName[],
                                    char *pszMergedBuffer, int iMaxFieldTypes,
                                    int (*parseSpecialFieldType)(unsigned char *, const char *, int, int),
                                    void (*parseStrCpy)(unsigned char *, const char *),
                                    int (*parseMergeSpecialCase)(const char *, char *[], char *, int));

float GetLeanFraction(float fFrac);
float UnGetLeanFraction(float fFrac);
void AddLeanToPosition(vec3_t &position, float fViewYaw, float fLeanFrac, float fViewRoll, float fLeanDist);
void OrientationConcatenate(const orientation_t *orFirst, const orientation_t *orSecond, orientation_t *out);
void OrientationInvert(const orientation_t *orient, orientation_t *out);
int Com_IsLegacyXModelName(const char *name);
float CMD_GetAnalogButtonValue(usercmd_s *cmd, int whichBit);
