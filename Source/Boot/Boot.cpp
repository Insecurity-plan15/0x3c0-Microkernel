#include <Boot/Multiboot.h>
#include <Boot/DescriptorTables.h>
#include <MemoryAllocation/Physical.h>
#include <Multitasking/Paging.h>
#include <Multitasking/Scheduler.h>
#include <Multitasking/SystemCalls.h>
#include <ELF/ELFLoader.h>

extern "C" void Main(unsigned int, MultibootInfo *);

void Main(unsigned int stack, MultibootInfo *multiboot)
{
	Scheduler *sch = 0;
	MemoryManagement::x86::PageDirectory pd;

	asm volatile ("cli");
	//First things first, load the GDT, IDT and TSS
	DescriptorTables::Install();
	//Set up the page allocation bitmap, accounting for the memory map
	MemoryManagement::Physical::PageAllocator::Initialise(multiboot);
	//Set up the infrastructure used for paging
	MemoryManagement::Virtual::Initialise();
//From this point onwards, heap allocations will work
	//Make the kernel's interrupt management system completely operational...
	Interrupts::Install();
	//...and use it to install a basic system call subsytem
	Interrupts::AddInterrupt(new SystemCalls::SystemCallInterrupt());

	sch = Scheduler::GetScheduler();
	//Set up a basic stack for the kernel to use on an external event
	sch->SetupStack();
	for(unsigned int i = 0; i < multiboot->ModuleCount; i++)
	{
		Module *mod = &multiboot->Modules[i];
		//The first 4 MiB is mapped to the last 0xF0000000 bytes of memory
		unsigned int properStart = mod->Start < 0x400000 ? mod->Start + 0xF0000000 : mod->Start;
		Process *elfProcess = 0;
		ELF::ELFLoader *loader = 0;

		//Note that I clone the kernel page directory and switch to it before I do any memory allocations related to the process I'm loading
		pd = MemoryManagement::Virtual::ClonePageDirectory();
		MemoryManagement::Virtual::SwitchPageDirectory(pd);

		//The second parameter is 0 because it can't receive messages yet. A system call is available to allow message reception
		elfProcess = new Process(pd, 0);
		loader = new ELF::ELFLoader((void *)properStart, mod->End - mod->Start);
		loader->SetProcess(elfProcess);
		sch->AddProcess(elfProcess);
		loader->Start((void *)0xABCDEF);
	}
	//A zero as the input state means that kernel execution should change the current stack
	sch->Yield(0);
	//This function will never return. It will not be part of the multitasking arrangement
}
