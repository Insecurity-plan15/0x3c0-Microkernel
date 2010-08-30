#ifndef DataStructures_Processes_H
#define DataStructures_Processes_H

#define bool char
#define true 1
#define false 0

typedef struct
{
	uint32_t Tid;
	unsigned int EIP;
	unsigned int Status;
} __attribute__((packed)) Thread;

typedef struct
{
	long long MemoryUsed;
	unsigned int SharedPageCount;
} __attribute__((packed)) MemoryUsageData;

typedef struct
{
	char Name[32];
	bool SupportsInterrupts;
	unsigned short IRQ;
	unsigned int Type;
} __attribute__((packed)) DriverInfoBlock;

typedef struct
{
	char Name[32];
	uint32_t Pid;
	DriverInfoBlock *Driver;
	uint32_t State;
	MemoryUsageData *MemoryUsage;
	uint32_t ThreadCount;
	Thread *Threads;
} __attribute__((packed)) Process;

typedef struct
{
	uint32_t Count;
	Process *Processes;
} __attribute__((packed)) ProcessList;

#endif
