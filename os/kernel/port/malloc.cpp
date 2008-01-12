/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include <new>
#include <stdlib.h>
#include "heap.h"

namespace
{
    // Preserve es.ldr area for VESA BIOS support
    Arena   arena((void*) 0x80100000, 15*1024*1024);

    // We do not want destructor of this heap to be called.
    u8      heapPlace[sizeof(Heap)];
    Heap*   heap;

    void heapConstructor(void) __attribute__ ((constructor));
    void heapConstructor(void)
    {
        heap = new(heapPlace) Heap(arena);
    }
}

void* calloc(size_t count, size_t size)
{
    size *= count;
    void* ptr = malloc(size);
    if (ptr)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}

void* malloc(size_t size)
{
    return heap->alloc(size);
}

void free(void* ptr)
{
    heap->free(ptr);
}

void* realloc(void* ptr, size_t size)
{
    return heap->realloc(ptr, size);
}

void* _malloc_r(_reent*, size_t size)
{
    return malloc(size);
}

void _free_r(_reent*, void* ptr)
{
    free(ptr);
}

void* _realloc_r(_reent*, void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void* _calloc_r(_reent*, size_t count, size_t size)
{
    return calloc(count, size);
}
