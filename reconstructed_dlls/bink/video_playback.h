#pragma once
#include "bink_index.h"
#include "video_packet.h"
#include <array>
#include <span>
#include <string>
#include <vector>

namespace t6_bink {
// Source-only sequential BIKi playback for trusted Store fixtures.
// Not the original BINK handle ABI or an untrusted-media parser.
// Plane views are borrowed; valid only until next decode, reset or open.
class VideoPlayback {
public:
    enum class Result { frame, end, error };
    bool open(std::vector<std::uint8_t> file);
    bool reset();
    Result decode_next();
    const HeaderPrefix& header() const { return header_; }
    std::uint32_t frames_decoded() const { return next_; }
    bool current_keyframe() const;
    std::span<const std::uint8_t> plane(unsigned p) const;
    // Storage dimensions (original BinkGetFrameBuffersInfo round-to-8).
    // Visible dimensions remain in header().
    unsigned plane_count() const { return !opened_ ? 0 : (header_.flags&kAlphaFlag ? 4 : 3); }
    unsigned plane_width(unsigned p) const;
    unsigned plane_height(unsigned p) const;
    const std::string& error() const { return error_; }

private:
    bool fail(std::string reason);
    HeaderPrefix header_{};
    std::vector<std::uint8_t> file_;
    std::vector<FrameSpan> index_;
    std::array<std::array<std::vector<std::uint8_t>,4>,2> buffers_;
    std::uint32_t next_=0;
    bool opened_=false, failed_=false;
    std::string error_;
};
}
