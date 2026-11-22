// gfx_d3d/r_state.cpp (CoDMPServer_PC: r_state.obj)
//
// The renderer's D3D state objects. So far the release of the sampler states (when texture filtering changes).
#define T6_SOURCE "gfx_d3d/r_state.cpp"

#include "gfx_d3d/r_dx.h"
#include "t6_static.h"

T6_GLOBAL ID3D11SamplerState *s_samplerStates[256];

// 00A8E080  gfx_d3d/r_state.cpp:1614 (inlined in the Store build)
void R_DX11_ReleaseSamplerStates()
{
	for (int i = 0; i < 256; i++)
	{
		if (s_samplerStates[i])
		{
			IUnknown_Release(s_samplerStates[i]);
			s_samplerStates[i] = 0;
		}
	}
}
