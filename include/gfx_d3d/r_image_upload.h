#pragma once
#ifdef T6_STORE
// Recovered regular upload contract. Volume images use a different path.
enum StoreUploadKind : unsigned { STORE_UPLOAD_SKIP, STORE_UPLOAD_UPDATE, STORE_UPLOAD_VOLUME };
struct StoreImageUploadPlan {
    StoreUploadKind kind;
    unsigned subresource, rowPitch, depthPitch;
};
extern "C" void Store_PlanImageUpload(StoreImageUploadPlan *, unsigned imageType, unsigned width,
    unsigned format, unsigned face, unsigned mip, unsigned resourceMipLevels);
#endif
