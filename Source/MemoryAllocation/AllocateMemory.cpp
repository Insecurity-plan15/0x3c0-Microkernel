/* This memory allocation came from LibAlloc. The copyright notice is below:
Copyright (c) 2009 Durand John Miller
All rights reserved.


Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:

   1. Redistributions of source code must retain the
      above copyright notice, this list of conditions and
      the following disclaimer.
   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
   3. The name of the author may not be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Multitasking/Process.h>
#include <MemoryAllocation/AllocateMemory.h>
#include <MemoryAllocation/Physical.h>
#include <Multitasking/MemoryMap.h>
#include <Common/CPlusPlus.h>

using namespace MemoryManagement;
using namespace HeapDataStructures;

#define ALIGNMENT	4ul				// This is the byte alignment that memory must be allocated on. IMPORTANT for GTK and other stuff.

#define ALIGN_TYPE		unsigned short
#define ALIGN_INFO		sizeof(ALIGN_TYPE)	// Alignment information is stored right before the pointer. This is the number of bytes of information stored there.


#define USE_CASE1
#define USE_CASE2
#define USE_CASE3
#define USE_CASE4
#define USE_CASE5


/* This macro will conveniently align our pointer upwards */
#define ALIGN( ptr )													\
		if ( ALIGNMENT > 1 )											\
		{																\
			uintptr_t delta;												\
			ptr = (void*)((uintptr_t)ptr + ALIGN_INFO);					\
			delta = (uintptr_t)ptr & (ALIGNMENT-1);						\
			if ( delta != 0 )											\
			{															\
				delta = ALIGNMENT - delta;								\
				ptr = (void*)((uintptr_t)ptr + delta);					\
			}															\
			*((ALIGN_TYPE*)((uintptr_t)ptr - ALIGN_INFO)) = 			\
				delta + ALIGN_INFO;										\
		}


#define UNALIGN( ptr )													\
		if ( ALIGNMENT > 1 )											\
		{																\
			uintptr_t delta = *((ALIGN_TYPE*)((uintptr_t)ptr - ALIGN_INFO));	\
			if ( delta < (ALIGNMENT + ALIGN_INFO) )						\
			{															\
				ptr = (void*)((uintptr_t)ptr - delta);					\
			}															\
		}



#define LIBALLOC_MAGIC	0xc001c0de
#define LIBALLOC_DEAD	0xdeaddead
#define NULL			0

Heap *Heap::kernelHeap = 0;

void* Heap::liballoc_memset(void* s, int c, size_t n)
{
	size_t i;

	for ( i = 0; i < n ; i++)
		((char*)s)[i] = c;

	return s;
}

void* Heap::liballoc_memcpy(void* s1, const void* s2, size_t n)
{
  char *cdest;
  char *csrc;
  unsigned int *ldest = (unsigned int*)s1;
  unsigned int *lsrc  = (unsigned int*)s2;

  while ( n >= sizeof(unsigned int) )
  {
      *ldest++ = *lsrc++;
	  n -= sizeof(unsigned int);
  }

  cdest = (char*)ldest;
  csrc  = (char*)lsrc;

  while ( n > 0 )
  {
      *cdest++ = *csrc++;
	  n -= 1;
  }

  return s1;
}

int Heap::liballoc_lock()
{
	asm volatile ("cli");
	return 0;
}

int Heap::liballoc_unlock()
{
	return 0;
}

void *Heap::liballoc_alloc(size_t pages)
{
	physAddress p = 0;
	virtAddress formerHeapEnd = heapEnd;

	//If the total pages would make the heap too large, then they shouldn't be allocated
	if(heapEnd + (pages * PageSize) > heapMax)
		return 0;
	for(size_t i = 0; i < pages; i++, heapEnd += PageSize)
	{
		//Otherwise, allocate the physical pages...
		p = Physical::PageAllocator::AllocatePage(false);
		if(p == 0)
			return 0;
		//...and map them in
		Virtual::MapMemory((unsigned int)p, heapEnd, mappingFlags);
	}
	//Return a pointer to the start of the allocated block
	return (void *)formerHeapEnd;
}

