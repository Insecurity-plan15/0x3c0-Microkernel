#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>

#include <sys/errno.h>
#include <stdio.h>

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/times.h>
#include <sys/time.h>

#include "DataStructures/Microkernel.h"
#include "DataStructures/Processes.h"
#include "SystemCalls.h"
#include "Microkernel.h"

#include "MessageHandlers.h"

int addPOSIXPid(uint32_t native);
FileOperation *sendMessageToVFS(int fh, uint32_t data, uint32_t length, uint32_t code, uint8_t flags);
int getNextMTID();

int addPOSIXPid(uint32_t native)
{
    int idx = getLinuxPID(native);
    ProcessData **tmp;
    int i = 0;

    //If the POSIX pid already exists, return it
    if(idx != -1)
        return idx;
    //Otherwise, find a neat spot to put it and return that
    for(i = 0; i < _pidCount; i++)
    {
        if(_pids[i] == 0)
        {
            _pids[i] = malloc(sizeof(ProcessData));
            _pids[i]->nativePid = native;
            return i;
        }
    }
    tmp = realloc(_pids, (_pidCount + 1) * sizeof(ProcessData *));
    if(tmp != 0)
        _pids = tmp;
    _pids[_pidCount] = malloc(sizeof(ProcessData));
    _pids[_pidCount]->nativePid = native;
    _pidCount++;
    return _pidCount - 1;
}

uint32_t getVFS()
{
    uint32_t *returnData = (uint32_t *)0;

    asm volatile ("int $0x30" : "=b"(returnData) : "a"(0x1), "d"(17));
    return (returnData == (uint32_t *)0 ? 0 : *returnData);
}

//flags:
    //Bit 0 set = send data, not just a code
void sendMessage(uint32_t destination, uint32_t data, uint32_t length, uint32_t code, uint32_t mID, uint8_t flags)
{
    uint32_t messageContents[5];

    messageContents[0] = ((flags & 1) == 0) ? 0 : data;
    messageContents[1] = ((flags & 1) == 0) ? 0 : length;
    messageContents[2] = code;
    messageContents[3] = 0;
    messageContents[4] = mID;
    asm volatile ("int $0x30" : : "a"(messageContents), "b"(destination), "d"(4));
}

//flags:
    //Bit 0 set = send data, not just a code
//This is a synchronous method, designed for POSIX-related monstrosities. It should also scale to multi-threaded POSIX-related
//monstrosities
FileOperation *sendMessageToVFS(int fh, uint32_t data, uint32_t length, uint32_t code, uint8_t flags)
{
	FileOperation *op = malloc(sizeof(FileOperation));

	if(fh >= 0)
	{
		uint32_t mID = getNextMTID();
		FileHandle *handle = _handles[fh];

		//Tack the current request onto the list of pending operations for that file
		handle->pendingOperations = realloc(handle->pendingOperations, (handle->operationCount + 1) * sizeof(FileOperation *));
		handle->pendingOperations[handle->operationCount++] = op;

		op->waitingForID = mID;
		//Send the message
		sendMessage(getVFS(), data, length, code, mID, flags);
		//It's possible that a response arrives quickly, causing the thread to sleep anyway, causing a deadlock. This gets
		//handled in the VFS handler

		//One final check to make sure that this doesn't get put to sleep unnecessarily. Won't prevent a deadlock on its own though
		if(op->operationWakeData == 0)
		{
			asm volatile ("int $0x30" : "=a"(op->wakeThread) : "d"(15));
			//Put the thread to sleep for a little while - the VFS handler will wake it up
			asm volatile ("int $0x30" : : "d"(15));
		}
	}
	//If a new file handle needs creating, do so and recurse
	else
	{
		int max = _handleCount + 1;
		FileHandle **newList = realloc(_handles, max * sizeof(FileHandle *));

		if(newList == 0)
		{
			free(op);
			op = 0;
		}
		newList[_handleCount] = malloc(sizeof(FileHandle));
		_handles = newList;
		return sendMessageToVFS(_handleCount++, data, length, code, flags);
	}
	return op;
}

int getNextMTID()
{
	uint32_t mID = 0;

	if(_freeMThreadCount != 0)
    {
        uint32_t maxIdx = _freeMThreadCount - 1;
        uint32_t *tids = realloc(_freeMThreadIDs, maxIdx * sizeof(uint32_t));

        //If the reallocation worked, just put the list in its proper place
        if(tids != 0)
            _freeMThreadIDs = tids;
        mID = _freeMThreadIDs[maxIdx];
        _freeMThreadCount--;
    }
    else
        mID = _maxMThread++;
	return mID;
}

