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
#include <string.h>
#include <stdlib.h>
#include <es.h>
#include "core.h"
#include "process.h"

Swap::
SwapUse::SwapUse(unsigned long offset) :
    offset(offset)
{
    ASSERT(sizeof(SwapUse) == Page::SIZE);

    freeCount = NUM_ENTRIES;
    freeIndex = 0;
    for (u16 n = 0; n < NUM_ENTRIES; ++n)
    {
        map[n] = n + 1;
        ref[n] = 0;
    }
}

Swap::
Swap(Cache* cache) :
    cache(cache),
    size(0)
{
    ASSERT(sizeof(SwapUse) == Page::SIZE);
    ASSERT(cache);
    cache->addRef();
}

Swap::
~Swap()
{
    ASSERT(cache);
    // Release SwapUse pages
    for (unsigned long offset(0); offset < size; offset += Page::SIZE * (NUM_ENTRIES + 1))
    {
        Page* page = cache->getPage(offset);
        page->release();
        page->release();
    }
    cache->invalidate();
    cache->release();
}

Swap::SwapUse* Swap::
getSwapUse(unsigned long offset)
{
    Page* page = cache->getPage(offset - (offset % (Page::SIZE * (NUM_ENTRIES + 1))));
    if (!page)
    {
        return 0;
    }
    page->release();
    return static_cast<SwapUse*>(page->getPointer());
}

Page* Swap::
restore(unsigned long offset)
{
    Page* page = cache->getPage(offset);
    if (page)
    {
        SwapUse* swap = getSwapUse(offset);
        int n = (offset % (Page::SIZE * (NUM_ENTRIES + 1))) / Page::SIZE - 1;
        ASSERT(0 <= n && n < NUM_ENTRIES);
        ++swap->ref[n];

        page->fill(cache->backingStore);
    }
    return page;
}

// Looks up a free page and returns it. The returned page must be zero cleared.
Page* Swap::
get()
{
    if (freeList.isEmpty())
    {
        // Extend swap
        Page* page = cache->getPage(size);  // for incrementing refrence count
        if (!page)
        {
            return 0;
        }
        SwapUse* swap = getSwapUse(size);
        if (!swap)
        {
            return 0;
        }
        new(swap) SwapUse(size);
        freeList.addLast(swap);

        size += Page::SIZE * (NUM_ENTRIES + 1);
    }

    ASSERT(!freeList.isEmpty());
    SwapUse* swap = freeList.getFirst();
    ASSERT(0 < swap->freeCount);

    unsigned long offset = swap->offset + Page::SIZE * (1 + swap->freeIndex);
    ++swap->ref[swap->freeIndex];
    ASSERT(swap->ref[swap->freeIndex] == 1);
    swap->freeIndex = swap->map[swap->freeIndex];
    if (--swap->freeCount <= 0)
    {
        freeList.remove(swap);
        fullList.addLast(swap);
    }
    return cache->getPage(offset);
}

void Swap::
put(unsigned long offset)
{
    SwapUse* swap = getSwapUse(offset);
    int n = (offset % (Page::SIZE * (NUM_ENTRIES + 1))) / Page::SIZE - 1;
    ASSERT(0 <= n && n < NUM_ENTRIES);
    if (--swap->ref[n] == 0)
    {
        swap->map[n] = swap->freeIndex;
        swap->freeIndex = n;
        if (++swap->freeCount == 1)
        {
            fullList.remove(swap);
            freeList.addLast(swap);
        }
    }
}

void Swap::
put(Page* page)
{
    long long offset = page->getOffset();
    page->release();
    put(static_cast<unsigned long>(offset));
}
