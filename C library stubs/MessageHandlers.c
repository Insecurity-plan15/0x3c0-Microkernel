#include "Microkernel.h"

void recycleMTID(uint32_t mtid)
{
	//Return the used message ID to the available pool
	if(mtid == _maxMThread - 1)
		_maxMThread--;
	else
	{
		uint32_t *newMTIDS = (uint32_t *)realloc(_freeMThreadIDs, sizeof(uint32_t *) * (_freeMThreadCount + 1));

		if(newMTIDS != 0)
		{
			newMTIDS[_freeMThreadCount++] = mtid;
			_freeMThreadIDs = newMTIDS;
		}
	}
}

void kernelHandler(Message *m)
{
	//handle wait requests here
	if(m->Code == MSG_CHILD_STATE_CHANGE)
	{
		int i = 0;

		for(i = 0; i < _waitCount; i++)
		{
			WaitData *wd = _waitDetails[i];
			uint64_t *data = (uint64_t *)m->Data;

			if(m->MessageThread == wd->waitingForMId)
			{
				//Set the fields based on the message contents
				wd->statusCode = *data;

				//Wake up the thread and return the MTID to the pool
				asm volatile ("int $0x30" : : "a"(wd->wakeThreadId), "d"(16));
				recycleMTID(m->MessageThread);
			}
		}
	}
}

void vfsHandler(Message *m)
{
	int i = 0;
	int j = 0;

    for(i = 0; i < _handleCount; i++)
    {
        FileHandle *fh = _handles[i];

        //need to iterate through each operation on the file
        for(j = 0; j < fh->operationCount; j++)
        {
            FileOperation *op = fh->pendingOperations[j];

            if(op->waitingForID == m->MessageThread)
            {
                op->operationWakeData = (uintptr_t)m->Data;

                //Yes, it's a busy loop. I hate this thing, but it shouldn't be for too long. It's mostly
                //just a last resort, if the VFS is able to get everything from the cache and respond very
                //quickly. AKA, quickly enough to cause a deadlock without it
                while(op->wakeThread == 0)
                    ;
				//Wake up the thread
                asm volatile ("int $0x30" : : "a"(op->wakeThread), "d"(16));
                //Make sure that the message thread ID is returned to the pool
				recycleMTID(m->MessageThread);
            }
        }
    }
}

void timerHandler(Message *m)
{
	TimerDataHandler *td = 0;
	uint32_t i = 0;

	for(i = 0; i < _timerCount; i++)
	{
		td = _timerDetails[i];

		if(td->waitingForMId == m->MessageThread)
		{
			td->data = (TimerData *)m->Data;
			asm volatile ("int $0x30" : : "a"(td->wakeThreadId), "d"(16));
			recycleMTID(m->MessageThread);
		}
	}
}

void loaderHandler(Message *m)
{
	if(m->Code == MSG_RETURN_MEMORY_DESCRIPTORS)
	{
		load->eip = *((uint32_t *)(m->Data));
		//Fill in the loading structure so that the kernel can know about the important data
		load->memBlocks = (MemoryBlock *)(m->Data + sizeof(uint32_t));
		load->memBlockCount = (m->Length - sizeof(uint32_t)) / sizeof(MemoryBlock);

		asm volatile ("int $0x40" : : "a"(load), "d"(1));
	}
}
