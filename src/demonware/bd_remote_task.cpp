#ifdef T6_STORE
#include "demonware/bd_reference.h"
#include <string.h>

extern "C" void BdPlatformLog(int, const char *, const char *, const char *, const char *, int, const char *, ...);
extern "C" void BdFree(void *);

namespace
{
struct ByteBuffer
{
	BdReferencable reference;
	const unsigned char *data;
	unsigned size;
	const unsigned char *cursor;
};

struct RemoteTask
{
	BdReferencable reference;
	unsigned char beforeState[0x1c - sizeof(BdReferencable)];
	unsigned state;
	BdReferencable *reply;
	unsigned char beforeTransaction[0x48 - 0x28];
	unsigned long long transaction;
};

struct TaskNode
{
	RemoteTask *task;
	TaskNode *next, *previous;
};

struct TaskQueue
{
	TaskNode *head, *tail;
	unsigned size;
};

struct TaskManager
{
	void *prefix;
	TaskQueue pending;
	unsigned char beforeConnection[0x70 - 0x20];
	unsigned long long connectionId;
};

static_assert(__builtin_offsetof(ByteBuffer, cursor) == 0x20);
static_assert(__builtin_offsetof(RemoteTask, state) == 0x1c);
static_assert(__builtin_offsetof(RemoteTask, transaction) == 0x48);
static_assert(__builtin_offsetof(TaskManager, connectionId) == 0x70);

void ReleaseOwned(BdReferencable *&reference)
{
	if (reference && reference->release())
		reference = nullptr;
}
}

extern "C" bool BdBytePackerRemoveBuffer(const unsigned char *source, unsigned sourceSize, unsigned offset,
                                         unsigned *next, void *destination, unsigned size)
{
	const unsigned end = offset + size;
	*next = end;
	if (!destination || !source)
		return true;
	if (sourceSize < end)
	{
		BdPlatformLog(1, "warn/", "byte packer",
		              R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdCore\bdUtilities\bdBytePacker.cpp)",
		              "bdBytePacker::removeBuffer", 106, "Not enough data left to read %u bytes.", size);
		return false;
	}
	if (offset > sourceSize)
		return false;
	memmove(destination, source + offset, size);
	return true;
}

extern "C" unsigned char BdByteBufferRead(ByteBuffer *buffer, void *destination, unsigned size)
{
	const auto available = buffer->size + static_cast<unsigned>(reinterpret_cast<__UINTPTR_TYPE__>(buffer->data)) -
		static_cast<unsigned>(reinterpret_cast<__UINTPTR_TYPE__>(buffer->cursor));
	if (size <= available)
	{
		unsigned next;
		const bool result = BdBytePackerRemoveBuffer(buffer->cursor, available, 0, &next, destination, size);
		buffer->cursor += size;
		return result;
	}
	BdPlatformLog(1, "warn/", "core/bytebuffer",
	              R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdCore\bdContainers\bdByteBuffer.cpp)",
	              "bdByteBuffer::read", 368, "Could not read data from buffer. Insufficient data available.\n");
	return 0;
}

extern "C" void BdRemoteTaskAcceptReply(RemoteTask *task, BdReferencable **reply, unsigned long long transaction)
{
	task->state = 2;
	if (&task->reply != reply)
	{
		ReleaseOwned(task->reply);
		task->reply = *reply;
		if (task->reply)
			++task->reply->count;
	}
	task->transaction = transaction;
	ReleaseOwned(*reply);
}

extern "C" void BdPendingTaskPop(TaskQueue *queue)
{
	auto *node = queue->head;
	if (!node)
		return;
	queue->head = node->next;
	if (node == queue->tail)
		queue->tail = node->previous;
	else
		node->next->previous = node->previous;
	if (node->task && node->task->reference.release())
		node->task = nullptr;
	BdFree(node);
	--queue->size;
}

extern "C" void BdRemoteTaskHandleReply(TaskManager *manager, BdReferencable **reply)
{
	constexpr char source[] =
		R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdLobby\bdRemoteTaskManager\bdRemoteTaskManager.cpp)";
	auto *task = manager->pending.head->task;
	if (task)
	{
		++task->reference.count;
		using Status = int (*)(RemoteTask *);
		if (reinterpret_cast<Status>(task->reference.vtable[1])(task) == 1)
		{
			unsigned long long transaction;
			const auto read = BdByteBufferRead(reinterpret_cast<ByteBuffer *>(*reply), &transaction, sizeof(transaction));
			if (read)
			{
				if (read != 1)
					transaction = 0;
				auto *ownedReply = *reply;
				if (ownedReply)
					++ownedReply->count;
				BdRemoteTaskAcceptReply(task, &ownedReply, transaction);
				BdPlatformLog(0, "info/", "lobby/remotetaskmanager", source, "bdRemoteTaskManager::handleTaskReply", 173,
				              "Received reply on\nconnection ID : %I64u\ntransaction ID: %I64u", manager->connectionId, transaction);
			}
			else
				BdPlatformLog(1, "warn/", "lobby/remotetaskmanager", source, "bdRemoteTaskManager::handleTaskReply", 177,
				              "Failed to retrieve transaction ID");
		}
	}
	BdPendingTaskPop(&manager->pending);
	if (task)
		task->reference.release();
	ReleaseOwned(*reply);
}
#endif
