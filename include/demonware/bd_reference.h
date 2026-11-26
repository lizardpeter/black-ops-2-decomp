#pragma once

struct BdReferencable
{
	void **vtable;
	unsigned count;

	bool release()
	{
		if (--count)
			return false;
		reinterpret_cast<void (*)(BdReferencable *, unsigned)>(vtable[0])(this, 1);
		return true;
	}
};