void _exit(int status)
{
    asm volatile ("int $0x30" : : "a"(0), "b"(status), "d"(3));

	for ( ; ; ) ;
}

int close(int file)
{
    if(file < 0)
    {
        errno = EBADF;
        return -1;
    }
    if(_handles == 0 || file > _handleCount)
    {
        errno = EBADF;
        return -1;
    }
    sendMessageToVFS(file, _handles[file]->absoluteHandle, sizeof(_handles[file]->absoluteHandle), MSG_CLOSE_STREAM, 1);
    return 0;
}

int execve(const char *name, char * const argv[], char * const env[])
{
    const char *loaderName = "processLoader";
    ProcessList *pl = getProcesses();
    uint32_t argC = 0;
    uint32_t i = 0;

    //argV is terminated by a null pointer. Calculate argC from that
    for(; argv[argC] != 0; argC++)
		;
	//Set up the parameter count
	load->paramCount = argC;
	load->params = malloc(argC * sizeof(ParameterDescriptor));
	//Tell the structure about the data
	for(i = 0; i < argC; i++)
	{
		load->params[i].location = (void *)argv[i];
		load->params[i].size = strlen(argv[i]);
	}

	for(i = 0; i < pl->Count; i++)
	{
		Process p = pl->Processes[i];

		if(strcmp(p.Name, loaderName) == 0)
		{
			installMessageHandler(p.Pid, 1, loaderHandler);
			sendMessage(p.Pid, (uint32_t)name, strlen(loaderName), MSG_LOAD_FILE, 0, 0);
			//Just loop infinitely. The return message from the loading process will trigger a handler which loads the memory image and process state
			for( ; ; ) ;
		}
	}
	//If the loading process doesn't exist, there are problems
	errno = ENOMEM;
	return -1;
}

int fork()
{
    unsigned int eax = 0;

    //EAX will return the native Pid. I need to convert it into a POSIX pid
    asm volatile ("int $0x40" : "=a"(eax) : "d"(0));

    //By convention, the child always gets the return value of zero
    if(eax == 0)
        return 0;
    //This is a sort-of hack. Theoretically, the kernel could kill a process and recreate one in the same place.
    //If this happens, there could be problems; to resolve this, I remove all existing entries for the native Pid
    return addPOSIXPid(eax);
}

int fstat(int file, struct stat *st)
{
    FileOperation *op = 0;
    FileData *fd = 0;

    if(file < 0)
    {
        errno = EBADF;
        return -1;
    }
    if(_handles == 0 || file > _handleCount)
    {
        errno = EBADF;
        return -1;
    }
    op = sendMessageToVFS(file, _handles[file]->absoluteHandle, sizeof(_handles[file]->absoluteHandle), MSG_GET_FILE_STATUS, 1);

    //After here, the thread's been awoken
    fd = (FileData *)(op->operationWakeData);
    st->st_dev = fd->deviceId;
    st->st_ino = fd->fileIdentifier;
    st->st_mode = fd->fileMode;
    //This is hard-coded for now. Either way, it's pretty irrelevant
    st->st_nlink = 1;
    st->st_uid = fd->userID;
    st->st_gid = fd->groupID;
    //These two will always be the same size
    st->st_rdev = st->st_dev;
    st->st_size = fd->fileSize;
    return 0;
}

int getpid()
{
    uint32_t flags = 1 << 5;
    uint32_t *data = 0;

    asm volatile ("int $0x30" : "=b"(data) : "a"(flags), "d"(17));
    if(data != 0)
    {
        uint32_t nativePid = data[1];

        return addPOSIXPid(nativePid);
    }
    return 0;
}

int isatty(int file)
{
    //Handle must be valid, and resolve to /dev/tty. The VFS has a compatibility layer for this sort of ickiness
    if( (file < _handleCount) && (file > 0) && (strcmp("/dev/tty/", (const char *)_handles[file]->name) == 0))
        return 1;
    else
    {
        errno = ENOTTY;
        return 0;
    }
}

