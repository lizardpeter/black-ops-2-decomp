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

void LiveAntiCheat_GenerateCRC32Checksum(const unsigned char *data, int length, unsigned int *key) {
    unsigned int crc = *key;
    // Original caller contract: length is nonnegative.
    while (length != 0) {
        crc = Step(crc, *data++);
        --length;
    }
    *key = crc;
}

unsigned long long LiveAntiCheat_GenerateCRC32ChecksumSplit(const unsigned char *data, int length, unsigned long long key) {
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
// Inventory completeness: the PDB-attributed x86 reference contains 23
// distinct LiveAntiCheat_* names. This file accounts for all 23 as five
// authored definitions, eight noncompiled candidate declarations, three
// inline candidates and seven unresolved standalone declarations.
// This is name coverage, not Store signature or implementation coverage.
//
// The generated Store MP matcher independently associates only two entries
// from this source unit with reference names: AnswerChallengesSuccess
// (named/registration, confidence 0.89) and AnswerChallenges (caller,
// confidence 0.75). All other proposed identifications in this block are
// manual candidates, not independently corroborated matcher results.
// An unused return register does not prove a void source return type;
// likewise, integer/pointer-width arguments alone do not establish the
// original C++ type, reference qualifiers, or eliminated arguments.
// No candidate is promoted to a compiled declaration or a registry entry
// on the strength of these observations alone.
//
// Complete return-type inventory from the PDB-attributed x86 reference:
//   void: BanDoConnected, DWPlayerBanned, GenerateCRC32Checksum,
//         AnswerChallengesRetry, ResetChallengeResponse,
//         OnChallengesReceived, ProcessChallenges, ClearGameReport,
//         SetThresholdExceeded, ReportLaggedDamage, SetGameReportTags.
//   bool: ContinueHash, CompleteHash, AnswerChecksumRequest,
//         AnswerDvarRequest, AnswerDvarFloatRequest,
//         AnswerExpressionRequest, GetChallengeResponse,
//         AnswerChallengesSuccess, AnswerChallengesFailure.
//   unsigned long long: GenerateCRC32ChecksumSplit.
//   void *: StartHash.
//   TaskRecord *: AnswerChallenges.
// Source: analysis/CoDMPServer_PC/pseudocode/live/live_anticheat.cpp.c.
// These are reference return types, not blanket Store ABI declarations.
// Store evidence and inline/unresolved status remain as specified below
// and in docs/store-checksum-reconstruction.md.
#if 0
// Candidate: Store body has no arguments or returned value; name based on
// ban/disconnect string references. Separate player-ban callback unresolved.
void LiveAntiCheat_BanDoConnected(); // Store candidate
// LiveAntiCheat_DWPlayerBanned: reference void; Store entry/return unresolved.

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
// LiveAntiCheat_AnswerDvarRequest: reference bool; inline candidate.
// LiveAntiCheat_AnswerDvarFloatRequest: reference bool; inline candidate.
// LiveAntiCheat_AnswerExpressionRequest: reference bool; inline candidate.

// Store candidates: one 32-bit argument, no consumed return value.
// The void return types are placeholders, not independently proven.
void LiveAntiCheat_AnswerChallengesRetry(ControllerIndex_t);
void LiveAntiCheat_ResetChallengeResponse(ControllerIndex_t);

// The success callback is matched by registration. A read-only inspection
// of the SHA-256-pinned original Store MP PE independently confirms a
// pointer-sized first input and an explicit 8-bit true return at its only
// normal exit. This supports a bool callback prototype; TaskRecord remains
// a reference-derived semantic type, not a verified x64 layout.
// The short callback body has multiple normalized matches in each Store
// executable; no individual ZM/SP identity is asserted on this basis.
// The failure callback retains its prior candidate status.
bool LiveAntiCheat_AnswerChallengesSuccess(TaskRecord *);
bool LiveAntiCheat_AnswerChallengesFailure(TaskRecord *);

// Store candidate has two 64-bit argument slots and no recovered result.
// The by-value reference wrapper's precise x64 ABI remains unverified.
void LiveAntiCheat_OnChallengesReceived(unsigned long long,
    bdReference<bdAntiCheatChallenges>);

// The generated matcher labels a Store candidate AnswerChallenges (0.75).
// The original MP PE confirms a 32-bit first input. Its initial unwind
// range is not the complete function: two further ranges chain to it.
// A matching prologue and unwind grouping are present in ZM and SP, but
// this is structural correspondence, not a proven semantic match.
// No consumed return value has been established; that does not prove void.
// This may be a merged/inlined body.
// LiveAntiCheat_AnswerChallenges: reference TaskRecord *; Store return unresolved.
// LiveAntiCheat_ProcessChallenges: candidate void(ControllerIndex_t);
// do not assert a separate entry for both names.

// Reporting:
// LiveAntiCheat_ClearGameReport: reference void; Store return unresolved.
// LiveAntiCheat_SetThresholdExceeded: reference void; Store return unresolved.
// LiveAntiCheat_ReportLaggedDamage: reference void; Store return unresolved.
// LiveAntiCheat_SetGameReportTags: Store candidate accepts two pointer-sized
// arguments; the reference's third argument is not established in this build.
// Reference return is void; exact Store return contract remains unresolved.
#endif // noncompiled Store MP signature inventory
