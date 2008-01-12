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
#include "core.h"
#include "memoryStream.h"

// #define VERBOSE

#define PAGE_SIZE        (4 * 1024)
#define PAGE_TABLE_SIZE  (16 * PAGE_SIZE)
#define BUF_SIZE         (PAGE_TABLE_SIZE)

static  u8 ReadBuf[BUF_SIZE];
static  u8 WriteBuf[BUF_SIZE];

static void SetData(u8* buf, long size)
{
    while (0 < size)
    {
        *buf++ = 'A' + size-- % 26;
    }
}

static long Verify(ICacheFactory* cacheFactory, long size, MemoryStream* backingStore)
{
    ICache* cache = cacheFactory->create(backingStore);
    if (!cache)
    {
        esReport("Unable to create cache\n");
        return -1;
    }

    IStream* stream = cache->getStream();
    if (!stream)
    {
        esReport("Unable to create stream\n");
        return -1;
    }

    SetData(WriteBuf, size);

    long ret;
    ret = stream->write(WriteBuf, size);
    if (ret != size)
    {
        ret = -1;
        goto ERROR;
    }

#ifdef VERBOSE
    esReport("-> write: %d ", ret);
#endif // VERBOSE
    stream->flush();

    stream->release();
    cache->release();

    cache = cacheFactory->create(backingStore);
    stream = cache->getStream();
    ret = stream->read(ReadBuf, size);
    if (ret != size)
    {
        ret = -1;
        goto ERROR;
    }
#ifdef VERBOSE
    esReport("read: %d ", ret);
#endif // VERBOSE

    if (memcmp(ReadBuf, WriteBuf, ret) != 0)
    {
        ret = -1;
    }
    else
    {
        ret = 0;
    }

ERROR:
    stream->release();
    cache->release();

    return ret;
}

int main()
{
    int result = 1;

    IInterface* root = NULL;

    esInit(&root);
    esReport("Check read() and write().\n");

    ICacheFactory* cacheFactory = 0;
    esCreateInstance(CLSID_CacheFactory,
                     IID_ICacheFactory,
                     reinterpret_cast<void**>(&cacheFactory));

    MemoryStream* backingStore = new MemoryStream(0);
    if (!backingStore)
    {
        esReport("Bad alloc. (backingStore)\n");
        return 1;
    }

    long size;

    // write and read data less than the size of a page.
    result = Verify(cacheFactory, 1, backingStore);
    if (result < 0)
    {
        goto ERROR;
    }

    result = Verify(cacheFactory, 8, backingStore);
    if (result < 0)
    {
        goto ERROR;
    }

    result = Verify(cacheFactory, PAGE_TABLE_SIZE, backingStore);
    if (result < 0)
    {
        goto ERROR;
    }

    /* Omit due to the memory size limitation.
    result = Verify(cacheFactory, PAGE_TABLE_SIZE * 2, backingStore);
    if (result < 0)
    {
        goto ERROR;
    }
    */

    backingStore->release();
    esReport("done.\n");
    return 0;

ERROR:
    backingStore->release();
    esReport("*** ERROR ***\n");
    return 1;
}
