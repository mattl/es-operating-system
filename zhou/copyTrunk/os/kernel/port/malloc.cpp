/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <new>
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
#ifdef NDEBUG
    return heap->alloc(size);
#else
    void* ptr = heap->alloc(size);
    if (ptr)
    {
        memset(ptr, 0xf5, size);
    }
    return ptr;
#endif
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
