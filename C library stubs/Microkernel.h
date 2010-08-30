#ifndef Microkernel_H
#define Microkernel_H

#include "DataStructures/Microkernel.h"
#include "DataStructures/VFS.h"

void sendMessage(uint32_t destination, uint32_t data, uint32_t length, uint32_t code, uint32_t mID, uint8_t flags);

FileHandle **_handles;
int _handleCount;

ProcessData **_pids;
int _pidCount;

TimerDataHandler **_timerDetails;
uint32_t _timerCount;

WaitData **_waitDetails;
uint32_t _waitCount;

//Only one of these. It gets wiped when a new process is made
LoadingDataBlock *load;

_sig_func_ptr *_signals;
int _signalCount;

MessageHandler *_messageHandlers;
int _messageHandlerCount;

//A message thread is allocated when a message chain needs to be created, and freed where necessary
//To optimise for speed and memory usage, there is a maximum message thread ID (which is usually used) and a list of free
//message thread IDs (which is used to make sure that the full range is used, with no gaps)
uint32_t *_freeMThreadIDs;
uint32_t _freeMThreadCount;
uint32_t _maxMThread;

#define MSG_IRQ                 1
#define MSG_SHARED_PAGE         2
#define MSG_REQUEST_DRIVERS     3
#define MSG_REQUEST_DEVICES     4
#define MSG_OPTIMISATIONS       5
#define MSG_CHANGE_VIDEO_MODE   6
#define MSG_GET_VIDEO_MODE      7
#define MSG_PLOT_PIXELS         8
#define MSG_GET_FRAMEBUFFER     9
#define MSG_WRITE_TO_DEVICE     10
#define MSG_SEEK                11
#define MSG_WRITE_TO_STORAGE    12
#define MSG_READ_FROM_STORAGE   13
#define MSG_GET_TICK_COUNT      14
#define MSG_GET_FREQUENCY       15
#define MSG_SLEEP               16

#define MSG_RETURN_DRIVERS      17
#define MSG_RETURN_DEVICES      18
#define MSG_RETURN_VIDEO_MODES  19
#define MSG_RETURN_FRAMEBUFFER  20
#define MSG_RETURN_BYTE_BUFFER  21
#define MSG_AWAKEN              22
#define MSG_RETURN_TICK_COUNT   23
#define MSG_RETURN_FREQUENCY    24

#define MSG_OPEN_STREAM         25
#define MSG_CLOSE_STREAM        26
#define MSG_READ_STREAM         27
#define MSG_WRITE_STREAM        28
#define MSG_GET_FILE_STATUS     29

#define MSG_RETURN_FILE_HANDLE  30
//There's no entry 31. I made a duplicate entry when typing the messages into the database *facepalm*
#define MSG_RETURN_FILE_STATUS  32

#define MSG_DELETE_FS_ENTRY     33
#define MSG_RENAME_FS_ENTRY     34

#define MSG_FILE_READ_COMPLETE	36
#define MSG_FILE_WRITE_COMPLETE	38

#define MSG_LOAD_FILE			39
#define MSG_RETURN_MEMORY_DESCRIPTORS	40

#define MSG_CHILD_STATE_CHANGE	41

#endif
