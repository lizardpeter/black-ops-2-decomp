#pragma once
#define WINAPI __attribute__((ms_abi))
#define T6_COM_CALL(obj,slot,type,...) (((type)((*(void ***)(obj))[slot]))((obj), ##__VA_ARGS__))
using HRESULT=int;
struct ID3D11Resource; struct ID3D11Device; struct ID3D11View;
extern ID3D11Device *device; extern unsigned multiSampleType;
#define DX(name) (::name)
inline void ID3D11View_GetResource(ID3D11View *v,ID3D11Resource **r){T6_COM_CALL(v,7,void(WINAPI *)(ID3D11View *,ID3D11Resource **),r);}
inline unsigned IUnknown_Release(void *obj){return T6_COM_CALL(obj,2,unsigned(WINAPI *)(void *));}
