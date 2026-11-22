// gfx_d3d/r_rendercmds.cpp: the front end's hand-off of a frame to the back end.
#pragma once

struct ID3D11Buffer;

// A dynamic vertex buffer and how much of it the frame used (0x10 bytes on x86, 0x18 on x64)
struct GfxVertexBufferState
{
	volatile int used;
	int total;
	ID3D11Buffer *buffer;
	unsigned char *verts;
};

struct GfxBackEndData;

#ifdef T6_STORE
// The few GfxBackEndData fields the Store code here reads, at their Store offsets (the type is the server's, laid out
// for x64 with Store-only members; it isn't reconstructed whole yet)
inline GfxVertexBufferState *&GfxBackEndData_skinnedCacheVb(GfxBackEndData *data)
{
	return *(GfxVertexBufferState **)((char *)data + 0x4BE9C0);
}
inline volatile int &GfxBackEndData_skinnedCacheVertsAllocated(GfxBackEndData *data)
{
	return *(volatile int *)((char *)data + 0x4BE9C8);
}
inline unsigned int &GfxBackEndData_viewInfoIndex(GfxBackEndData *data)
{
	return *(unsigned int *)((char *)data + 0x4BEA50);
}

extern GfxBackEndData *frontEndDataOut;
void R_ToggleSmpFrame();
void R_UpdateSkinCacheUsage();
void R_ToggleSmpFrameCmd(unsigned int type);
#endif
