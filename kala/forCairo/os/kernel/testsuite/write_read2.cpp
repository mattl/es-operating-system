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
#include <es/ref.h>
#include <es/handle.h>
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

static long Verify(long size, MemoryStream* backingStore)
{
    es::Cache* cache = es::Cache::createInstance(backingStore);
    if (!cache)
    {
        esReport("Unable to create cache\n");
        return -1;
    }

    es::Stream* stream = cache->getStream();
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

    cache = es::Cache::createInstance(backingStore);
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

    Object* root = NULL;

    esInit(&root);
    esReport("Check read() and write().\n");

    MemoryStream* backingStore = new MemoryStream(0);
    if (!backingStore)
    {
        esReport("Bad alloc. (backingStore)\n");
        return 1;
    }

    long size;

    // write and read data less than the size of a page.
    result = Verify(1, backingStore);
    if (result < 0)
    {
        goto ERROR;
    }

    result = Verify(8, backingStore);
    if (result < 0)
    {
        goto ERROR;
    }

    result = Verify(PAGE_TABLE_SIZE, backingStore);
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