int kill(int pid, int sig)
{
    //Just send a straight message. No data, the only thing left is a code
    uint32_t nativePid = getKernelPID(pid);

    if(nativePid == 0)
    {
        errno = ESRCH;
        return -1;
    }
    sendMessage(nativePid, 0, 0, sig, 0, 0);
    return 0;
}

int link(const char *old, const char *new)
{
    size_t oldLength = strlen(old);
    size_t newLength = strlen(new);
    //Allocate enough bytes for the old filename, new filename, and null character
    char *concatenated = malloc(oldLength + newLength + 1);

    strcpy(concatenated, (const char *)old);
    concatenated[oldLength] = '\0';
    //Concatenate the two paths together, separating them with a null character
    strcpy(&concatenated[oldLength + 1], (const char *)new);

    //Note that I don't use sendMessageToVFS here. This is because I don't have a file handle to work with,
    //and it'd be inefficient to open one
    sendMessage(getVFS(), (uint32_t)concatenated, oldLength + newLength + 1, MSG_RENAME_FS_ENTRY, 0, 1);
    return 0;
}

off_t lseek(int file, off_t ptr, int dir)
{
    int fullOffset = ptr;
    struct stat st;

    if(file < 0)
    {
        errno = EBADF;
        return -1;
    }
    if(file > _handleCount)
    {
        errno = EBADF;
        return -1;
    }
    if(dir == SEEK_CUR)
        fullOffset += _handles[file]->filePosition;
    if(dir == SEEK_END)
    {
        //Perform a quick fstat, to find out the size of the file
        fstat(file, &st);
        fullOffset += st.st_size;
    }
    _handles[file]->filePosition = fullOffset;
}

int open(const char *name, int flags, ...)
{
    int len = strlen(name);
    //Allocate a buffer which is big enough for both the string , and the flags before it
    char *buffer = malloc(len + sizeof(int));
    uint32_t *result = 0;
    FileOperation *op = 0;
    int i = 0;

    *((int *)buffer) = flags;
    strcpy(&buffer[sizeof(int)], name);

    //Need to return a file handle
    //If the file is already open, there's no need to open it again - it'll also mess up the VFS handler. Return the old handle
    for(i = 0; i < _handleCount; i++)
        if(strcmp(name, _handles[i]->name) == 0)
            return i;
    //Send the buffer containing the name and flags to the VFS. A file handle of -1 means that it doesn't exist yet, and
    //should be created
    op = sendMessageToVFS(-1, (uint32_t)buffer, len + sizeof(int), MSG_OPEN_STREAM, 1);

    //When the thread wakes up, the return value will be an array of two uint32_t's
    //If the read suceeded, the first one will contain a 'proper' file handle. If it failed, the first one will be
    //zero, and the second one will be an error code (stored as an int)
    free(buffer);
    result = (uint32_t *)op->operationWakeData;
    if(result[0] != 0)
    {
        FileHandle *fh = malloc(sizeof(FileHandle));
        char *nameBuffer = malloc(len);
        FileHandle **handles = realloc(_handles, (_handleCount + 1) * sizeof(FileHandle *));

        strcpy(nameBuffer, name);

        fh->name = nameBuffer;
        fh->absoluteHandle = result[0];
        fh->filePosition = 0;
        fh->flags = flags;
        fh->operationCount = 0;

        if(handles != 0)
        {
            handles[_handleCount] = fh;
            _handles = handles;
            //Line below is incredibly ugly, but something like 'return _handleCount++' doesn't do what would be most elegant
            return ++_handleCount - 1;
        }
        else
        {
            errno = EIO;
            free(fh);
            free(nameBuffer);
            return -1;
        }
    }
    else
    {
        errno = result[1];
        return -1;
    }
}

int read(int file, void *ptr, size_t len)
{
    FileHandle *fh = 0;
    FileOperation *op = 0;
    char *cpy = 0;
    //This is the structure which is passed to the VFS
    uint32_t structure[3];
    //This is the array of integers which is returned. DWORD 0 = address of new buffer. DWORD 1 = number of bytes read
    uint32_t *returnStructure;

    //read [len] bytes from [file] into [ptr]
    if(file < 0)
    {
        errno = EBADF;
        return -1;
    }
    if(file > _handleCount)
    {
        errno = EBADF;
        return -1;
    }
    if( (fh = _handles[file]) == 0)
    {
        errno = EBADF;
        return -1;
    }
    structure[0] = fh->absoluteHandle;
    structure[1] = len;
    structure[2] = fh->filePosition;

    op = sendMessageToVFS(file, (uint32_t)&structure, sizeof(structure), MSG_READ_STREAM, 1);
    returnStructure = (uint32_t *)op->operationWakeData;
    cpy = (char *)returnStructure[0];
    //memcpy the buffer across from the message
    memcpy(ptr, (const void *)cpy, returnStructure[1]);
    fh->filePosition += returnStructure[1];

    return (int)returnStructure[1];
}

