#pragma once
inline unsigned IUnknown_Release(void *obj) {return reinterpret_cast<unsigned (__attribute__((ms_abi)) *)(void *)>((*static_cast<void ***>(obj))[2])(obj);}
