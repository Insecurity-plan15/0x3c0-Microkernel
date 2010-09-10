#include <MemoryAllocation/Physical.h>
#include <Memory.h>

using namespace MemoryManagement::Physical;

virtAddress *PageAllocator::bitmap;
//A frame is otherwise known as an integer. frameCount therefore, contains the number of integers in the bitmap
uint64 PageAllocator::frameCount = 0;
//The end of the DMA zone is the number of pages between 0 and 16 MiB
//If there is less than 16 MiB of memory, this is 0
uint32 PageAllocator::dmaZoneEnd = 0;

PageAllocator::PageAllocator()
{
}

PageAllocator::~PageAllocator()
{
}

bool PageAllocator::testBit(uint64 bit)
{
	//This could easily be written using ==, not !=. But that would require another value to be computed, instead of a constant
	//Plus, it's easier to read
	return (bitmap[bit / 32] & (1 << (bit % 32))) != 0;
}

void PageAllocator::setBit(uint64 bit)
{
	bitmap[bit / 32] |= (1 << (bit % 32));
}

void PageAllocator::clearBit(uint64 bit)
{
	bitmap[bit / 32] &= ~(1 << (bit % 32));
}

void PageAllocator::Initialise(MultibootInfo *multiboot)
{
	extern virtAddress end;	//This is just a simple pointer to the end of the kernel. Nothing else uses it
	uint64 memorySize = (multiboot->MemoryHigh * 1024) & PageMask;
	unsigned int pages = memorySize / PageSize;
	unsigned int dmaSegment = DMAMemoryConstraint / PageSize;

	frameCount = pages / 32;	//pages is the number of bits. I convert this to the number of integers
	bitmap = &end;
	Memory::Clear(bitmap, frameCount * 4);	//frameCount is multiplied by 4 because Clear erases n bytes, not n integers
	dmaZoneEnd = dmaSegment / 32;

	//If there is more than 16 MiB of memory, then frameCount is the number of frames I can use
	//Otherwise, just lump everything in together; all allocations are guaranteed to be DMA-safe
	if(frameCount <= dmaZoneEnd)
		dmaZoneEnd = 0;
	//This just uses the E820 memory map to mark some of the physical zones of memory as allocated (if unusable)
	//Trick alert: &ele[1] just increments ele by sizeof(MemoryMapElement). It's just easier for me to write
	for(MemoryMapElement *ele = multiboot->MemoryMap; (virtAddress)ele < (virtAddress)multiboot->MemoryMap + multiboot->MemoryMapLength;
		ele = &ele[1])
	{
		unsigned int startBit = (ele->BaseAddressLow & PageMask) / PageSize;
		unsigned int endBit = ((ele->BaseAddressLow + ele->LengthLow) & PageMask) / PageSize;

		/*
		* I've noticed that a region of memory which appears to be the location of the LAPIC is marked in the memory map.
		* When the ending address is computed, the integer rolls over to 0. I'm hacking around this and doing a sanity check
		* by making certain that the physical addresses in the memory map aren't beyond the end of physical memory.
		*/
		if(ele->BaseAddressLow >= memorySize)
			continue;
		if(ele->BaseAddressLow + ele->LengthLow >= memorySize)
			continue;
		//If the memory range is unusable or the BIOS knows it's bad, then don't allow the OS to use it
		if(ele->Type == 2 || ele->Type == 5)
			for(unsigned int i = startBit; i < endBit; i++)
				setBit(i);
	}
	//Finally, I want to preserve zero as a valid return address if there's no more memory
	setBit(0);
}

uint64 PageAllocator::firstFreeBit(bool dma)
{
	//Iterate every integer in the array
	for(unsigned int i = (dma ? 0 : dmaZoneEnd); i < (dma ? dmaZoneEnd : frameCount); i++)
	{
		//Quick optimisation to save time. Unfortunately, the rest of the method is a timesink
		if(bitmap[i] == 0xFFFFFFFF)
			continue;
		for(unsigned int bit = 0; bit < 32; bit++)
			if(! testBit((i * 32) + bit))
				return (i * 32) + bit;
	}
	return 0;
}

uint64 PageAllocator::firstFreeBits(unsigned int n, bool dma)
{
	for(unsigned int i = (dma ? 0 : dmaZoneEnd); i < (dma ? dmaZoneEnd : frameCount); i++)
	{
		if(bitmap[i] == 0xFFFFFFFF)
			continue;
		for(unsigned int bit = 0; bit < 32; bit++)
		{
			if(! testBit((i * 32) + bit))
			{
				unsigned int startingBit = (i * 32) + bit;
				unsigned int endingBit = startingBit + n;
				unsigned int count = 0;

				for(unsigned int j = startingBit; j < endingBit; j++)
					if(! testBit(j))
						count++;
				if(count == n)
					return (i * 32) + bit;
			}
		}
	}
	return 0;
}

physAddress PageAllocator::AllocatePage(bool dma)
{
	uint64 bit = firstFreeBit(dma);

	//Mark the memory as allocated
	setBit(bit);
	//Fairly simple. firstFreeBit can return 0, but 0*PageSize = 0, signalling no more memory
	return (physAddress)(bit * PageSize);
}

physAddress PageAllocator::AllocatePages(unsigned int n, bool dma)
{
	uint64 bit = firstFreeBits(dma);

	//Mark the memory as allocated
	for(uint64 i = bit; i < bit + n; i++)
		setBit(i);
	return (physAddress)(bit * PageSize);
}

bool PageAllocator::FreePage(physAddress pg)
{
	uint64 bitIndex = (uint64)((physAddress)pg / PageSize);

	//If the pointer isn't page-aligned, die quickly
	if(((physAddress)pg % PageSize) != 0)
		return false;
	clearBit(bitIndex);
	return true;
}