void *sbrk(ptrdiff_t incr)
{
    #define heapStart   0xC0000000
    static caddr_t heapPosition = (caddr_t)heapStart;
    caddr_t newPosition = heapPosition + incr;
    uint32_t currentPage = (uint32_t)heapPosition & 0xFFFFF000;
    uint32_t newPage = (uint32_t)newPosition & 0xFFFFF000;
    uint32_t pageDifference = (currentPage - newPage) / PageSize;
    uint32_t i = 0;

    //A method calling sbrk with incr==0 simply wants to know the current heap position
    if(incr == 0)
        return heapPosition;
    //The heap can't get too big here. This constraint is also enforced in the kernel
    if((uint32_t)newPosition > 0xCFFFFFFF)
        return heapPosition;
    //sbrk is used for both allocation and freeing. A negative value indicates that memory should be freed
    if(newPage > currentPage)
    {
        //For every page boundary between the current position and the new position, allocate and map a page in sequence
        for(i = 0; i < pageDifference; i++)
            asm volatile ("int $0x30" : : "a"(heapPosition + (i * PageSize)), "b"(0), "d"(11));
    }
    else if(newPage < currentPage)
    {
        //The page difference will obviously need to be recalculated to take this into account
        pageDifference = (newPage - currentPage) / PageSize;
        //There should always be at least one page which acts as the heap
        for(i = 0; i < pageDifference - 1; i++)
            asm volatile ("int $0x30" : : "a"(heapPosition - (i * PageSize)), "b"(2), "d"(11));
    }
    //Update and return the start of the memory region containing the data
    heapPosition = newPosition;
    return (void *)(heapPosition - incr);
}

int stat(const char *file, struct stat *st)
{
    int handle = open(file, O_RDONLY);
    int ret = fstat(handle, st);

    close(handle);
    return ret;
}

clock_t times(struct tms *buf)
{
	TimerData *timer = getCurrentTime();

    if(buf != 0)
    {
        buf->tms_utime = 0;
        buf->tms_stime = 0;
        buf->tms_cutime = 0;
        buf->tms_cstime = 0;
    }
    return timer->secondsSinceEpoch;
}

int unlink(const char *name)
{
    sendMessage(getVFS(), (uint32_t)name, strlen(name), MSG_DELETE_FS_ENTRY, 0, 1);
    return 0;
}

int wait(int *status)
{
	uint32_t tID = 0;
	uint32_t pid = 0;
	WaitData *data;
	WaitData **newList;
	uint32_t i = 0;

	asm volatile ("int $0x30" : "=a"(tID) : "d"(15));
	//If a process has already changed state, return immediately.
	for(i = 0; i < _waitCount; i++)
	{
		if(_waitDetails[i]->wakeThreadId == tID)
			data = _waitDetails[i];
	}

	if(data == 0)
	{
		newList = realloc(_waitDetails, (_timerCount + 1) * sizeof(WaitData *));

		data = malloc(sizeof(WaitData));
		newList[_timerCount++] = data;
		data->waitingForMId = getNextMTID();
		data->wakeThreadId = tID;

		_waitDetails = newList;
		if(data->statusCode == 0)
			asm volatile ("int $0x30" : : : "ebx");
	}

	pid = (uint32_t)((data->statusCode >> 32) & 0xFFFFFFFF);
	if(status != NULL)
	{
		//If the exit type is 0x2, then the child process died. The bottom 16 bits contain the exit value
		if((data->statusCode & 0xFF00) == 0x2)
			*status = 0x0 | (data->statusCode & 0xFF);
		//If the exit type is 0, then the child has gone to sleep. I don't implement signals in the normal way, so the signal number is 0
		else if ((data->statusCode & 0xFF00) == 0x0)
			*status = 0x7F | 0x0;
	}
	//Return the POSIX Pid
	return addPOSIXPid(pid);
}

