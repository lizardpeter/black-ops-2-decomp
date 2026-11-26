#ifdef T6_STORE
#include "gfx_d3d/r_nvapi.h"

using NvCreateDevice = decltype(&NvAPI_D3D11_CreateDevice);

extern "C" int NvAPI_AcquireInterface(void **context);
extern "C" void *(*nvapiQueryInterface)(unsigned);
extern "C" void (*nvapiBeforeCall)(unsigned, void **);
extern "C" void (*nvapiAfterCall)(unsigned, void *, int);
extern "C" int nvapiActiveCalls;
extern "C" int nvapiInterfaceGeneration;
extern "C" NvCreateDevice nvapiCreateDeviceEntry;
extern "C" int nvapiCreateDeviceGeneration;

int R_NvAPICreateDevice(IDXGIAdapter1 *adapter, D3D_DRIVER_TYPE driverType, void *software, UINT flags,
                        const D3D_FEATURE_LEVEL *featureLevels, UINT featureLevelCount, UINT sdkVersion,
                        ID3D11Device **device, D3D_FEATURE_LEVEL *featureLevel,
                        ID3D11DeviceContext **immediateContext, int *supportedLevel)
{
	void *context = nullptr;
	int status = NvAPI_AcquireInterface(&context);
	constexpr int invalidateInterface = -14;
	if (status != 0 && status != invalidateInterface)
		return status;

	constexpr unsigned functionId = 0x6A16D3A0;
	if (status == invalidateInterface)
		nvapiCreateDeviceEntry = nullptr;
	if (nvapiCreateDeviceEntry && nvapiCreateDeviceGeneration != nvapiInterfaceGeneration)
		nvapiCreateDeviceEntry = nullptr;
	if (!nvapiCreateDeviceEntry && context && nvapiQueryInterface)
	{
		nvapiCreateDeviceEntry = reinterpret_cast<NvCreateDevice>(nvapiQueryInterface(functionId));
		nvapiCreateDeviceGeneration = nvapiInterfaceGeneration;
	}

	const auto create = nvapiCreateDeviceEntry;
	if (!create)
	{
		--nvapiActiveCalls;
		return -3; // NVAPI_NO_IMPLEMENTATION
	}

	void *callbackContext = nullptr;
	if (nvapiBeforeCall)
		nvapiBeforeCall(functionId, &callbackContext);
	status = create(adapter, driverType, software, flags, featureLevels, featureLevelCount, sdkVersion,
	                device, featureLevel, immediateContext, supportedLevel);
	if (nvapiAfterCall)
		nvapiAfterCall(functionId, callbackContext, status);
	--nvapiActiveCalls;
	return status;
}

extern "C" int NvAPI_D3D11_CreateDevice(
	IDXGIAdapter1 *adapter, D3D_DRIVER_TYPE driverType, void *software, UINT flags,
	const D3D_FEATURE_LEVEL *featureLevels, UINT featureLevelCount, UINT sdkVersion,
	ID3D11Device **device, D3D_FEATURE_LEVEL *featureLevel,
	ID3D11DeviceContext **immediateContext, int *supportedLevel)
{
	return R_NvAPICreateDevice(adapter, driverType, software, flags, featureLevels, featureLevelCount,
	                          sdkVersion, device, featureLevel, immediateContext, supportedLevel);
}
#endif
