#pragma once
#include "entropy_bytes.h"
#include <cstdint>

namespace t6_bink::internal {

// Original Bink 32-bit LSB-first bitstream semantics. Ghidra's code keeps
// combined incoming words in a 64-bit register until AFTER consuming each
// Huffman symbol. The stored cache, however, is only 32-bit.
// A Huffman lookahead may inspect the next 32-bit word WITHOUT advancing the
// cursor if a shorter code fits entirely inside the current reservoir.
class BitCursor {
    WordBitStream& state_;
    std::uint64_t pending_;
    std::uint32_t available_;
    const std::uint32_t* next_;
public:
    explicit BitCursor(WordBitStream& state)
        : state_(state), pending_(state.cache),
          available_(state.available_bits), next_(state.next_word) {}

    std::uint32_t available() const { return available_; }
    const std::uint32_t* next() const { return next_; }
    void refill() {
        pending_ |= std::uint64_t(*next_++) << available_;
        available_ += 32;
    }
    std::uint32_t lookahead(std::uint32_t width) const {
        std::uint64_t value = pending_;
        if (available_ < width)
            value |= std::uint64_t(*next_) << available_;
        return static_cast<std::uint32_t>(value) &
               ((std::uint32_t{1} << width) - 1U);
    }
    void consume(std::uint32_t width) {
        if (available_ < width) refill();
        pending_ >>= width;
        available_ -= width;
    }
    std::uint32_t read(std::uint32_t width) {
        if (available_ < width) refill();
        const auto result = static_cast<std::uint32_t>(pending_) &
                            ((std::uint32_t{1} << width) - 1U);
        consume(width);
        return result;
    }
    void save() {
        state_.cache = static_cast<std::uint32_t>(pending_);
        state_.available_bits = available_;
        state_.next_word = next_;
    }
};
} // namespace t6_bink::internal
