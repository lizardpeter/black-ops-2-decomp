// Store MP/ZM inline the bool registration. Change only its default immediate.
#pragma once
struct BuildNumberDefaultSite {
    unsigned rva;
    unsigned char expected[19];
};
static const struct BuildNumberDefaultSite t6_build_number_mp = {
    0x3FA37C, {0x89,0x75,0xD7,0x48,0x8D,0x0D,0xC2,0x40,0x57,0x00,0xC6,0x45,0xD7,0x01,0xE8,0x21,0x15,0x07,0x00}
};
static const struct BuildNumberDefaultSite t6_build_number_zm = {
    0x3FAD4C, {0x89,0x75,0xD7,0x48,0x8D,0x0D,0x8A,0x16,0x57,0x00,0xC6,0x45,0xD7,0x01,0xE8,0x41,0x18,0x07,0x00}
};
#define T6_BUILD_NUMBER_DEFAULT_OFFSET 13u
static inline const struct BuildNumberDefaultSite *T6_BuildNumberDefaultSite(unsigned timestamp, unsigned size) {
    if (timestamp==0x6758BF4Fu && size==0x045AC000u) return &t6_build_number_mp;
    if (timestamp==0x6758BC50u && size==0x045AA000u) return &t6_build_number_zm;
    return 0;
}
static inline int T6_BuildNumberDefaultMatches(const unsigned char *code, const struct BuildNumberDefaultSite *site) {
    for (unsigned i=0; i<sizeof(site->expected); ++i)
        if (code[i]!=site->expected[i]) return 0;
    return 1;
}
