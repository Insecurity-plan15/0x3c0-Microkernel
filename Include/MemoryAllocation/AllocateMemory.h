#ifndef AllocateMemory_H
#define AllocateMemory_H
#include <LinkedList.h>
#include <Multitasking/Paging.h>
#include <Multitasking/MemoryMap.h>

namespace MemoryManagement
{
	//This is a ported version of liballoc
	namespace HeapDataStructures
	{
		/*
		* A structure found at the top of all system allocated
		* memory blocks. It details the usage of the memory block.
		*/
		struct liballoc_major
		{
			struct liballoc_major *prev;		// Linked list information.
			struct liballoc_major *next;		// Linked list information.
			unsigned int pages;					// The number of pages in the block.
			unsigned int size;					// The number of pages in the block.
			unsigned int usage;					// The number of bytes used in the block.
			struct liballoc_minor *first;		// A pointer to the first allocated memory in the block.
		};

		/* This is a structure found at the beginning of all
		 * sections in a major block which were allocated by a
		 * malloc, calloc, realloc call.
		 */
		struct liballoc_minor
		{
			struct liballoc_minor *prev;		// Linked list information.
			struct liballoc_minor *next;		// Linked list information.
			struct liballoc_major *block;		// The owning block. A pointer to the major structure.
			unsigned int magic;					// A magic number to idenfity correctness.
			unsigned int size; 					// The size of the memory allocated. Could be 1 byte or more.
			unsigned int req_size;				// The size of memory requested.
		};

		struct MemoryUsageDetails
		{
			long long MemoryAllocated;
			long long MemoryInUse;
			long long Warnings;
			long long Errors;
			long long PossibleOverruns;
		};
	}

	/*
	* Every process has a heap. When the process exits, everything not shared is freed
	* A heap is created on demand, because it is always at least 4 KiB long. This would take up a lot of space
	*/
	class Heap
	{
	private:
		typedef unsigned int size_t;
		typedef unsigned int uintptr_t;
		HeapDataStructures::liballoc_major *l_memRoot;
		HeapDataStructures::liballoc_major *l_bestBet;
		int l_pageSize;
		unsigned int l_pageCount;
		long long l_allocated;
		long long l_inuse;
		long long l_warningCount;
		long long l_errorCount;
		long long l_possibleOverruns;

		static Heap *kernelHeap;
		unsigned int mappingFlags;
		unsigned int heapStart;
		unsigned int heapEnd;
		unsigned int heapMax;
		MemoryManagement::x86::PageDirectory pageDirectory;

		void *liballoc_memset(void *s, int c, size_t n);
		void *liballoc_memcpy(void *s1, const void *s2, size_t n);
		int liballoc_lock();
		int liballoc_unlock();
		void *liballoc_alloc(size_t pages);
		int liballoc_free(void *p, size_t pages);

		HeapDataStructures::liballoc_major *allocate_new_page(unsigned int size);
	public:
		Heap(unsigned int start, unsigned int max, bool kernelMode, MemoryManagement::x86::PageDirectory pd);
		~Heap();

		void *Allocate(size_t req_sz);
		void *CleanAllocate(size_t nobj, size_t size);
		void *Reallocate(void *p, size_t size);
		void Free(void *ptr);
		long long GetAllocatedMemory();

		MemoryManagement::x86::PageDirectory GetPageDirectory();

		static Heap *GetKernelHeap();
	};
}

#endif
