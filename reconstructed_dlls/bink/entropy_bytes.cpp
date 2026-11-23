// Machine-code-derived reconstruction of Bink's Huffman byte block decoder.
// FUN_180024980, RVA 0x24980, pinned original Store binkw64.dll:
// SHA256 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// The original first reads a short bit-coded block length, then selects either
// a literal nibble repeated for the whole block or a nibble-indexed Huffman
// lookahead table with symbol map. This implementation preserves the actual
// 0x48-byte context and 24-byte bitreader layouts, the LSB-first 32-bit word
// refill, the 4-bit repeated-value branch, and the original 0x30/0x40 state
// offsets. No library decoder, compression library, or third-party source used.
//
// Precondition: caller supplies the original valid decoder configuration,
// aligned padded input words, <=31 count/lookahead bits, code lengths 1..15,
// enough input for the decoded output, and an output buffer >=capacity+4.
// Deliberately not a safe parser for untrusted arbitrary files by itself.
#include "entropy_bytes.h"
#include <cstdint>

namespace {
using t6_bink::WordBitStream;

// Original decoder keeps the combined incoming word in a 64-bit register
// UNTIL after the code bits have been shifted out. Truncating to 32 bits
// immediately on refill loses high bits of the new word at cross-word
// boundaries; the exported bitstream cache is only narrowed on write-back.
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
  std::uint32_t peek(std::uint32_t width) {
      if (available_ < width) refill();
      return static_cast<std::uint32_t>(pending_) &
             ((std::uint32_t{1} << width) - 1U);
  }
  void consume(std::uint32_t width) {
      pending_ >>= width;
      available_ -= width;
  }
  std::uint32_t read(std::uint32_t width) {
      const auto result = peek(width);
      consume(width);
      return result;
  }
  void save() {
      state_.cache = static_cast<std::uint32_t>(pending_);
      state_.available_bits = available_;
      state_.next_word = next_;
  }
};
}

extern "C" void bink_decode_entropy_bytes(t6_bink::ByteEntropyState* d,
                                           t6_bink::WordBitStream* bits,
                                           const std::uint32_t* word_end) {
    if (d->current != d->end) return;

    BitCursor cursor(*bits);
    const std::uint32_t count = cursor.read(d->count_bits);
    if (count == 0 || count > d->capacity) {
        d->current = d->buffer + 4;
        d->end = d->buffer;
        cursor.save();
        return;
    }
    d->current = d->buffer;
    d->end = d->buffer + count;

    const bool repeat = cursor.read(1) != 0;
    if (repeat) {
        const std::uint8_t literal = static_cast<std::uint8_t>(cursor.read(4));
        for (std::uint32_t i = 0; i < count; ++i) d->buffer[i] = literal;
        cursor.save();
        return;
    }

    for (std::uint32_t i = 0; i < count; ++i) {
        // The original takes this boundary check before a necessary lookahead
        // refill. Its higher-level video decoder is responsible for providing
        // safe padding for the other bit consumption paths.
        if (cursor.available() < d->lookup_bits && cursor.next() >= word_end) {
            d->current = d->buffer + 4;
            d->end = d->buffer;
            return;
        }
        const std::uint8_t packed = d->lookup_table[cursor.peek(d->lookup_bits)];
        const std::uint32_t code_length = packed >> 4;
        const std::uint8_t code_symbol = packed & 15U;
        if (cursor.available() < code_length) cursor.refill();
        cursor.consume(code_length);
        d->buffer[i] = d->symbol_map[code_symbol];
    }
    cursor.save();
}
