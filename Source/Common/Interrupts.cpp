#include <Common/Ports.h>
#include <Common/Interrupts.h>
#include <Memory.h>

List<LinkedList<InterruptSink *> *> *Interrupts::interrupts = 0;

//The next two methods will get called automatically, as the assembly stubs drop through to them
extern "C"
{
	StackStates::Interrupt *interruptHandler(StackStates::Interrupt *stack);

	StackStates::Interrupt *interruptHandler(StackStates::Interrupt *stack)
	{
		if(stack->InterruptNumber != 0x30)
			asm volatile ("xchg %%bx, %%bx" : : "a"(stack->InterruptNumber), "b"(stack->RIP), "c"(0xABCFED));
		//Interrupts::InvokeInterruptChain((uint16)stack->InterruptNumber, stack);
		if(stack->InterruptNumber > 41)
			Ports::WriteByte(0xA0, 0x20);
		Ports::WriteByte(0x20, 0x20);
		return stack;
	}
}

Interrupts::Interrupts()
{
}

Interrupts::~Interrupts()
{
}

void Interrupts::Install()
{
	//There are 48 vectors to start with, but the IOAPIC setup may increase this substantially
	//The below code adds an array of 48 pointers to linked lists of interrupt sinks
	interrupts = new List<LinkedList<InterruptSink *> *>(0);
	for(unsigned int i = 0; i < 48; i++)
		interrupts->Add(new LinkedList<InterruptSink *>(0));
}

void Interrupts::AddInterrupt(InterruptSink *sink)
{
	unsigned int i = (unsigned int)sink->v;

	if(sink->v >= interrupts->GetCount())
		return;
	interrupts->GetItem(i)->Add(sink);
}

void Interrupts::RemoveInterrupt(InterruptSink *sink)
{
	if(sink->v >= interrupts->GetCount())
		return;
	interrupts->GetItem(sink->v)->Remove(sink);
	if(interrupts->GetItem(sink->v)->First == 0)
		interrupts->Remove(sink->v);
}

LinkedList<InterruptSink *> *Interrupts::GetInterrupts(uint16 vector)
{
	return interrupts->GetItem(vector);
}

void Interrupts::InvokeInterruptChain(uint16 vector, StackStates::Interrupt *stack)
{
	LinkedList<InterruptSink *> *ints = GetInterrupts(vector);

	for(LinkedListNode<InterruptSink *> *nd = ints->First; nd != 0 && nd->Value != 0; nd = nd->Next)
		nd->Value->receive(stack);
}
