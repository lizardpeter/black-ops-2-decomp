#define T6_SOURCE "gfx_d3d/r_buffers.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_buffer_store.h"
#include "universal/dvar.h"
#include <string.h>
extern int store_142155DFC;
extern const dvar_t *store_imageCreateDvar;
extern "C" {
extern GfxVertexBufferState store_skinnedVertexPool[2];
void *Store_SkinVirtualAlloc(int bytes);
[[noreturn]] void Store_SkinAllocFatal();
}
namespace {
void CreateBuffer(ID3D11Buffer **out,unsigned bytes,unsigned usage,unsigned bind,unsigned access,const void *data,bool initial) {
    const unsigned desc[6]={bytes,usage,bind,access,0,0};
    struct {const void *data;unsigned pitch,slice;} init={data,0,0};
    do {T6_COM_CALL(DX(device),3,HRESULT(WINAPI *)(ID3D11Device *,const unsigned *,const void *,ID3D11Buffer **),desc,initial?&init:nullptr,out);} while(store_142155DFC);
}
}
void Load_IndexBuffer(ID3D11Buffer **out,void *data,int count) {
    if(Dvar_GetBool(store_imageCreateDvar)) CreateBuffer(out,static_cast<unsigned>(count)*2u,1,2,0,data,true);
}
void Load_VertexBuffer(ID3D11Buffer **out,void *data,int bytes) {
    if(Dvar_GetBool(store_imageCreateDvar)&&data) CreateBuffer(out,bytes,1,1,0,data,true);
    else *out=nullptr;
}
void R_InitDynamicVertexBufferState(GfxVertexBufferState *state,int bytes) {
    state->total=bytes;state->used=0;
    if(Dvar_GetBool(store_imageCreateDvar)) CreateBuffer(&state->buffer,bytes,2,1,0x10000,nullptr,false);
    state->verts=nullptr;
}
void R_InitSkinnedVertexBuffers(SkinnedVertsDef *def) {
    for(auto &state:store_skinnedVertexPool) {
        unsigned count;memcpy(&count,reinterpret_cast<const unsigned char *>(def)+8,4);
        R_InitDynamicVertexBufferState(&state,static_cast<int>(count<<5));
        auto *mem=static_cast<unsigned char *>(Store_SkinVirtualAlloc(static_cast<int>(state.total)));
        if(!mem) Store_SkinAllocFatal();
        state.verts=mem;
    }
}
void R_ShutdownSkinnedVertexBuffers() {
    for(auto &state:store_skinnedVertexPool) if(state.buffer) {
        do {IUnknown_Release(state.buffer);state.buffer=nullptr;} while(store_142155DFC);
        VirtualFree(state.verts,0,0x8000);
        state.verts=nullptr;
    }
}
#endif
