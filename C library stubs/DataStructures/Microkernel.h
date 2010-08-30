#ifndef DataStructures_Microkernel_H
#define DataStructures_Microkernel_H

#include <signal.h>
#include <stdint.h>

typedef struct
{
	void *Data;
	unsigned int Length;
	unsigned int Code;
	//When a message is being sent, Source is ignored by the kernel
	unsigned int Source;

    //I use this to identify a collection of messages
	unsigned int MessageThread;
} __attribute__((packed)) Message;

typedef void (*MessageHandler)(Message *m);

typedef struct
{
	uint32_t Start;
	uint32_t End;
	uint32_t CopyTo;
} __attribute__((packed)) MemoryBlock;

typedef struct
{
	void *location;
	uint32_t size;
} __attribute__((packed)) ParameterDescriptor;

typedef struct
{
	uint32_t eip;

	MemoryBlock *memBlocks;
	uint32_t memBlockCount;

	ParameterDescriptor *params;
	uint32_t paramCount;
} __attribute__((packed)) LoadingDataBlock;

typedef struct
{
    //POSIX compatibility - the Epoch is defined to be 1 January 1970
    uint64_t secondsSinceEpoch;
    long microsecondsSinceEpoch;
    //New stuff
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;
    uint64_t year;

    //This is the offset between local time and GMT in minutes (east/west)
    int32_t offset;
    //If bit zero is set, the country is currently in Daylight Saving Time
    uint8_t flags;
} __attribute__((packed)) TimerData;

typedef struct
{
    uint32_t nativePid;
    MessageHandler handler;
} ProcessData;

typedef struct
{
	TimerData *data;
	uint32_t wakeThreadId;
	//This is auto-assigned from the list of available MTIDs
	uint32_t waitingForMId;
} TimerDataHandler;

typedef struct
{
	uint64_t statusCode;

	uint32_t waitingForMId;
	uint32_t wakeThreadId;
} __attribute__((packed)) WaitData;

#endif
