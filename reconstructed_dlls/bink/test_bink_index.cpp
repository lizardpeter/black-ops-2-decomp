// Test original-code-derived container index reader against three real,
// SHA-256-pinned BO2 .bik files. No codec or decoder library involved.
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "bink_index.h"

struct Expected {
    std::uint32_t frames, first_offset, last_offset;
};
static std::vector<std::uint8_t> load(const char* name) {
    std::ifstream stream(name, std::ios::binary);
    assert(stream && "fixture missing; run tools/dlls/fetch_bik_fixtures.py");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
static void write32(std::vector<std::uint8_t>& bytes, std::size_t pos, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) bytes.at(pos + i) = std::uint8_t(v >> (8 * i));
}
static void check(const char* path, Expected e) {
    auto data = load(path);
    t6_bink::HeaderPrefix h{};
    assert(t6_bink::validate_header_prefix(data.data(), data.size(), &h));
    assert(h.audio_track_count == 0 && h.frame_count == e.frames);
    std::vector<t6_bink::FrameSpan> frames(h.frame_count);
    assert(t6_bink::read_video_only_index(data.data(), data.size(), frames.data(),
                                          frames.size(), &h));
    assert(frames.front().start == e.first_offset);
    assert(frames.back().end == e.last_offset);
    assert(frames.back().end == data.size());
    std::size_t keyframes = 0;
    for (std::size_t i = 0; i < frames.size(); ++i) {
        assert(frames[i].start < frames[i].end);
        if (i) assert(frames[i - 1].end == frames[i].start);
        keyframes += frames[i].keyframe;
    }
    assert(keyframes == 1 && frames.front().keyframe);
    std::cout << path << ": " << frames.size() << " compressed frame spans, "
              << keyframes << " keyframe\n";

    // Reject malformed indexes without reading outside input data.
    auto broken = data;
    write32(broken, 48, 0); // End of first frame points before its start
    assert(!t6_bink::read_video_only_index(broken.data(), broken.size(),
                                           frames.data(), frames.size()));
    broken = data;
    write32(broken, 44 + 4 * h.frame_count, static_cast<std::uint32_t>(data.size() + 4));
    assert(!t6_bink::read_video_only_index(broken.data(), broken.size(),
                                           frames.data(), frames.size()));
    assert(!t6_bink::read_video_only_index(data.data(), data.size(),
                                           frames.data(), frames.size() - 1));
    assert(!t6_bink::read_video_only_index(data.data(), 44 + 4 * h.frame_count,
                                           frames.data(), frames.size()));
    broken = data;
    write32(broken, 40, 1); // This tested helper intentionally rejects audio tracks
    assert(!t6_bink::read_video_only_index(broken.data(), broken.size(),
                                           frames.data(), frames.size()));
}
int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: test_bink_index la_pip_seq_6 karma_security yemen_drone\n";
        return 2;
    }
    check(argv[1], {150, 648, 335164});
    check(argv[2], {114, 504, 422748});
    check(argv[3], {172, 736, 517352});
}
