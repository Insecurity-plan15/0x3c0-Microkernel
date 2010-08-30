#ifndef DataStructures_VFS_H
#define DataStructures_VFS_H

typedef struct
{
    //This is set to 0 if the file has not sent a message requiring a response from the VFS. Otherwise, it contains the message
    //thread ID being waited for
    uint8_t waitingForID;

    //When the response is complete, this thread gets awoken by the message handler
    //Make sure GCC knows that this can be altered at strange times
    volatile uint32_t wakeThread;

    //When the response is complete, this data gets passed to the thread being awoken
    uintptr_t operationWakeData;
} FileOperation;

typedef struct
{
    char *name;
    //This is what the VFS will identify it as
    uint32_t absoluteHandle;
    uint32_t filePosition;
    uint32_t flags;

    FileOperation **pendingOperations;
    uint32_t operationCount;
} FileHandle;

typedef struct
{
    uint32_t deviceId;
    uint32_t fileIdentifier;
    //This appears to use a bit mask. Masks are set in <stat.h>
    uint32_t fileMode;

    //I'm not going to say how many hard link (shortcuts) are to the file. It's pointless

    uint32_t userID;
    uint32_t groupID;
    uint32_t fileSize;
} __attribute__((packed)) FileData;

#endif
