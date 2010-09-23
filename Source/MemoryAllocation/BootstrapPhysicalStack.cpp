#include <MemoryAllocation/Physical.h>

using namespace MemoryManagement::Physical;

BootstrapStack::BootstrapStack()
{
	extern virtAddress EscrowPage1;
	extern virtAddress EscrowPage2;
	extern virtAddress EscrowPage3;
	extern virtAddress EscrowPage4;

	physStack[0] = (physAddress)(&EscrowPage1) - KernelVirtualAddress;
	physStack[1] = (physAddress)(&EscrowPage2) - KernelVirtualAddress;
	physStack[2] = (physAddress)(&EscrowPage3) - KernelVirtualAddress;
	physStack[3] = (physAddress)(&EscrowPage4) - KernelVirtualAddress;
	count = 4;
	marker = 0;
}

BootstrapStack::~BootstrapStack()
{
}

physAddress BootstrapStack::Pop()
{
	if(marker == count)
		return 0;
	return physStack[marker++];
}

