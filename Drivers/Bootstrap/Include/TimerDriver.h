#ifndef TimerDriver_H
#define TimerDriver_H

#include <List.h>
#include <Message.h>

struct ProcessSleepData
{
	unsigned int PID;
	unsigned int TicksRemaining;
};

class Timer
{
private:
	//How am I supposed to allocate space for this without a new implementation
	List<ProcessSleepData *> *wakeProcesses;
	unsigned int frequency;
	unsigned long long ticks;
	void onIRQ();
public:
	Timer(unsigned int f);
	~Timer();
	void ReceiveMessage(Message *m);
};

#endif
