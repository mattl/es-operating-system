/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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
