#include <Message.h>

struct DriverInfoBlock
{
	char Name[32];
	bool SupportsInterrupts;
	unsigned short IRQ;
	unsigned int Type;
};

extern "C"
{
	void Main(int abc);
}

void performCPUID();

#define TimerFrequency	100

void Main(int parameter)
{
	DriverInfoBlock testDriver;
	const char *name = "pitDriver";

	for(unsigned int i = 0; name[i] != 0; i++)
		testDriver.Name[i] = name[i];
	testDriver.SupportsInterrupts = true;
	testDriver.IRQ = 32;
	testDriver.Type = 1;

	performCPUID();

	asm volatile ("int $0x30" : : "a"(&testDriver), "d"(9));

	for(;;) ;
}

void performCPUID()
{
	unsigned int edx = 0;

	asm volatile ("cpuid" : "=d"(edx) : "0"(0x1) : "%eax", "%ebx", "%ecx");
	//If the bit indicating the presence of an FPU is set, initialise it
	if((edx & 0x1) == 0x1)
		asm volatile ("fninit");
}
