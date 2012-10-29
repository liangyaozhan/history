
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *         Simon Goldschmidt
 *
 */

#include "rtk.h"


/* #define MEM_STATS */

/* #define ALIGN_SIZE   8 */


#define ALIGN(a, s)     ROUND_UP(a,s)

/* #define OBJECT_HOOK_CALL(func, params)  func params */
#define OBJECT_HOOK_CALL(func, params)
/* #define OBJECT_HOOK_CALL(func, params) do { if (func) func params }while (0) */

/* #define DEBUG_LOG(flag, params)     kprintf params */
#define DEBUG_LOG(flag, params)
#define DEBUG_MEM

static void (*malloc_hook)(void *ptr, size_t size);
static void (*free_hook)(void *ptr);

/**
 * @addtogroup grp_hook   memory management hook
 * @{
 */

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
void malloc_sethook(void (*hook)(void *ptr, size_t size))
{
	malloc_hook = hook;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is released to heap memory.
 *
 * @param hook the hook function
 */
void free_sethook(void (*hook)(void *ptr))
{
	free_hook = hook;
}

/** @} */

#define HEAP_MAGIC 0x1ea0
struct heap_mem
{
	/* magic and used flag */
	uint16_t magic;
	uint16_t used;

	size_t next, prev;
};

/** pointer to the heap: for alignment, heap_ptr is now a pointer instead of an array */
static uint8_t *heap_ptr;

/** the last entry, always unused! */
static struct heap_mem *heap_end;

#define MIN_SIZE 12
#define MIN_SIZE_ALIGNED     ROUND_UP(MIN_SIZE, ALIGN_SIZE)
#define SIZEOF_STRUCT_MEM    ROUND_UP(sizeof(struct heap_mem), ALIGN_SIZE)

static struct heap_mem *lfree;   /* pointer to the lowest free block */

static mutex_t heap_sem;
static size_t mem_size_aligned;

#ifdef MEM_STATS
static size_t used_mem, max_mem;
#endif

static void plug_holes(struct heap_mem *mem)
{
	struct heap_mem *nmem;
	struct heap_mem *pmem;

	ASSERT((uint8_t *)mem >= heap_ptr);
	ASSERT((uint8_t *)mem < (uint8_t *)heap_end);
	ASSERT(mem->used == 0);

	/* plug hole forward */
	nmem = (struct heap_mem *)&heap_ptr[mem->next];
	if (mem != nmem && nmem->used == 0 && (uint8_t *)nmem != (uint8_t *)heap_end)
	{
		/* if mem->next is unused and not end of heap_ptr, combine mem and mem->next */
		if (lfree == nmem)
		{
			lfree = mem;
		}
		mem->next = nmem->next;
		((struct heap_mem *)&heap_ptr[nmem->next])->prev = (uint8_t *)mem - heap_ptr;
	}

	/* plug hole backward */
	pmem = (struct heap_mem *)&heap_ptr[mem->prev];
	if (pmem != mem && pmem->used == 0)
	{
		/* if mem->prev is unused, combine mem and mem->prev */
		if (lfree == mem)
		{
			lfree = pmem;
		}
		pmem->next = mem->next;
		((struct heap_mem *)&heap_ptr[mem->next])->prev = (uint8_t *)pmem - heap_ptr;
	}
}

/**
 * @ingroup grp_sysinit     system initialization
 *
 * This function will init system heap
 *
 * @param begin_addr the beginning address of system page
 * @param end_addr the end address of system page
 *
 */
void system_heap_init(void *begin_addr, void *end_addr)
{
	struct heap_mem *mem;
	uint32_t begin_align = ROUND_UP(begin_addr, ALIGN_SIZE);
	uint32_t end_align = ROUND_DOWN(end_addr, ALIGN_SIZE);

	/* alignment addr */
	if ((end_align > (2 * SIZEOF_STRUCT_MEM)) &&
		((end_align - 2 * SIZEOF_STRUCT_MEM) >= begin_align))
	{
		/* calculate the aligned memory size */
		mem_size_aligned = end_align - begin_align - 2 * SIZEOF_STRUCT_MEM;
	}
	else
	{
		kprintf("mem init, error begin address 0x%x, and end address 0x%x\n", (uint32_t)begin_addr, (uint32_t)end_addr);
		return;
	}

	/* point to begin address of heap */
	heap_ptr = (uint8_t *)begin_align;
    
	DEBUG_LOG(DEBUG_MEM,
		("mem init, heap begin address 0x%x, size %d\n", (uint32_t)heap_ptr, mem_size_aligned));

	/* initialize the start of the heap */
	mem = (struct heap_mem *)heap_ptr;
	mem->magic= HEAP_MAGIC;
	mem->next = mem_size_aligned + SIZEOF_STRUCT_MEM;
	mem->prev = 0;
	mem->used = 0;

	/* initialize the end of the heap */
	heap_end = (struct heap_mem *)&heap_ptr[mem->next];
	heap_end->magic= HEAP_MAGIC;
	heap_end->used = 1;
	heap_end->next = mem_size_aligned + SIZEOF_STRUCT_MEM;
	heap_end->prev = mem_size_aligned + SIZEOF_STRUCT_MEM;

    mutex_init( &heap_sem );

	/* initialize the lowest-free pointer to the start of the heap */
	lfree = (struct heap_mem *)heap_ptr;
}

/**
 * @addtogroup MM
 * @{
 */


/**
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size is the minimum size of the requested block in bytes.
 *
 * @return pointer to allocated memory or NULL if no free memory was found.
 */
void *malloc(size_t size)
{
	size_t ptr, ptr2;
	struct heap_mem *mem, *mem2;

	if (size == 0) return NULL;

	if (size != ALIGN(size, ALIGN_SIZE))
		DEBUG_LOG(DEBUG_MEM, ("malloc size %d, but align to %d\n", size, ALIGN(size, ALIGN_SIZE)));
	else
		DEBUG_LOG(DEBUG_MEM, ("malloc size %d\n", size));

	/* alignment size */
	size = ALIGN(size, ALIGN_SIZE);

	if (size > mem_size_aligned)
	{
		DEBUG_LOG(DEBUG_MEM, ("no memory\n"));

		return NULL;
	}

	/* every data block must be at least MIN_SIZE_ALIGNED long */
	if (size < MIN_SIZE_ALIGNED) size = MIN_SIZE_ALIGNED;

	/* take memory semaphore */
	mutex_lock(&heap_sem, -1);

	for (ptr = (uint8_t *)lfree - heap_ptr; ptr < mem_size_aligned - size;
         ptr = ((struct heap_mem *)&heap_ptr[ptr])->next)
	{
		mem = (struct heap_mem *)&heap_ptr[ptr];

		if ((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size)
		{
			/* mem is not used and at least perfect fit is possible:
			 * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

			if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED))
			{
				/* (in addition to the above, we test if another struct heap_mem (SIZEOF_STRUCT_MEM) containing
				 * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
				 * -> split large block, create empty remainder,
				 * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
				 * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
				 * struct heap_mem would fit in but no data between mem2 and mem2->next
				 * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
				 *       region that couldn't hold data, but when mem->next gets freed,
				 *       the 2 regions would be combined, resulting in more free memory
				 */
				ptr2 = ptr + SIZEOF_STRUCT_MEM + size;

				/* create mem2 struct */
				mem2 = (struct heap_mem *)&heap_ptr[ptr2];
				mem2->used = 0;
				mem2->next = mem->next;
				mem2->prev = ptr;

				/* and insert it between mem and mem->next */
				mem->next = ptr2;
				mem->used = 1;

				if (mem2->next != mem_size_aligned + SIZEOF_STRUCT_MEM)
				{
					((struct heap_mem *)&heap_ptr[mem2->next])->prev = ptr2;
				}
#ifdef MEM_STATS
				used_mem += (size + SIZEOF_STRUCT_MEM);
				if (max_mem < used_mem) max_mem = used_mem;
#endif
			}
			else
			{
				/* (a mem2 struct does no fit into the user data space of mem and mem->next will always
				 * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
				 * take care of this).
				 * -> near fit or excact fit: do not split, no mem2 creation
				 * also can't move mem->next directly behind mem, since mem->next
				 * will always be used at this point!
				 */
				mem->used = 1;
#ifdef MEM_STATS
				used_mem += mem->next - ((uint8_t*)mem - heap_ptr);
				if (max_mem < used_mem) max_mem = used_mem;
#endif
			}
			/* set memory block magic */
			mem->magic = HEAP_MAGIC;

			if (mem == lfree)
			{
				/* Find next free block after mem and update lowest free pointer */
				while (lfree->used && lfree != heap_end) lfree = (struct heap_mem *)&heap_ptr[lfree->next];

				ASSERT(((lfree == heap_end) || (!lfree->used)));
			}

			mutex_unlock(&heap_sem);
			ASSERT((uint32_t)mem + SIZEOF_STRUCT_MEM + size <= (uint32_t)heap_end);
 			ASSERT((uint32_t)((uint8_t *)mem + SIZEOF_STRUCT_MEM) % ALIGN_SIZE == 0);
			ASSERT((((uint32_t)mem) & (ALIGN_SIZE-1)) == 0);

			DEBUG_LOG(DEBUG_MEM, ("allocate memory at 0x%x, size: %d\n", 
				(uint32_t)((uint8_t *)mem + SIZEOF_STRUCT_MEM),
				(uint32_t)(mem->next - ((uint8_t *)mem - heap_ptr))));

			OBJECT_HOOK_CALL(malloc_hook, (((void*)((uint8_t *)mem + SIZEOF_STRUCT_MEM)), size));
			/* return the memory data except mem struct */
			return (uint8_t *)mem + SIZEOF_STRUCT_MEM;
		}
	}

	mutex_unlock(&heap_sem);
	return NULL;
}

/**
 * This function will change the previously allocated memory block.
 *
 * @param rmem pointer to memory allocated by malloc
 * @param newsize the required new size
 *
 * @return the changed memory block address
 */
void *realloc(void *rmem, size_t newsize)
{
	size_t size;
	size_t ptr, ptr2;
	struct heap_mem *mem, *mem2;
	void *nmem;

	/* alignment size */
	newsize = ALIGN(newsize, ALIGN_SIZE);
	if (newsize > mem_size_aligned)
	{
		DEBUG_LOG(DEBUG_MEM, ("realloc: out of memory\n"));

		return NULL;
	}

	/* allocate a new memory block */
	if (rmem == NULL)
		return malloc(newsize);

	mutex_lock(&heap_sem, -1);

	if ((uint8_t *)rmem < (uint8_t *)heap_ptr ||
		(uint8_t *)rmem >= (uint8_t *)heap_end)
	{
		/* illegal memory */
		mutex_unlock(&heap_sem);
		return rmem;
	}

	mem = (struct heap_mem *)((uint8_t *)rmem - SIZEOF_STRUCT_MEM);

	ptr = (uint8_t *)mem - heap_ptr;
	size = mem->next - ptr - SIZEOF_STRUCT_MEM;
	if (size == newsize)
	{
		/* the size is the same as */
		mutex_unlock(&heap_sem);
		return rmem;
	}

	if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE < size)
	{
		/* split memory block */
#ifdef MEM_STATS
  		used_mem -= (size - newsize);
#endif

		ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
		mem2 = (struct heap_mem *)&heap_ptr[ptr2];
		mem2->magic= HEAP_MAGIC;
		mem2->used = 0;
		mem2->next = mem->next;
		mem2->prev = ptr;
		mem->next = ptr2;
		if (mem2->next != mem_size_aligned + SIZEOF_STRUCT_MEM)
		{
			((struct heap_mem *)&heap_ptr[mem2->next])->prev = ptr2;
		}

		plug_holes(mem2);

		mutex_unlock(&heap_sem);
		return rmem;
	}
	mutex_unlock(&heap_sem);

	/* expand memory */
	nmem = malloc(newsize);
	if (nmem != NULL) /* check memory */
	{
		memcpy(nmem, rmem, size < newsize ? size : newsize);	
		free(rmem);
	}

	return nmem;
}

