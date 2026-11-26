#pragma once
#define WINAPI __attribute__((ms_abi))
#define T6_COM_CALL(obj,slot,type,...) (((type)((*(void ***)(obj))[slot]))((obj), ##__VA_ARGS__))
using HRESULT=int;
struct ID3D11Resource; struct ID3D11Device;
extern ID3D11Device *device;
extern unsigned multiSampleType,multiSampleQuality;
#define DX(name) (::name)
inline unsigned IUnknown_Release(void *obj){return T6_COM_CALL(obj,2,unsigned(WINAPI *)(void *));}
