// bgame/: the shared game state the client reads. Structures list their fields up to the last one reconstructed
// code uses, at the server PDB's offsets (static_asserts), then pad to the full size.
#pragma once

#include "universal/q_shared.h"

enum pmtype_t
{
	PM_NORMAL = 0,
	PM_NORMAL_LINKED = 1,
	PM_NOCLIP = 2,
	PM_UFO = 3,
	PM_MPVIEWER = 4,
	PM_SPECTATOR = 5,
};

struct playerState_s
{
	int commandTime;
	int pm_type;
	int bobCycle;
	int pm_flags;
	unsigned char unknown10[0x18 - 0x10];
	int otherFlags;
	unsigned char unknown1C[0x98 - 0x1C];
	vec3_t delta_angles;
	unsigned char unknownA4[0xFC - 0xA4];
	int eFlags;
	int eFlags2;
	unsigned char unknown104[0x1B8 - 0x104];
	Weapon weapon;
	unsigned char unknown1BC[0x1D0 - 0x1BC];
	int weaponstate;
	unsigned char unknown1D4[0x208 - 0x1D4];
	float viewHeightCurrent;
	unsigned char unknown20C[0x484 - 0x20C];
	int linkFlags;
	unsigned char unknown488[0x570 - 0x488];
	Weapon inventoryWeapon;
	unsigned char unknown574[0x2A08 - 0x574];
};
static_assert(sizeof(playerState_s) == 0x2A08, "playerState_s");
static_assert(__builtin_offsetof(playerState_s, eFlags) == 0xFC, "playerState_s.eFlags");
static_assert(__builtin_offsetof(playerState_s, weapon) == 0x1B8, "playerState_s.weapon");
static_assert(__builtin_offsetof(playerState_s, viewHeightCurrent) == 0x208, "playerState_s.viewHeightCurrent");
static_assert(__builtin_offsetof(playerState_s, inventoryWeapon) == 0x570, "playerState_s.inventoryWeapon");
static_assert(__builtin_offsetof(playerState_s, weaponstate) == 0x1D0, "playerState_s.weaponstate");
static_assert(__builtin_offsetof(playerState_s, linkFlags) == 0x484, "playerState_s.linkFlags");

enum
{
	WEAPON_DEPLOYED = 0x2C,   // playerState_s.weaponstate (weaponstate_t)
	POF_NO_MOVE_INPUT = 0x400,   // playerState_s.otherFlags: the mouse may not move the player (descriptive)
	EF2_VIEW_LOCKED = 0x40000,   // playerState_s.eFlags2: the mouse may not turn the view (descriptive)
	PMF_FROZEN = 0x800,       // playerState_s.pm_flags: no view control (CoD4's name; CL_AdjustAngles tests it)
	PMF_SPRINTING = 0x8000,   // playerState_s.pm_flags (as cl_input.cpp tests it)
	EF_VEHICLE_DRIVER = 0x4000,  // playerState_s.eFlags: the stance key works the vehicle (as cl_input.cpp tests it)
};

enum weapFireType_t
{
	WEAPON_FIRETYPE_FULLAUTO = 0,
	WEAPON_FIRETYPE_SINGLESHOT = 1,
	WEAPON_FIRETYPE_BURSTFIRE2 = 2,
	WEAPON_FIRETYPE_BURSTFIRE3 = 3,
};

struct WeaponDef
{
	unsigned char unknown0[0x44];
	int iJamFireTime;
};

// bg_weapons.cpp
const WeaponDef *BG_GetWeaponDef(Weapon weapon);
weapFireType_t BG_GetFireType(Weapon weapon);
bool BG_WeaponBlocksProne(Weapon weapon);

// bg_misc.cpp
struct dvar_t;
extern const dvar_t *player_standingViewHeight;
