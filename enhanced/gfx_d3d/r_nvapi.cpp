#ifdef T6_STORE
#include "gfx_d3d/r_nvapi.h"

extern "C" int NvAPI_D3D11_CreateDevice(
	IDXGIAdapter1 *adapter, D3D_DRIVER_TYPE driverType, void *software, UINT flags,
	const D3D_FEATURE_LEVEL *featureLevels, UINT featureLevelCount, UINT sdkVersion,
	ID3D11Device **device, D3D_FEATURE_LEVEL *featureLevel,
	ID3D11DeviceContext **immediateContext, int *supportedLevel)
{
	const int status = R_NvAPICreateDevice(adapter, driverType, software, flags, featureLevels,
	                                     featureLevelCount, sdkVersion, device, featureLevel,
	                                     immediateContext, supportedLevel);
	if (!R_NvAPIUnavailable(status))
		return status;

	D3D_FEATURE_LEVEL level;
	const HRESULT created = D3D11CreateDevice(adapter, driverType, software, flags, featureLevels,
	                                         featureLevelCount, sdkVersion, device, &level, immediateContext);
	if (created < 0)
		return -1;
	if (featureLevel)
		*featureLevel = level;
	if (supportedLevel)
	{
		constexpr int nvFeatureLevel10 = 1;
		constexpr int nvFeatureLevel10_1 = 2;
		constexpr int nvFeatureLevel11 = 3;
		*supportedLevel = nvFeatureLevel10;
		if (level >= D3D_FEATURE_LEVEL_11_0)
			*supportedLevel = nvFeatureLevel11;
		else if (level >= D3D_FEATURE_LEVEL_10_1)
			*supportedLevel = nvFeatureLevel10_1;
	}
	return 0;
}
#endif
