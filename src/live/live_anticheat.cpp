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

// Remaining translation-unit functions: Store MP signature evidence only.
// No implementation, executable declarations, or runtime binding is supplied.
// Store LTCG can inline source functions or combine their callers. A source
// prototype is not necessarily a standalone x64 ABI. The following inventory
// distinguishes observed return/argument shapes from proposed semantic types.
//
// Evidence: analysis/t6mp_msstore/{pseudocode,disasm}/_unattributed/14050000.*
// and analysis/t6mp_msstore/match/functions.tsv. ZM/SP not verified here.
#if 0
// Candidate: Store body has no arguments or returned value; name based on
// ban/disconnect string references. Separate player-ban callback unresolved.
void LiveAntiCheat_BanDoConnected(); // Store candidate
// LiveAntiCheat_DWPlayerBanned: standalone Store entry/signature unresolved.

// Store pseudocode and caller analysis support a Boolean result for the
// following two retained functions. Argument widths are visible, but exact
// original C++ parameter types and qualifiers are not independently proven.
// Keep reference semantic types provisional; no definition is supplied.
bool LiveAntiCheat_AnswerChecksumRequest(ControllerIndex_t,
    bdArray<bdAntiCheatChallengeParam> *, long long *, challengeTest_t);
bool LiveAntiCheat_GetChallengeResponse(ControllerIndex_t, unsigned short,
    bdArray<bdAntiCheatChallengeParam> *, long long *);

// The dispatcher contains the other three reference request handlers'
// behavior. No distinct Store entries or standalone signatures established.
// LiveAntiCheat_AnswerDvarRequest: inline candidate.
// LiveAntiCheat_AnswerDvarFloatRequest: inline candidate.
// LiveAntiCheat_AnswerExpressionRequest: inline candidate.

// Store candidates: one 32-bit argument, no consumed return value.
void LiveAntiCheat_AnswerChallengesRetry(ControllerIndex_t);
void LiveAntiCheat_ResetChallengeResponse(ControllerIndex_t);

// The success callback is matched by registration; the failure callback
// has the corresponding single-pointer/Boolean machine-level shape.
// TaskRecord is a reference-derived semantic type, not a verified x64 layout.
bool LiveAntiCheat_AnswerChallengesSuccess(TaskRecord *);
bool LiveAntiCheat_AnswerChallengesFailure(TaskRecord *);

// Store candidate has two 64-bit argument slots and no recovered result.
// The by-value reference wrapper's precise x64 ABI remains unverified.
void LiveAntiCheat_OnChallengesReceived(unsigned long long,
    bdReference<bdAntiCheatChallenges>);

// The generated matcher labels a Store candidate AnswerChallenges (0.75).
// Its observed interface has one 32-bit argument, no consumed return value,
// and includes processing logic. This may be a merged/inlined body.
// LiveAntiCheat_AnswerChallenges: separate Store signature unresolved.
// LiveAntiCheat_ProcessChallenges: candidate void(ControllerIndex_t);
// do not assert a separate entry for both names.

// Reporting:
// LiveAntiCheat_ClearGameReport: separate Store signature unresolved.
// LiveAntiCheat_SetThresholdExceeded: separate Store signature unresolved.
// LiveAntiCheat_ReportLaggedDamage: separate Store signature unresolved.
// LiveAntiCheat_SetGameReportTags: Store candidate accepts two pointer-sized
// arguments; the reference's third argument is not established in this build.
// Exact source prototype and return contract remain unresolved.
#endif // noncompiled Store MP signature inventory
