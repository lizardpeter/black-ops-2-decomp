// <dxgi.h>: the parts the reconstruction uses (swap chain description), as the Windows SDK lays them out.
#pragma once

#include <windows.h>

typedef enum DXGI_FORMAT
{
	DXGI_FORMAT_UNKNOWN = 0,
	DXGI_FORMAT_R8G8B8A8_UNORM = 28,
} DXGI_FORMAT;

typedef enum DXGI_SWAP_EFFECT
{
	DXGI_SWAP_EFFECT_DISCARD = 0,
	DXGI_SWAP_EFFECT_SEQUENTIAL = 1,
	DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL = 3,
	DXGI_SWAP_EFFECT_FLIP_DISCARD = 4,
} DXGI_SWAP_EFFECT;

typedef struct DXGI_RATIONAL
{
	UINT Numerator;
	UINT Denominator;
} DXGI_RATIONAL;

typedef struct DXGI_MODE_DESC
{
	UINT Width;
	UINT Height;
	DXGI_RATIONAL RefreshRate;
	DXGI_FORMAT Format;
	UINT ScanlineOrdering;
	UINT Scaling;
} DXGI_MODE_DESC;

typedef struct DXGI_SAMPLE_DESC
{
	UINT Count;
	UINT Quality;
} DXGI_SAMPLE_DESC;

typedef struct DXGI_SWAP_CHAIN_DESC
{
	DXGI_MODE_DESC BufferDesc;
	DXGI_SAMPLE_DESC SampleDesc;
	UINT BufferUsage;
	UINT BufferCount;
	HWND OutputWindow;
	BOOL Windowed;
	DXGI_SWAP_EFFECT SwapEffect;
	UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

#define DXGI_USAGE_RENDER_TARGET_OUTPUT 0x00000020UL
#define DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH 2
#define DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT 64
#define DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING 2048
