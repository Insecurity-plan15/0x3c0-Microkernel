#include "onReceipt.h"
#include "MessageHandlers.h"
#include "SystemCalls.h"

#include <malloc.h>

void preMain();
void vfsHandler(Message *m);

void preMain()
{
	//NOTE. MALLOC(0) MAY NOT PROVIDE CORRECT DATA. RESOLVE BY ALLOCATING A SINGLE ELEMENT
    uint32_t vfs = getVFS();

    //Do some basic setup, to prevent the use of uninitialised data
    _handleCount = _pidCount = _signalCount = _messageHandlerCount = _freeMThreadCount = _maxMThread = _timerCount = _waitCount = 1;
    _handles = malloc(sizeof(FileHandle *));
    _pids = malloc(sizeof(ProcessData *));
    _signals = malloc(sizeof(_sig_func_ptr));
    _messageHandlers = malloc(sizeof(MessageHandler));
    _freeMThreadIDs = malloc(sizeof(uint32_t));
    _timerDetails = malloc(sizeof(TimerDataHandler *));
    _waitDetails = malloc(sizeof(WaitData *));
    environ = malloc(sizeof(char *));
    //Provide a method which gets invoked upon receipt of a message
    asm volatile ("int $0x30" : : "a"(&onReceipt), "d"(12));

	//First, we want a kernel handler
	installMessageHandler(0, 1, kernelHandler);
    //Install a simple handler for the VFS to work with
    installMessageHandler(vfs, 1, vfsHandler);
}
