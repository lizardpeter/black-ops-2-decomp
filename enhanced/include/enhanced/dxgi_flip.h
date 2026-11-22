// enhanced/include/enhanced/dxgi_flip.h: the flip-model presentation state shared by the enhanced renderer files
// (P0005: enhanced/gfx_d3d/r_init.cpp, rb_backend.cpp).
#pragma once

#include "gfx_d3d/r_dx.h"

// false once the system refused a flip-model swap chain (the original blit model is used from then on)
extern bool r_flipModel;
// DXGI can present with tearing (IDXGIFactory5, DXGI 1.5); asked once, before the first swap chain
extern bool r_allowTearing;

bool R_CheckTearingSupport();
void R_UseBlitModel(DXGI_SWAP_CHAIN_DESC *swapChainDesc);
HRESULT R_Present(IDXGISwapChain *swapChain, UINT syncInterval);