int Heap::liballoc_free(void *p, size_t pages)
{
	//For consistency, I'll start at the end of the range and sequentially unmap the pages
	virtAddress pageEnd = (virtAddress)p + pages * PageSize;

	for(virtAddress i = pageEnd; i > (virtAddress)p; i -= PageSize)
	{
		physAddress phys = Virtual::RetrieveMapping(i, 0);

		Physical::PageAllocator::FreePage(phys);
		Virtual::EraseMapping(i);
	}
	if((virtAddress)p + pages * PageSize == heapEnd)
		heapEnd -= (pages * PageSize);
	return 0;
}

liballoc_major *Heap::allocate_new_page(unsigned int size)
{
	unsigned int st;
	struct liballoc_major *maj;

		// This is how much space is required.
		st  = size + sizeof(struct liballoc_major);
		st += sizeof(struct liballoc_minor);

				// Perfect amount of space?
		if ( (st % l_pageSize) == 0 )
			st  = st / (l_pageSize);
		else
			st  = st / (l_pageSize) + 1;
							// No, add the buffer.


		// Make sure it's >= the minimum size.
		if ( st < l_pageCount ) st = l_pageCount;

		maj = (struct liballoc_major*)liballoc_alloc( st );
		if ( maj == NULL )
		{
			l_warningCount += 1;
			return NULL;	// uh oh, we ran out of memory.
		}

		maj->prev 	= NULL;
		maj->next 	= NULL;
		maj->pages 	= st;
		maj->size 	= st * l_pageSize;
		maj->usage 	= sizeof(struct liballoc_major);
		maj->first 	= NULL;
		l_allocated += maj->size;
      return maj;
}

Heap::Heap(unsigned int start, unsigned int max, bool kernelMode, MemoryManagement::x86::PageDirectory pd)
{
	heapStart = heapEnd = start;
	heapMax = max;
	mappingFlags = x86::PageDirectoryFlags::Present | x86::PageDirectoryFlags::ReadWrite |
		(kernelMode ? 0 : x86::PageDirectoryFlags::UserAccessible);
	pageDirectory = pd;

	l_memRoot = l_bestBet = 0;
	l_pageSize = PageSize;
	l_pageCount = 16;
	l_allocated = l_inuse = 0;
	l_warningCount = l_errorCount = l_possibleOverruns = 0;
}

Heap::~Heap()
{
	MemoryManagement::x86::PageDirectory pd = MemoryManagement::Virtual::GetPageDirectory();

	MemoryManagement::Virtual::SwitchPageDirectory(pageDirectory);
	//When the heap is destroyed, all the allocations go with it
	for(virtAddress i = heapStart; i < heapEnd; i += PageSize)
	{
		physAddress physicalAddress = Virtual::RetrieveMapping(i, 0);

		//Free the physical addresses to avoid memory leaks, and the virtual mappings to remain consistent
		Physical::PageAllocator::FreePage(physicalAddress);
		Virtual::EraseMapping(i);
	}
	MemoryManagement::Virtual::SwitchPageDirectory(pd);
}

