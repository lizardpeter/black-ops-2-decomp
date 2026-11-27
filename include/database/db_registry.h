// database/db_registry.cpp: the asset pools.
#pragma once

enum XAssetType : int
{
	ASSET_TYPE_FONT = 20,
	ASSET_TYPE_FONTICON = 21,
};

struct Font_s;

union XAssetHeader
{
	Font_s *font;
	void *data;
};

XAssetHeader DB_FindXAssetHeader(XAssetType type, const char *name, bool errorIfMissing, int waitTime);
