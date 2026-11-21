// qcommon/bitarray.h: fixed-size bit arrays, bit 0 the most significant bit of the first word.
#pragma once

#include "t6_assert.h"

template <int BIT_COUNT>
class bitarray
{
public:
	enum { BITS_PER_WORD = 32, WORD_COUNT = (BIT_COUNT + BITS_PER_WORD - 1) / BITS_PER_WORD };

	int array[WORD_COUNT];

	bool testBit(unsigned int pos) const
	{
		assertexpr_in(T6_ASSERT_DIR "../qcommon/bitarray.h", 109, pos < BIT_COUNT);
		return (array[pos / BITS_PER_WORD] & (0x80000000 >> (pos & (BITS_PER_WORD - 1)))) != 0;
	}
};