void *Heap::Allocate(size_t req_size)
{
	MemoryManagement::x86::PageDirectory pd = MemoryManagement::Virtual::GetPageDirectory();
	int startedBet = 0;
	uint64 bestSize = 0;
	void *p = NULL;
	uintptr_t diff;
	struct liballoc_major *maj;
	struct liballoc_minor *min;
	struct liballoc_minor *new_min;
	size_t size = req_size;

	//Switch page directories to make sure that everything stays consistent
	//Checking that I don't switch to an already-present page directory saves TLB flushes
	if(pd != pageDirectory)
		MemoryManagement::Virtual::SwitchPageDirectory(pageDirectory);

	// For alignment, we adjust size so there's enough space to align.
	if ( ALIGNMENT > 1 )
	{
		size += ALIGNMENT + ALIGN_INFO;
	}
				// So, ideally, we really want an alignment of 0 or 1 in order
				// to save space.

	liballoc_lock();

	if ( size == 0 )
	{
		l_warningCount += 1;
		liballoc_unlock();
		return Allocate(1);
	}


	if ( l_memRoot == NULL )
	{
		// This is the first time we are being used.
		l_memRoot = allocate_new_page( size );
		if ( l_memRoot == NULL )
		{
		  liballoc_unlock();
		  return NULL;
		}
	}

	// Now we need to bounce through every major and find enough space....

	maj = l_memRoot;
	startedBet = 0;

	// Start at the best bet....
	if ( l_bestBet != NULL )
	{
		bestSize = l_bestBet->size - l_bestBet->usage;

		if ( bestSize > (size + sizeof(struct liballoc_minor)))
		{
			maj = l_bestBet;
			startedBet = 1;
		}
	}
	while ( maj != NULL )
	{
		diff  = maj->size - maj->usage;
										// free memory in the block

		if ( bestSize < diff )
		{
			// Hmm.. this one has more memory then our bestBet. Remember!
			l_bestBet = maj;
			bestSize = diff;
		}


#ifdef USE_CASE1

		// CASE 1:  There is not enough space in this major block.
		if ( diff < (size + sizeof( struct liballoc_minor )) )
		{
			// Another major block next to this one?
			if ( maj->next != NULL )
			{
				maj = maj->next;		// Hop to that one.
				continue;
			}

			if ( startedBet == 1 )		// If we started at the best bet,
			{							// let's start all over again.
				maj = l_memRoot;
				startedBet = 0;
				continue;
			}

			// Create a new major block next to this one and...
			maj->next = allocate_new_page( size );	// next one will be okay.
			if ( maj->next == NULL ) break;			// no more memory.
			maj->next->prev = maj;
			maj = maj->next;
			// .. fall through to CASE 2 ..
		}

#endif
#ifdef USE_CASE2

		// CASE 2: It's a brand new block.
		if ( maj->first == NULL )
		{
			maj->first = (struct liballoc_minor*)((uintptr_t)maj + sizeof(struct liballoc_major) );

			maj->first->magic 		= LIBALLOC_MAGIC;
			maj->first->prev 		= NULL;
			maj->first->next 		= NULL;
			maj->first->block 		= maj;
			maj->first->size 		= size;
			maj->first->req_size 	= req_size;
			maj->usage 	+= size + sizeof( struct liballoc_minor );


			l_inuse += size;


			p = (void*)((uintptr_t)(maj->first) + sizeof( struct liballoc_minor ));
			ALIGN( p );

			if(pd != pageDirectory)
				MemoryManagement::Virtual::SwitchPageDirectory(pd);
			liballoc_unlock();		// release the lock
			return p;
		}

#endif
#ifdef USE_CASE3

		// CASE 3: Block in use and enough space at the start of the block.
		diff =  (uintptr_t)(maj->first);
		diff -= (uintptr_t)maj;
		diff -= sizeof(struct liballoc_major);

		if ( diff >= (size + sizeof(struct liballoc_minor)) )
		{
			// Yes, space in front. Squeeze in.
			maj->first->prev = (struct liballoc_minor*)((uintptr_t)maj + sizeof(struct liballoc_major) );
			maj->first->prev->next = maj->first;
			maj->first = maj->first->prev;
			maj->first->magic 	= LIBALLOC_MAGIC;
			maj->first->prev 	= NULL;
			maj->first->block 	= maj;
			maj->first->size 	= size;
			maj->first->req_size 	= req_size;
			maj->usage 			+= size + sizeof( struct liballoc_minor );

			l_inuse += size;

			p = (void*)((uintptr_t)(maj->first) + sizeof( struct liballoc_minor ));
			ALIGN( p );

			if(pd != pageDirectory)
				MemoryManagement::Virtual::SwitchPageDirectory(pd);
			liballoc_unlock();		// release the lock
			return p;
		}

#endif


#ifdef USE_CASE4

		// CASE 4: There is enough space in this block. But is it contiguous?
		min = maj->first;

			// Looping within the block now...
		while ( min != NULL )
		{
				// CASE 4.1: End of minors in a block. Space from last and end?
				if ( min->next == NULL )
				{
					// the rest of this block is free...  is it big enough?
					diff = (uintptr_t)(maj) + maj->size;
					diff -= (uintptr_t)min;
					diff -= sizeof( struct liballoc_minor );
					diff -= min->size;
						// minus already existing usage..

					if ( diff >= (size + sizeof( struct liballoc_minor )) )
					{
						// yay....
						min->next = (struct liballoc_minor*)((uintptr_t)min + sizeof( struct liballoc_minor ) + min->size);
						min->next->prev = min;
						min = min->next;
						min->next = NULL;
						min->magic = LIBALLOC_MAGIC;
						min->block = maj;
						min->size = size;
						min->req_size = req_size;
						maj->usage += size + sizeof( struct liballoc_minor );

						l_inuse += size;

						p = (void*)((uintptr_t)min + sizeof( struct liballoc_minor ));
						ALIGN( p );

						if(pd != pageDirectory)
							MemoryManagement::Virtual::SwitchPageDirectory(pd);
						liballoc_unlock();		// release the lock
						return p;
					}
				}


				// CASE 4.2: Is there space between two minors?
				if ( min->next != NULL )
				{
					// is the difference between here and next big enough?
					diff  = (uintptr_t)(min->next);
					diff -= (uintptr_t)min;
					diff -= sizeof( struct liballoc_minor );
					diff -= min->size;
										// minus our existing usage.

					if ( diff >= (size + sizeof( struct liballoc_minor )) )
					{
						// yay......
						new_min = (struct liballoc_minor*)((uintptr_t)min + sizeof( struct liballoc_minor ) + min->size);

						new_min->magic = LIBALLOC_MAGIC;
						new_min->next = min->next;
						new_min->prev = min;
						new_min->size = size;
						new_min->req_size = req_size;
						new_min->block = maj;
						min->next->prev = new_min;
						min->next = new_min;
						maj->usage += size + sizeof( struct liballoc_minor );

						l_inuse += size;

						p = (void*)((uintptr_t)new_min + sizeof( struct liballoc_minor ));
						ALIGN( p );

						if(pd != pageDirectory)
							MemoryManagement::Virtual::SwitchPageDirectory(pd);
						liballoc_unlock();		// release the lock
						return p;
					}
				}	// min->next != NULL

				min = min->next;
		} // while min != NULL ...


#endif

#ifdef USE_CASE5

		// CASE 5: Block full! Ensure next block and loop.
		if ( maj->next == NULL )
		{
			if ( startedBet == 1 )
			{
				maj = l_memRoot;
				startedBet = 0;
				continue;
			}

			// we've run out. we need more...
			maj->next = allocate_new_page( size );		// next one guaranteed to be okay
			if ( maj->next == NULL ) break;			//  uh oh,  no more memory.....
			maj->next->prev = maj;

		}

#endif

		maj = maj->next;
	} // while (maj != NULL)


	if(pd != pageDirectory)
		MemoryManagement::Virtual::SwitchPageDirectory(pd);
	liballoc_unlock();		// release the lock
	return NULL;
}

