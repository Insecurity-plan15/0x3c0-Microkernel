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