int write(int file, const void *ptr, size_t len)
{
    char *buffer = 0;

    if(len <= 0)
    {
        errno = ERANGE;
        return -1;
    }
    if(file > _handleCount)
    {
        errno = EBADF;
        return -1;
    }
    buffer = malloc(len + sizeof(uint32_t));
    memcpy((void *)(&buffer[sizeof(uint32_t)]), (void *)ptr, len);
    *((uint32_t *)buffer) = _handles[file]->absoluteHandle;
    sendMessageToVFS(file, (uint32_t)buffer, len + sizeof(uint32_t), MSG_WRITE_STREAM, 1);
    return 0;
}

int gettimeofday(struct timeval *p, void *z)
{
    TimerData *td = getCurrentTime();

	if(p != 0)
	{
		p->tv_sec = td->secondsSinceEpoch;
		p->tv_usec = td->microsecondsSinceEpoch;
	}/*
	if(z != 0)
	{
		z->tz_minuteswest = td->offset;
		//If the current time zone is in DST, tz_dsttime is non-zero
		z->tz_dsttime = td->flags & 1;
	}*/
    return 0;
}

_sig_func_ptr signal(int sig, _sig_func_ptr action)
{
    if(sig >= _signalCount)
    {
        _signalCount = sig;
        _signals = realloc(_signals, _signalCount * sizeof(_sig_func_ptr));
    }
    _signals[sig] = action;
    return action;
}

uint32_t getKernelPID(int linuxPid)
{
    if(linuxPid > _pidCount)
        return 0;
    if(linuxPid < 0)
        return 0;
    return _pids[linuxPid]->nativePid;
}

int getLinuxPID(uint32_t kernelPid)
{
	int i = 0;

    for(i = 0; i < _pidCount; i++)
    {
        if(_pids[i] == 0)
            continue;
        if(_pids[i]->nativePid == kernelPid)
            return i;
    }
    return -1;
}

void installMessageHandler(uint32_t pid, uint8_t nativePid, MessageHandler hnd)
{
    int idx = (int)pid;

    if(nativePid == 1)
        idx = addPOSIXPid(pid);
    //Shouldn't happen, but best to be absolutely certain
    if(idx < 0)
        return;
    //The only way this can happen is if somebody decides to pass a horrid POSIX index. This always means they've made a mistake
    if(idx > _pidCount)
        return;
    _pids[idx]->handler = hnd;
}

MessageHandler retreiveMessageHandler(uint32_t pid, uint8_t nativePid)
{
    int idx = (int)pid;

    if(nativePid == 1)
        idx = getLinuxPID(pid);
    if(idx == -1 || idx >= _pidCount || _pids[idx] == 0)
        return (MessageHandler)0;
    return _pids[idx]->handler;
}

ProcessList *getProcesses()
{
	uint32_t *ebx = 0;
	uint32_t flags = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
	ProcessList *pl = malloc(sizeof(ProcessList));

	asm volatile ("int $0x30" : "=b"(ebx) : "a"(flags), "d"(17));
	pl->Count = ebx[2];
	pl->Processes = (Process *)(ebx[3]);
	return pl;
}

TimerData *getCurrentTime()
{
	uint32_t threadID = 0;
	TimerDataHandler **newList = realloc(_timerDetails, (_timerCount + 1) * sizeof(TimerDataHandler *));
	TimerDataHandler *itm = malloc(sizeof(TimerDataHandler));
	ProcessList *processes = getProcesses();
	uint32_t pid = 0;
	uint32_t i = 0;

	asm volatile ("int $0x30" : "=a"(threadID) : "d"(15));
	itm->wakeThreadId = threadID;
	newList[_timerCount++] = itm;
	itm->waitingForMId = getNextMTID();
	_timerDetails = newList;
	for(i = 0; i < processes->Count; i++)
	{
		Process *p = &processes->Processes[i];

		if(p->Driver != 0 && strcmp(p->Driver->Name, "primaryTimer") == 0)
			pid = p->Pid;
	}
	if(pid == 0)
	{
		free(itm);
		return 0;
	}
	//Install a message handler for the primary timer and go to sleep until it wakes this thread back up
	installMessageHandler(pid, 1, timerHandler);
	asm volatile ("int $0x30" : : : "ebx");
	//We've been woken up. Continue here
	return itm->data;
}