void *Heap::CleanAllocate(size_t nobj, size_t size)
{
	int real_size;
    void *p;

    real_size = nobj * size;

	p = Allocate( real_size );

    liballoc_memset( p, 0, real_size );

	return p;
}

void *Heap::Reallocate(void *p, size_t size)
{
	void *ptr;
	struct liballoc_minor *min;
	int real_size;

	// Honour the case of size == 0 => free old and return NULL
	if ( size == 0 )
	{
		Free( p );
		return NULL;
	}

	// In the case of a NULL pointer, return a simple malloc.
	if ( p == NULL ) return Allocate( size );

	// Unalign the pointer if required.
	ptr = p;
	UNALIGN(ptr);

	liballoc_lock();		// lockit

		min = (struct liballoc_minor*)((uintptr_t)ptr - sizeof( struct liballoc_minor ));

		// Ensure it is a valid structure.
		if ( min->magic != LIBALLOC_MAGIC )
		{
			l_errorCount += 1;

			// Check for overrun errors. For all bytes of LIBALLOC_MAGIC
			if (
				((min->magic & 0xFFFFFF) == (LIBALLOC_MAGIC & 0xFFFFFF)) ||
				((min->magic & 0xFFFF) == (LIBALLOC_MAGIC & 0xFFFF)) ||
				((min->magic & 0xFF) == (LIBALLOC_MAGIC & 0xFF))
			   )
			{
				l_possibleOverruns += 1;
			}

			// being lied to...
			liballoc_unlock();		// release the lock
			return NULL;
		}

		// Definitely a memory block.

		real_size = min->req_size;

		if ( (unsigned int)real_size >= size )
		{
			min->req_size = size;
			liballoc_unlock();
			return p;
		}

	liballoc_unlock();

	// If we got here then we're reallocating to a block bigger than us.
	ptr = Allocate( size );					// We need to allocate new memory
	liballoc_memcpy( ptr, p, real_size );
	Free( p );

	return ptr;
}

