// universal/com_workercmds.cpp: the job queue's worker commands.
#pragma once

// A job queue group: batches queued and executing (atomic as one 64-bit read)
struct jqBatchGroup
{
	union
	{
		struct
		{
			int QueuedBatchCount;
			int ExecutingBatchCount;
		};
		unsigned long long BatchCount;
	};
};

enum jqWorkerType
{
	JQ_WORKER_GENERIC = 0,
};

struct jqBatch;

// A kind of job (0x28 bytes on x86, 0x30 on x64)
struct jqModule
{
	const char *Name;
	jqWorkerType Type;
	int (*Code)(jqBatch *);
	jqBatchGroup Group;
	unsigned long long Ticks;
	unsigned short Calls;
	unsigned short Requeues;
};

// Whether the group (or the pool's default group, for null) still has queued or executing batches
bool jqPoll(jqBatchGroup *GroupID);

int R_FinishedFrontendWorkerCmds();
