#pragma once
#ifdef T6_STORE
// Store-specific state offsets, not a claim that the complete structures are known.
struct GfxCmdBufSourceState;
struct GfxCmdBufState;
struct StoreCmdBufContext {GfxCmdBufSourceState *source; GfxCmdBufState *state;};
void Store_ResetWorldMatrix(GfxCmdBufSourceState *source);
void Store_Set3DView(GfxCmdBufSourceState *source);
void R_SetRenderTarget(const StoreCmdBufContext *context,unsigned char target);
void Store_ClearScreen(GfxCmdBufState *state,unsigned char mask,const float *color,float depth,unsigned char stencil);
#endif