void Heap::Free(void *ptr)
{
	MemoryManagement::x86::PageDirectory pd = MemoryManagement::Virtual::GetPageDirectory();
	struct liballoc_minor *min;
	struct liballoc_major *maj;

	if ( ptr == NULL )
	{
		l_warningCount += 1;
		return;
	}

	if(pd != pageDirectory)
		MemoryManagement::Virtual::SwitchPageDirectory(pageDirectory);
	UNALIGN( ptr );

	liballoc_lock();		// lockit


	min = (struct liballoc_minor*)((uintptr_t)ptr - sizeof( struct liballoc_minor ));


	if ( min->magic != LIBALLOC_MAGIC )
	{
		l_errorCount += 1;

		// Check for overrun errors. For all bytes of LIBALLOC_MAGIC
		if (
			((min->magic & 0xFFFFFF) == (LIBALLOC_MAGIC & 0xFFFFFF)) ||
			((min->magic & 0xFFFF) == (LIBALLOC_MAGIC & 0xFFFF)) ||
			((min->magic & 0xFF) == (LIBALLOC_MAGIC & 0xFF))
		   )
		{
			l_possibleOverruns += 1;
		}

		// being lied to...
		liballoc_unlock();		// release the lock
		return;
	}

		maj = min->block;

		l_inuse -= min->size;

		maj->usage -= (min->size + sizeof( struct liballoc_minor ));
		min->magic  = LIBALLOC_DEAD;		// No mojo.

		if ( min->next != NULL ) min->next->prev = min->prev;
		if ( min->prev != NULL ) min->prev->next = min->next;

		if ( min->prev == NULL ) maj->first = min->next;
							// Might empty the block. This was the first
							// minor.


	// We need to clean up after the majors now....

	if ( maj->first == NULL )	// Block completely unused.
	{
		if ( l_memRoot == maj ) l_memRoot = maj->next;
		if ( l_bestBet == maj ) l_bestBet = NULL;
		if ( maj->prev != NULL ) maj->prev->next = maj->next;
		if ( maj->next != NULL ) maj->next->prev = maj->prev;
		l_allocated -= maj->size;

		liballoc_free( maj, maj->pages );
	}
	else
	{
		if ( l_bestBet != NULL )
		{
			int bestSize = l_bestBet->size  - l_bestBet->usage;
			int majSize = maj->size - maj->usage;

			if ( majSize > bestSize ) l_bestBet = maj;
		}

	}

	if(pd != pageDirectory)
		MemoryManagement::Virtual::SwitchPageDirectory(pd);
	liballoc_unlock();		// release the lock
}

uint64 Heap::GetAllocatedMemory()
{
	return l_inuse;
}

MemoryManagement::x86::PageDirectory Heap::GetPageDirectory()
{
	return pageDirectory;
}

Heap *Heap::GetKernelHeap()
{
	extern unsigned int kernelHeapStart;

	if(kernelHeap == 0)
		kernelHeap = new((virtAddress)&kernelHeapStart) Heap(KernelHeapStart, KernelHeapEnd, true,
            MemoryManagement::Virtual::GetPageDirectory());
	return kernelHeap;
}
