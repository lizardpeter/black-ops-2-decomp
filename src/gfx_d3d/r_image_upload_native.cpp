#define T6_SOURCE "gfx_d3d/r_image.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_dx.h"
#include "universal/win_common.h"
#include <string.h>

struct GfxImage;
namespace {
const unsigned char *Bytes(const GfxImage *image) { return reinterpret_cast<const unsigned char *>(image); }
unsigned Read16(const GfxImage *image, unsigned offset) {
    const auto *p=Bytes(image)+offset; return p[0] | (unsigned(p[1])<<8);
}
unsigned Mip(unsigned n,unsigned mip) { n >>= mip & 31; return n ? n : 1; }
ID3D11Resource *GetResource(GfxImage *image) {
    ID3D11View *view; memcpy(&view, image, sizeof(view));
    ID3D11Resource *res; ID3D11View_GetResource(view,&res); return res;
}
unsigned SliceBytes(unsigned fmt,unsigned w,unsigned h) {
    switch(fmt) {
    case 28: case 41: case 45: return w*h*4;
    case 55: case 85: return w*h*2;
    case 65: return w*h;
    case 71: return ((w+3)>>2)*((h+3)>>2)*8;
    case 74: case 77: case 83: return ((w+3)>>2)*((h+3)>>2)*16;
    default: return 0;
    }
}
unsigned RowBytes(unsigned fmt,unsigned w,unsigned &step) {
    step=1;
    switch(fmt) {
    case 10: case 11: case 28: case 35: case 41: return w*4;
    case 65: return w;
    case 71: step=4; return ((w+3)>>2)*8;
    case 74: case 77: step=4; return ((w+3)>>2)*16;
    case 85: return w*2;
    default: return 0;
    }
}
}
// Store MP 1407770F0 / ZM 140774C30 / SP 14061C9E0.
extern "C" void Store_ImageUploadVolume(GfxImage *image,unsigned format,unsigned mip,const unsigned char *pixels)
{
    const unsigned w=Mip(Read16(image,0x18),mip), h=Mip(Read16(image,0x1a),mip), d=Mip(Read16(image,0x1c),mip);
    const unsigned slice=SliceBytes(format,w,h);
    unsigned step; const unsigned row=RowBytes(format,w,step);
    auto *resource=GetResource(image);
    Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
    for(unsigned z=0;z<d;++z) {
        const unsigned sub=z*Bytes(image)[0x1e]+mip;
        D3D11_MAPPED_SUBRESOURCE mapped;
        // The original does not check HRESULT; preserve the successful-map contract.
        ID3D11DeviceContext_Map(DX(context),resource,sub,D3D11_MAP_WRITE_DISCARD,0,&mapped);
        if(row) {
            auto *dst=static_cast<unsigned char *>(mapped.pData);
            const auto *src=pixels;
            const int pitch=static_cast<int>(mapped.RowPitch);
            if(pitch==static_cast<int>(row)) {
                memcpy(dst,src,static_cast<__SIZE_TYPE__>(static_cast<int>(row*((h-1)/step+1))));
            } else {
                for(unsigned y=0;y<h;y+=step) {
                    memcpy(dst,src,row); dst+=pitch; src+=row;
                }
            }
        }
        ID3D11DeviceContext_Unmap(DX(context),resource,sub);
        pixels+=static_cast<int>(slice);
    }
    Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
    IUnknown_Release(resource);
}
// Store MP 140777440 / ZM 140774F80 / SP 14061CD30.
extern "C" void Store_ImageUploadData(GfxImage *image,unsigned format,int face,int mip,const unsigned char *pixels)
{
    if(Bytes(image)[8]==4) { Store_ImageUploadVolume(image,format,mip,pixels); return; }
    unsigned step; const unsigned row=RowBytes(format,Mip(Read16(image,0x18),mip),step);
    if(!row) return;
    auto *resource=GetResource(image);
    unsigned desc[11];
    T6_COM_CALL(resource,10,void(WINAPI *)(ID3D11Resource *,unsigned *),desc);
    const unsigned sub=static_cast<unsigned>(face)*desc[2]+static_cast<unsigned>(mip);
    Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
    T6_COM_CALL(DX(context),48,void(WINAPI *)(ID3D11DeviceContext *,ID3D11Resource *,unsigned,const void *,const void *,unsigned,unsigned),resource,sub,nullptr,pixels,row,0);
    Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
    IUnknown_Release(resource);
}
#endif
