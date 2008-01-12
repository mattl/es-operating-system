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

#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/ref.h>
#include <es/clsid.h>
#include <es/interlocked.h>
#include <es/base/ICache.h>
#include "memoryStream.h"
#include "core.h"

#define PAGE_SIZE        (4 * 1024)
#define BUF_SIZE         (1024 * 1024)
#define NON_RESERVED_PAGE  4

static  u8 WriteBuf[BUF_SIZE];
static  u8 WriteBuf2[BUF_SIZE];

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static void SetData(u8* buf, long size)
{
    memset(buf, size, size % 0xff);
}

int main()
{
    IInterface* root = NULL;

    esInit(&root);
    unsigned long maxFreeCount = PageTable::getFreeCount();

    esReport("Check getPageCount().\n");

    // Reserved all pages except for NON_RESERVER_PAGE pages.
    IPageSet* pageSet;
    pageSet = reinterpret_cast<IPageSet*>(
        esCreateInstance(CLSID_PageSet, IPageSet::iid()));
    unsigned long reserved = maxFreeCount - NON_RESERVED_PAGE;
    pageSet->reserve(reserved);

    // The tests are carried out using the non-reserved pages.
    maxFreeCount = NON_RESERVED_PAGE;

#ifdef VERBOSE
    esReport("Max free count: %d\n", maxFreeCount);
#endif // VERBOSE

    ICacheFactory* cacheFactory = 0;
    cacheFactory = reinterpret_cast<ICacheFactory*>(
        esCreateInstance(CLSID_CacheFactory, ICacheFactory::iid()));

    MemoryStream* backingStore = new MemoryStream(16);
    if (!backingStore)
    {
        esReport("Bad alloc. (backingStore)\n");
        return 1;
    }

    ICache* cache1 = cacheFactory->create(backingStore);
    if (!cache1)
    {
        esReport("Bad alloc. (cache)\n");
        return 1;
    }

    IStream* stream1 = cache1->getStream();
    if (!stream1)
    {
        esReport("Bad alloc. (stream1)\n");
        return 1;
    }

    MemoryStream* backingStore2 = new MemoryStream(16);
    if (!backingStore2)
    {
        esReport("Bad alloc. (backingStore2)\n");
        return 1;
    }

    ICache* cache2 = cacheFactory->create(backingStore2);
    if (!cache2)
    {
        esReport("Bad alloc. (cache2)\n");
        return 1;
    }

    IStream* stream2 = cache2->getStream();
    if (!stream2)
    {
        esReport("Bad alloc. (stream2)\n");
        return 1;
    }

    unsigned long pageCount1;
    unsigned long pageCount2;
    pageCount1 = cache1->getPageCount();
    pageCount2 = cache2->getPageCount();
#ifdef VERBOSE
    esReport("page count1: %d, page count2: %d\n", pageCount1, pageCount2);
#endif // VERBOSE

    long long size;
    long long offset;
    long ret1;
    long ret2;

    // Associate all pages to cache 1.
    size = maxFreeCount * PAGE_SIZE;
    offset = 0;

    SetData(WriteBuf, size);
    ret1 = stream1->write(WriteBuf, size, offset);
    stream1->flush();

    pageCount1 = cache1->getPageCount();
#ifdef VERBOSE
    esReport("page count1: %d (%d)\n", pageCount1, ret1);
#endif // VERBOSE
    TEST(pageCount1 == maxFreeCount);

    // Associate one page to cache 2.
    size = PAGE_SIZE;
    offset = 0;
    ret2 = stream2->write(WriteBuf2, size, offset);
    stream2->flush();
    pageCount2 = cache2->getPageCount();
#ifdef VERBOSE
    esReport("page count2: %d (%d)\n", pageCount2, ret2);
#endif // VERBOSE
    TEST(pageCount2 == 1);

    // check current pages associated to cache1.
    pageCount1 = cache1->getPageCount();
#ifdef VERBOSE
    esReport("page count1: %d \n", pageCount1);
#endif // VERBOSE
    TEST(pageCount1 == maxFreeCount - 1);

    stream1->release();
    cache1->release();
    stream2->release();
    cache2->release();

    esReport("done.\n");
    return 0;
}
