#pragma once
#ifdef T6_STORE
struct ID3D11Buffer;
struct GfxVertexBufferState {unsigned used,total;ID3D11Buffer *buffer;unsigned char *verts;};
struct SkinnedVertsDef;
static_assert(sizeof(GfxVertexBufferState)==24,"Store vertex buffer state");
void Load_IndexBuffer(ID3D11Buffer **out,void *data,int count);
void Load_VertexBuffer(ID3D11Buffer **out,void *data,int bytes);
void R_InitDynamicVertexBufferState(GfxVertexBufferState *state,int bytes);
void R_InitSkinnedVertexBuffers(SkinnedVertsDef *def);
void R_ShutdownSkinnedVertexBuffers();
#endif
