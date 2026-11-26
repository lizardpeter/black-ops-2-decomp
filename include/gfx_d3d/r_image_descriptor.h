#pragma once
#ifdef T6_STORE
// Exact D3D11 descriptor words observed at the Store resource boundary.
// These are not Vulkan allocations or a general DXGI format policy.
struct StoreImageDescriptor {
    unsigned texture[11]; // D3D11_TEXTURE2D_DESC or D3D11_TEXTURE3D_DESC
    unsigned view[6];     // D3D11_SHADER_RESOURCE_VIEW_DESC
    unsigned textureWords, type, width, height, depth;
};
extern "C" void Store_DescribeImage2D(StoreImageDescriptor *, unsigned width, unsigned height,
    unsigned mipLevels, unsigned flags, unsigned format, unsigned sampleCount, unsigned sampleQuality);
extern "C" void Store_DescribeImage3D(StoreImageDescriptor *, unsigned width, unsigned height,
    unsigned depth, unsigned mipLevels, unsigned format);
extern "C" void Store_DescribeImageCube(StoreImageDescriptor *, unsigned width, unsigned mipLevels, unsigned format);
#endif
