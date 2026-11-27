// gfx_d3d: vidConfig_t (r_dx.h, client.h's cls).
#pragma once

// The video settings the renderer works to (0x40 bytes, no pointers: the same on x64). Right after dx in both builds.
struct vidConfig_t
{
	unsigned int sceneWidth;
	unsigned int sceneHeight;
	unsigned int displayWidth;
	unsigned int displayHeight;
	unsigned short outputDisplayWidth;
	unsigned short outputDisplayHeight;
	unsigned int displayFrequency;
	bool isToolMode;
	int isWideScreen;
	int isHiDef;
	int isFullscreen;
	float aspectRatioWindow;
	float aspectRatioScenePixel;
	float aspectRatioDisplayPixel;
	unsigned int maxTextureSize;
	unsigned int maxTextureMaps;
	bool deviceSupportsGamma;
};
static_assert(sizeof(vidConfig_t) == 0x40, "vidConfig_t");
