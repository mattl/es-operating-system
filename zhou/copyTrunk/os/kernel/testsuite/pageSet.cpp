/*
 * Copyright 2008, 2009 Google Inc.
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

#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/interlocked.h>
#include <es/base/ICache.h>
#include "memoryStream.h"
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

#define PAGE_SIZE         (4 * 1024LL)
#define NON_RESERVED_PAGE  4

void WriteCache(es::Cache* cache, unsigned long numPage)
{
#ifdef VERBOSE
    esReport("Write %u pages\n", numPage);
#endif // VERBOSE

    try
    {
        long long size = (long long) numPage * PAGE_SIZE;
        u8* buf = new u8[size];
        TEST(buf);
        memset(buf, size, numPage % 0xff);

#ifdef VERBOSE
    esReport("size %llu\n", size);
#endif // VERBOSE
        Handle<es::Stream> stream = cache->getStream();
        TEST(stream);

        long long ret = stream->write(buf, size, 0);
        TEST(ret == size);
        stream->flush();

        delete [] buf;
    }
    catch (...)
    {
        esReport("Caught an exception.\n");
        TEST(0);
    }
}

void CheckAssociatedPage(es::Cache* cache, unsigned long numPage)
{
    unsigned long pageCount;
    pageCount = cache->getPageCount();
#ifdef VERBOSE
    esReport("Check cache 0x%p, page count: %d == %d\n", cache, numPage, pageCount);
#endif // VERBOSE
    TEST(pageCount == numPage);
}

int main()
{
    Object* root = 0;
    esInit(&root);

    unsigned long initialMaxFreeCount = PageTable::getFreeCount();
#ifdef VERBOSE
    esReport("Max free count: %d\n", initialMaxFreeCount);
#endif

    unsigned long maxFreeCount = PageTable::getFreeCount();

#ifdef VERBOSE
    esReport("Reserve (maxFreeCount - NON_RESERVED_PAGE) pages.\n");
#endif
    // Reserve (maxFreeCount - NON_RESERVED_PAGE) pages.
    Handle<es::PageSet> pageSet = es::PageSet::createInstance();
    unsigned long reserved = maxFreeCount - NON_RESERVED_PAGE;
    pageSet->reserve(reserved);

    // Check current free pages.
    maxFreeCount = PageTable::getFreeCount();
    TEST(initialMaxFreeCount == maxFreeCount + reserved);

#ifdef VERBOSE
    esReport("Max free count: %d, reserved %d\n", maxFreeCount, reserved);
#endif

    // Create 2 caches.
    // Cache1 uses the reserved pages.
    MemoryStream* backingStore1 = new MemoryStream(0);
    TEST(backingStore1);
    Handle<es::Cache> cache1 = es::Cache::createInstance(backingStore1, pageSet);
    TEST(cache1);

    // Cache2 uses non-reserved pages.
    MemoryStream* backingStore2 = new MemoryStream(0);
    TEST(backingStore2);
    Handle<es::Cache> cache2 = es::Cache::createInstance(backingStore2);
    TEST(cache2);

    CheckAssociatedPage(cache1, 0);
    CheckAssociatedPage(cache2, 0);

    // Associate all non-reserved pages to cache2.
    WriteCache(cache2, NON_RESERVED_PAGE);
    CheckAssociatedPage(cache2, NON_RESERVED_PAGE);

    // Associate one page to cache1.
    WriteCache(cache1, 1);
    CheckAssociatedPage(cache1, 1);
    // Confirm the number of the pages associated to cache2 is not changed.
    CheckAssociatedPage(cache2, NON_RESERVED_PAGE);

    WriteCache(cache2, NON_RESERVED_PAGE + 1);
    // Confirm reserved pages are not associated to cache2.
    CheckAssociatedPage(cache2, NON_RESERVED_PAGE);

    delete backingStore1;
    delete backingStore2;

    esReport("done.\n");
    root->release();
}
