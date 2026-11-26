// live/live_anticheat.cpp -- Store MP/ZM hash lifecycle.
// Reference: CoDMPServer_PC, lines 535 and 553; Store MP continuation assembly.
// Original hash routines; service integration is not reconstructed.
#define T6_SOURCE "live/live_anticheat.cpp"
#include "live/live_anticheat.h"
#include <stdint.h>

namespace {
struct CrcTable {
    unsigned int entries[256];
    constexpr CrcTable() : entries{} {
        for (unsigned int i = 0; i < 256; ++i) {
            unsigned int crc = i;
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
            entries[i] = crc;
        }
    }
};
constexpr CrcTable table{};
static_assert(sizeof(unsigned int) == 4, "CRC32 state width");
static_assert(sizeof(unsigned long long) == 8, "split CRC state width");
unsigned int Step(unsigned int crc, unsigned char byte) {
    return (crc >> 8) ^ table.entries[(crc & 0xffu) ^ byte];
}
}

void LiveAntiCheat_GenerateCRC32Checksum(unsigned char *data, int length, unsigned int *key) {
    unsigned int crc = *key;
    // Original caller contract: length is nonnegative.
    while (length != 0) {
        crc = Step(crc, *data++);
        --length;
    }
    *key = crc;
}

unsigned long long LiveAntiCheat_GenerateCRC32ChecksumSplit(unsigned char *data, int length, unsigned long long key) {
    unsigned int low = static_cast<unsigned int>(key);
    unsigned int high = static_cast<unsigned int>(key >> 32);
    while (length != 0) {
        if ((reinterpret_cast<uintptr_t>(data) & 1u) == 0)
            low = Step(low, *data);
        else
            high = Step(high, *data);
        ++data;
        --length;
    }
    return (static_cast<unsigned long long>(high) << 32) | low;
}

#ifdef T6_STORE
namespace {
// Native reconstruction storage. Hybrid binding to the original global is
// not installed until the runtime integration is independently validated.
HashInfo s_hashInfos[16]{};
}

// Store MP/ZM startup body; the fourth argument is eliminated by LTCG.
// The original source prototype is retained for the authored implementation.
void *LiveAntiCheat_StartHash(challengeTest_t type, unsigned int start, unsigned int length,
                             unsigned int /*blockSize*/, unsigned long long key)
{
    for (auto &info : s_hashInfos) {
        if (info.type != CT_TYPE_INVALID)
            continue;
        info = {};
        info.type = type;
        info.start = start;
        info.length = length;
        info.blockSize = 0x2000;
        switch (type) {
        case CT_TYPE_CRC32:
        case CT_TYPE_CRC32_SPLIT:
            info.state = key;
            return &info;
        case CT_TYPE_SHA:
            return &info;
        default:
            info.type = CT_TYPE_INVALID;
            return nullptr;
        }
    }
    return nullptr;
}

// Store MP/ZM continuation. Both CRC recurrences were inlined here.
bool LiveAntiCheat_ContinueHash(void *handle)
{
    auto *info = static_cast<HashInfo *>(handle);
    if (info->length == info->position)
        return true;

    auto *data = reinterpret_cast<unsigned char *>(
        static_cast<std::uintptr_t>(info->start + info->position));
    int length = static_cast<int>(info->length - info->position);
    const int blockSize = static_cast<int>(info->blockSize);
    if (blockSize <= length)
        length = blockSize;

    if (info->type == CT_TYPE_CRC32) {
        unsigned int key = static_cast<unsigned int>(info->state);
        LiveAntiCheat_GenerateCRC32Checksum(data, length, &key);
        info->state = key;
    } else if (info->type == CT_TYPE_CRC32_SPLIT) {
        info->state = LiveAntiCheat_GenerateCRC32ChecksumSplit(data, length, info->state);
    }
    info->position += static_cast<unsigned int>(length);
    return info->position == info->length;
}

// Recovered from Store MP inlined finalization and the original server
// CompleteHash reference. Standalone Store entry not established.
bool LiveAntiCheat_CompleteHash(void *handle, unsigned long long *result)
{
    auto *info = static_cast<HashInfo *>(handle);
    if (info->type == CT_TYPE_CRC32 || info->type == CT_TYPE_CRC32_SPLIT)
        *result = info->state;
    *info = {};
    return true;
}
#endif

// Remaining original translation-unit functions -- declaration inventory only.
// Prototypes below are from the named CoDMPServer_PC reference. They are kept
// disabled until their Store x64 ABI is independently established. No bodies,
// placeholder return values, linkage, or runtime overrides are supplied.
// In particular, LTCG may change the emitted Store signature or inline a body.
#if 0
// Ban notification (reference lines 375, 385).
void LiveAntiCheat_BanDoConnected();
void LiveAntiCheat_DWPlayerBanned(unsigned long long userID);

// Challenge request handlers (reference lines 723, 831, 852, 896, 992).
bool LiveAntiCheat_AnswerChecksumRequest(ControllerIndex_t,
    bdArray<bdAntiCheatChallengeParam> *, long long *, challengeTest_t);
bool LiveAntiCheat_AnswerDvarRequest(ControllerIndex_t,
    bdArray<bdAntiCheatChallengeParam> *, long long *);
bool LiveAntiCheat_AnswerDvarFloatRequest(ControllerIndex_t,
    bdArray<bdAntiCheatChallengeParam> *, long long *);
bool LiveAntiCheat_AnswerExpressionRequest(ControllerIndex_t,
    bdArray<bdAntiCheatChallengeParam> *, long long *);
bool LiveAntiCheat_GetChallengeResponse(ControllerIndex_t, unsigned short,
    bdArray<bdAntiCheatChallengeParam> *, long long *);

// Challenge lifecycle (reference lines 1038--1228).
void LiveAntiCheat_AnswerChallengesRetry(ControllerIndex_t);
// Reference returns TaskRecord*; Store candidate return ABI not verified.
TaskRecord *LiveAntiCheat_AnswerChallenges(ControllerIndex_t);
bool LiveAntiCheat_AnswerChallengesSuccess(TaskRecord *);
bool LiveAntiCheat_AnswerChallengesFailure(TaskRecord *);
void LiveAntiCheat_ResetChallengeResponse(ControllerIndex_t);
void LiveAntiCheat_OnChallengesReceived(unsigned long long,
    bdReference<bdAntiCheatChallenges>);
void LiveAntiCheat_ProcessChallenges(ControllerIndex_t);

// Game reporting (reference lines 1394--1425).
void LiveAntiCheat_ClearGameReport();
void LiveAntiCheat_SetThresholdExceeded(int threshold);
void LiveAntiCheat_ReportLaggedDamage();
void LiveAntiCheat_SetGameReportTags(int *numTags, bdTag *tags, int maxTags);
#endif // reference declarations only
