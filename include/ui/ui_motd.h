// ui/ui_motd.cpp: the message of the day, from Treyarch's online file motd-mp-<language>.json (motd-zm-... for
// Zombies), parsed into one motd_t the main menu shows.
#pragma once

// The layout the code uses (the reference PDB's motd_t, 0x560 bytes, is an older version with shorter fields and no
// action strings; MemTrackInst records 0x6DC for s_motd).
struct motd_t
{
	int version;          // the message's own version: the profile remembers the last one seen (motdVersionViewed)
	bool singleView;      // show it once only
	int expiration;       // a time() after which it isn't shown
	unsigned short imageCategory;  // the publisher-file category the message's images are listed from
	char title[65];
	char message[1025];
	char image[33];
	char action[32];
	char actionContext[65];
	char actionString[65];
	char bannerTitle[65];
	char bannerMessage[129];
	char bannerImage[33];
	char bannerAction[32];
	char bannerActionContext[65];
	char bannerActionString[65];
	char eliteImage[33];
	char founderImage[33];
	bool isValid;  // parsed, with a message
};
static_assert(sizeof(motd_t) == 0x6DC, "motd_t");
static_assert(__builtin_offsetof(motd_t, message) == 0x4F && __builtin_offsetof(motd_t, bannerMessage) == 0x554 &&
                  __builtin_offsetof(motd_t, isValid) == 0x6DA,
              "motd_t");

// One key of a JSON object and the parser for its value (which reads the value with JSMN_GetToken).
struct motdKeyHandler_t
{
	const char *key;
	bool (*handler)();
};

bool MOTD_ParseVersion();
bool MOTD_ParseMOTDVersion();
bool MOTD_ParseSingleView();
bool MOTD_ParseExpiration();
bool MOTD_ParseImageCategory();
bool MOTD_ParseTitle();
bool MOTD_ParseMessage();
bool MOTD_ParseImage();
bool MOTD_ParseAction();
bool MOTD_ParseActionContext();
bool MOTD_ParseActionString();
bool MOTD_ParseBannerTitle();
bool MOTD_ParseBannerMessage();
bool MOTD_ParseBannerImage();
bool MOTD_ParseBannerAction();
bool MOTD_ParseBannerActionContext();
bool MOTD_ParseBannerActionString();
bool MOTD_ParseEliteImage();
bool MOTD_ParseFounderImage();
bool MOTD_ParseDataEntry();
bool MOTD_Parse(const char *buffer);
void MOTD_GetImageDescriptors_f();
