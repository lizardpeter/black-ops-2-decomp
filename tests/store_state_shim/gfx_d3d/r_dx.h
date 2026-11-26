#pragma once
#define WINAPI __attribute__((ms_abi))
#define T6_COM_CALL(obj,slot,type,...) (((type)((*(void ***)(obj))[slot]))((obj), ##__VA_ARGS__))
struct ID3D11DeviceContext;
