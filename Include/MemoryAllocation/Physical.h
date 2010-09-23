#ifndef Physical_H
#define Physical_H
#include <Multitasking/Paging.h>
#include <Boot/Multiboot.h>

namespace MemoryManagement
{
	namespace Physical
	{
		//All DMA transactions must be in the lower 16 MiB of memory
		static const unsigned int DMAMemoryConstraint = 0x1000000;

		class BootstrapStack
		{
		private:
			physAddress physStack[4];
			uint32 count;
			uint32 marker;
		public:
			BootstrapStack();
			~BootstrapStack();
			physAddress Pop();
		};

		class PageAllocator
		{
		private:
			PageAllocator();
			~PageAllocator();
			static BootstrapStack initAllocator;
			static virtAddress *bitmap;
			static uint64 frameCount;
			static uint32 dmaZoneEnd;

			static bool testBit(uint64 bit);
			static void setBit(uint64 bit);
			static void clearBit(uint64 bit);
			static uint64 firstFreeBit(bool dma = false);
			static uint64 firstFreeBits(unsigned int n, bool dma = false);

			//hmm. this returns a physical address - not mapped?
			static physAddress optimumMemoryLocation(uint64 byteCount, MultibootInfo *info);
		public:
			static void Initialise();
			static void CompleteInitialisation(MultibootInfo *multiboot);
			static physAddress AllocatePage(bool dma = false);
			static physAddress AllocatePages(unsigned int n, bool dma = false);
			static bool FreePage(physAddress pg);
		};
	}
}
#endif
