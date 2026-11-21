// qcommon/bitarray.h: fixed-size bit arrays, bit 0 the most significant bit of the first word.
#pragma once

#include "t6_assert.h"

#define BITARRAY_H "c:\\t6\\code\\src\\universal\\../qcommon/bitarray.h"  // as universal/q_shared.h includes it

template <int BIT_COUNT>
class bitarray
{
public:
	enum { BITS_PER_WORD = 32, WORD_COUNT = (BIT_COUNT + BITS_PER_WORD - 1) / BITS_PER_WORD };

	int array[WORD_COUNT];

	bool testBit(unsigned int pos) const
	{
		assertexpr_in(BITARRAY_H, 109, pos < BIT_COUNT);
		return (array[pos / BITS_PER_WORD] & (0x80000000 >> (pos & (BITS_PER_WORD - 1)))) != 0;
	}

	void resetBit(unsigned int pos)
	{
		assert_in(BITARRAY_H, 136, pos < BIT_COUNT);
		array[pos / BITS_PER_WORD] &= ~(0x80000000 >> (pos & (BITS_PER_WORD - 1)));
	}
};
