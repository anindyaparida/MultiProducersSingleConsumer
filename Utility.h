/*
	Common function for memory allocation
*/
#include <atomic>
#include <iostream>
#include <string.h>
#include <thread>
#include <stdlib.h>

static inline void * allocateMem(size_t align, size_t size)
{
	void * ptr = aligned_alloc(align, size);
	return ptr;
}

static inline void freeMem(void * ptr)
{
	free(ptr);
}
