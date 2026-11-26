#pragma once
#ifdef T6_STORE
// Original Store memory-accounting arithmetic, not Vulkan allocation size.
// The original names are unresolved; these are explicit Store ABI labels.
extern "C" unsigned Store_ImageMipBytes(unsigned format, unsigned width,
                                        unsigned height, unsigned depth);
extern "C" void Store_ImageTrackMemory(void *image, unsigned flags, unsigned format,
                                      unsigned width, unsigned height, unsigned depth);
#endif
