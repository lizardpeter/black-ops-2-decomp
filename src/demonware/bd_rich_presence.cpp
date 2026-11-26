#ifdef T6_STORE
#include "demonware/bd_reference.h"

extern "C" void *BdAllocate(__SIZE_TYPE__);
extern "C" void BdPlatformLog(int, const char *, const char *, const char *, const char *, int, const char *, ...);
extern "C" BdReferencable *BdConstructTaskByteBuffer(void *, unsigned, bool);
extern "C" void BdInitializeTaskBuffer(BdReferencable **, unsigned char, unsigned char);
extern "C" bool BdByteBufferWriteUInt64(BdReferencable *, unsigned long long);
extern "C" bool BdByteBufferWriteBlob(BdReferencable *, const void *, unsigned);
extern "C" int BdStartRemoteTask(void *, void **, BdReferencable **);
extern "C" void BdSetRemoteTaskResult(void *, void *, unsigned);

namespace
{
constexpr char source[] =
	R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdLobby\bdRichPresence\bdRichPresenceService.cpp)";
constexpr unsigned char richPresenceService = 0x44;

struct RichPresenceInfo
{
	unsigned char prefix[9];
	unsigned char data[0x2c - 9];
	unsigned size;
};

static_assert(__builtin_offsetof(RichPresenceInfo, size) == 0x2c);

BdReferencable *CreateTaskBuffer(unsigned capacity)
{
	auto *storage = BdAllocate(0x50);
	if (!storage)
		return nullptr;
	auto *buffer = BdConstructTaskByteBuffer(storage, capacity, true);
	if (buffer)
		++buffer->count;
	return buffer;
}
}

extern "C" void **BdRichPresenceSetInfo(void **service, void **task, const RichPresenceInfo *presence)
{
	auto *buffer = CreateTaskBuffer(0x6a);
	*task = nullptr;
	BdInitializeTaskBuffer(&buffer, richPresenceService, 1);
	constexpr char function[] = "bdRichPresenceService::setInfo";
	if (BdByteBufferWriteUInt64(buffer, 0) && BdByteBufferWriteBlob(buffer, presence->data, presence->size))
	{
		const int error = BdStartRemoteTask(*service, task, &buffer);
		if (error)
			BdPlatformLog(1, "warn/", "richPresence", source, function, 48, "Failed to start task: Error %i", error);
	}
	else
		BdPlatformLog(1, "warn/", "richPresence", source, function, 53, "Failed to write param into buffer");

	if (buffer)
		buffer->release();
	return task;
}

extern "C" void **BdRichPresenceGetInfo(void **service, void **task, const unsigned long long *users,
                                        void *results, int count)
{
	auto *buffer = CreateTaskBuffer(0x40u + 9u * static_cast<unsigned>(count));
	*task = nullptr;
	BdInitializeTaskBuffer(&buffer, richPresenceService, 2);
	constexpr char function[] = "bdRichPresenceService::getInfo";
	bool serialized = true;
	for (int i = 0; i < count && serialized; ++i)
		serialized = BdByteBufferWriteUInt64(buffer, users[i]);
	if (serialized)
	{
		const int error = BdStartRemoteTask(*service, task, &buffer);
		if (error)
			BdPlatformLog(1, "warn/", nullptr, source, function, 78, "Failed to start task: Error %i", error);
		else
			BdSetRemoteTaskResult(*task, results, static_cast<unsigned>(count));
	}
	else
		BdPlatformLog(1, "warn/", nullptr, source, function, 87, "Failed to write param into buffer");

	if (buffer)
		buffer->release();
	return task;
}
#endif
