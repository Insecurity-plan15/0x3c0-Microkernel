#include <TimerDriver.h>

#define OutportByte(port, value)	asm volatile ("outb %%al, %0" : : "dN"(port), "a"(value))
#define TimerHertz		1193180

void Timer::onIRQ()
{
	ticks++;
}

Timer::Timer(unsigned int f)
{
	unsigned int divisor = TimerHertz / f;

	frequency = f;
	ticks = 0;
	OutportByte(0x43, 0x65);
	OutportByte(0x40, (unsigned char)(divisor & 0xFF));
	OutportByte(0x40, (unsigned char)((divisor >> 8) & 0xFF));
	//wakeProcesses = new List<ProcessSleepData *>();
}

Timer::~Timer()
{
}

void Timer::ReceiveMessage(Message *m)
{
	switch(m->Code)
	{
	//IRQ received
	case 1:
		{
			onIRQ();
			break;
		}
	//Sender wants to know the total tick count
	case 14:
		{
			//
			break;
		}
	//Sender wants to know the frequency
	case 15:
		{
			//
			break;
		}
	//Sender wants to be woken up after a given number of ticks
	case 16:
		{
			//
			break;
		}
	}
}
