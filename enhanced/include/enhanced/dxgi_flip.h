// enhanced/include/enhanced/dxgi_flip.h: the flip-model presentation state shared by the enhanced renderer files
// (P0005: enhanced/gfx_d3d/r_init.cpp, rb_backend.cpp) and the frame-latency wait (P0007: enhanced/gfx_d3d/r_init.cpp,
// enhanced/qcommon/common.cpp).
#pragma once

#include "gfx_d3d/r_dx.h"

// false once the system refused a flip-model swap chain (the original blit model is used from then on)
extern bool r_flipModel;
// DXGI can present with tearing (IDXGIFactory5, DXGI 1.5); asked once, before the first swap chain
extern bool r_allowTearing;

bool R_CheckTearingSupport();
void R_UseBlitModel(DXGI_SWAP_CHAIN_DESC *swapChainDesc);
HRESULT R_Present(IDXGISwapChain *swapChain, UINT syncInterval);

// P0007: the swap chain's frame-latency waitable object, with at most one frame queued. The render thread counts the
// presents made on that swap chain (R_Present); the main thread waits once per counted present before its frame
// reads input (R_WaitForFrameLatency, enhanced/qcommon/common.cpp).
extern bool r_latencyWaitable;              // false once the system refused a waitable swap chain
void R_AcquireFrameLatencyWaitable(bool waitableSwapChain);  // after the swap chain is created
void R_CountPresent();                      // after each successful present on it
void R_WaitForFrameLatency();
