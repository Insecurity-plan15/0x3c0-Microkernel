#ifndef SystemCalls_H
#define SystemCalls_H

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

#include "DataStructures\Microkernel.h"
#include "DataStructures\Processes.h"

#define PageSize    0x1000

uint32_t getVFS();

void _exit();
int close(int file);
char **environ;
//int execve(char *name, char **argv, char **env);
int fork();
int fstat(int file, struct stat *st);
int getpid();
int isatty(int file);
int kill(int pid, int sig);
//int link(char *old, char *new);
//int lseek(int file, int ptr, int dir);
int open(const char *name, int flags, ...);
//int read(int file, char *ptr, int len);
//caddr_t sbrk(int incr);
int stat(const char *file, struct stat *st);
//clock_t times(struct tms *buf);
//int unlink(char *name);
int wait(int *status);
//int write(int file, char *ptr, int len);
//int gettimeofday(struct timeval *p, struct timezone *z);
//void signal(int sig, _sig_func_ptr action);

uint32_t getKernelPID(int linuxPid);
int getLinuxPID(uint32_t kernelPid);

//Install a message handler for messages from the specified process ID (if a native ID, [nativePid] will be one)
//To remove a handler, [hnd] must be zero
void installMessageHandler(uint32_t pid, uint8_t nativePid, MessageHandler hnd);
//Same as above, but to retrieve a message handler
MessageHandler retreiveMessageHandler(uint32_t pid, uint8_t nativePid);

ProcessList *getProcesses();
TimerData *getCurrentTime();
#endif
