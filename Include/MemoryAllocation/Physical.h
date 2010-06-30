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
			static unsigned int *bitmap;
			static unsigned int frameCount;
			static unsigned int dmaZoneEnd;

			static bool testBit(unsigned int bit);
			static void setBit(unsigned int bit);
			static void clearBit(unsigned int bit);
			static unsigned int firstFreeBit(bool dma = false);
			static unsigned int firstFreeBits(unsigned int n, bool dma = false);
		public:
			static void Initialise(MultibootInfo *multiboot);
			static void *AllocatePage(bool dma = false);
			static void *AllocatePages(unsigned int n, bool dma = false);
			static bool FreePage(void *ptr);
		};
	}
}
#endif
