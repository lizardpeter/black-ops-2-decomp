#pragma once

#include <d3d11.h>

extern "C" int NvAPI_D3D11_CreateDevice(
	IDXGIAdapter1 *adapter, D3D_DRIVER_TYPE driverType, void *software, UINT flags,
	const D3D_FEATURE_LEVEL *featureLevels, UINT featureLevelCount, UINT sdkVersion,
	ID3D11Device **device, D3D_FEATURE_LEVEL *featureLevel,
	ID3D11DeviceContext **immediateContext, int *supportedLevel);

decltype(NvAPI_D3D11_CreateDevice) R_NvAPICreateDevice;

inline bool R_NvAPIUnavailable(int status)
{
	return status == -2 || status == -3; // NVAPI_LIBRARY_NOT_FOUND or NVAPI_NO_IMPLEMENTATION
}
