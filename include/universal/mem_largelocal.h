// universal/mem_largelocal.cpp: large temporary buffers ("large locals") from one 512 KB static block. The main
// thread allocates from the start up and the server thread from the end down; each LargeLocal frees its buffer when it
// goes out of scope, so they nest like the stack variables they stand in for.
#pragma once

class LargeLocal
{
public:
	explicit LargeLocal(int sizeParam);
	~LargeLocal();
	void *GetBuf();

private:
	int startPos;  // the main thread's offset of the buffer, or the server thread's end of it
	int size;
};
static_assert(sizeof(LargeLocal) == 8, "LargeLocal");

int LargeLocalBegin(int size);
void LargeLocalEnd(int startPos);
int LargeLocalBeginRight(int size);
void LargeLocalEndRight(int startPos);
void *LargeLocalGetBuf(int startPos, int size);
void LargeLocalReset();
