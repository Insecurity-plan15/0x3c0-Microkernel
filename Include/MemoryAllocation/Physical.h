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

		class PageAllocator
		{
		private:
			PageAllocator();
			~PageAllocator();
			static virtAddress *bitmap;
			static uint64 frameCount;
			static uint32 dmaZoneEnd;

			static bool testBit(uint64 bit);
			static void setBit(uint64 bit);
			static void clearBit(uint64 bit);
			static uint64 firstFreeBit(bool dma = false);
			static uint64 firstFreeBits(unsigned int n, bool dma = false);
		public:
			static void Initialise(MultibootInfo *multiboot);
			static physAddress AllocatePage(bool dma = false);
			static physAddress AllocatePages(unsigned int n, bool dma = false);
			static bool FreePage(physAddress pg);
		};
	}
}
#endif
