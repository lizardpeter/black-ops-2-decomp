#pragma once
#define WINAPI __attribute__((ms_abi))
#define T6_COM_CALL(obj,slot,type,...) (((type)((*(void ***)(obj))[slot]))((obj), ##__VA_ARGS__))
struct ID3D11Resource; struct ID3D11View; struct ID3D11DeviceContext;
struct D3D11_MAPPED_SUBRESOURCE {void *pData; unsigned RowPitch,DepthPitch;};
#define D3D11_MAP_WRITE_DISCARD 4
extern ID3D11DeviceContext *context;
#define DX(x) (x)
inline void ID3D11View_GetResource(ID3D11View *v,ID3D11Resource **r){T6_COM_CALL(v,7,void(WINAPI *)(ID3D11View *,ID3D11Resource **),r);}
inline unsigned IUnknown_Release(void *r){return T6_COM_CALL(r,2,unsigned(WINAPI *)(void *));}
inline int ID3D11DeviceContext_Map(ID3D11DeviceContext *c,void *r,unsigned s,unsigned t,unsigned f,D3D11_MAPPED_SUBRESOURCE *m){return T6_COM_CALL(c,14,int(WINAPI *)(ID3D11DeviceContext *,void *,unsigned,unsigned,unsigned,D3D11_MAPPED_SUBRESOURCE *),r,s,t,f,m);}
inline void ID3D11DeviceContext_Unmap(ID3D11DeviceContext *c,void *r,unsigned s){T6_COM_CALL(c,15,void(WINAPI *)(ID3D11DeviceContext *,void *,unsigned),r,s);}
