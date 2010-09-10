#include <Common/Ports.h>
#include <Common/Interrupts.h>
#include <Memory.h>

List<LinkedList<InterruptSink *> *> *Interrupts::interrupts = 0;

//The next two methods will get called automatically, as the assembly stubs drop through to them
extern "C" StackStates::Interrupt *exceptionHandler(StackStates::Interrupt *stack)
{
	//When stack->Interrupt number is passed as a 32-bit integer, it is sign-extended. This can make the value strange
	//It needs to be capped to a reasonable value
	if((stack->InterruptNumber & 0xFF) != 0x30)
		asm volatile ("xchg %%bx, %%bx" : : "a"(stack->InterruptNumber & 0xFF), "c"(stack->RIP), "d"(0xABCFED));
	Interrupts::InvokeInterruptChain(stack->InterruptNumber & 0xFF, stack);
	Ports::WriteByte(0x20, 0x20);
	return stack;
}

extern "C" StackStates::Interrupt *irqHandler(StackStates::Interrupt *stack)
{
	Interrupts::InvokeInterruptChain(stack->InterruptNumber & 0xFF, stack);
	if((stack->InterruptNumber & 0xFF) > 41)
		Ports::WriteByte(0xA0, 0x20);
	Ports::WriteByte(0x20, 0x20);
	return stack;
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

LinkedList<InterruptSink *> *Interrupts::GetInterrupts(unsigned short vector)
{
	return interrupts->GetItem(vector);
}

void Interrupts::InvokeInterruptChain(unsigned short vector, StackStates::Interrupt *stack)
{
	LinkedList<InterruptSink *> *ints = GetInterrupts(vector);

	for(LinkedListNode<InterruptSink *> *nd = ints->First; nd != 0 && nd->Value != 0; nd = nd->Next)
		nd->Value->receive(stack);
}