/**
 * This function will contiguously allocate enough space for count objects
 * that are size bytes of memory each and returns a pointer to the allocated
 * memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 *
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *calloc(size_t count, size_t size)
{
	void *p;

	/* allocate 'count' objects of size 'size' */
	p = malloc(count * size);

	/* zero the memory */
	if (p) memset(p, 0, count * size);

	return p;
}

/**
 * This function will release the previously allocated memory block by malloc.
 * The released memory block is taken back to system heap.
 *
 * @param rmem the address of memory which will be released
 */
void free(void *rmem)
{
	struct heap_mem *mem;

	if (rmem == NULL) return;
	ASSERT((((uint32_t)rmem) & (ALIGN_SIZE-1)) == 0);
	ASSERT((uint8_t *)rmem >= (uint8_t *)heap_ptr &&
			  (uint8_t *)rmem < (uint8_t *)heap_end);

	OBJECT_HOOK_CALL(free_hook, (rmem));

	if ((uint8_t *)rmem < (uint8_t *)heap_ptr || (uint8_t *)rmem >= (uint8_t *)heap_end)
	{
		DEBUG_LOG(DEBUG_MEM, ("illegal memory\n"));

		return;
	}

	/* Get the corresponding struct heap_mem ... */
	mem = (struct heap_mem *)((uint8_t *)rmem - SIZEOF_STRUCT_MEM);

	DEBUG_LOG(DEBUG_MEM, ("release memory 0x%x, size: %d\n", 
		(uint32_t)rmem, 
		(uint32_t)(mem->next - ((uint8_t *)mem - heap_ptr))));


	/* protect the heap from concurrent access */
	mutex_lock(&heap_sem, -1);

	/* ... which has to be in a used state ... */
	ASSERT(mem->used);
	ASSERT(mem->magic == HEAP_MAGIC);
	/* ... and is now unused. */
	mem->used = 0;
	mem->magic = 0;

	if (mem < lfree)
	{
		/* the newly freed struct is now the lowest */
		lfree = mem;
	}

#ifdef MEM_STATS
	used_mem -= (mem->next - ((uint8_t*)mem - heap_ptr));
#endif

	/* finally, see if prev or next are free also */
	plug_holes(mem);
	mutex_unlock(&heap_sem);
}

#ifdef MEM_STATS
void memory_info(uint32_t *total, uint32_t *used, uint32_t *max_used)
{
	if (total != NULL) *total = mem_size_aligned;
	if (used  != NULL) *used = used_mem;
	if (max_used != NULL) *max_used = max_mem;
}


/*@}*/

#endif
