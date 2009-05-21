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

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

#define PAGE_SIZE        (4 * 1024)
#define PAGE_TABLE_SIZE  (16 * PAGE_SIZE)
#define BUF_SIZE         (16 * PAGE_TABLE_SIZE)

static  u8 ReadBuf[BUF_SIZE];
static  u8 WriteBuf[BUF_SIZE];
static  u8 Invalidated[BUF_SIZE];

static void SetData(u8* buf, long size)
{
    while (0 < size)
    {
        *buf++ = 'A' + size-- % 26;
    }
}

static void InitMemoryStream(MemoryStream* backingStore, long size, long offset)
{
    es::Cache* cache = es::Cache::createInstance(backingStore);
    TEST(cache);

    es::Stream* stream = cache->getStream();
    TEST(stream);

    SetData(WriteBuf, size);

    long ret;
    ret = stream->write(WriteBuf, size, offset);
    TEST(ret == size);

#ifdef VERBOSE
    esReport("write: %d -> ", ret);
#endif // VERBOSE
    stream->flush();

    stream->release();
    cache->release();

    cache = es::Cache::createInstance(backingStore);
    stream = cache->getStream();
    ret = stream->read(ReadBuf, size, offset);
    TEST(ret == size);
#ifdef VERBOSE
    esReport("read: %d\n", ret);
#endif // VERBOSE

    TEST(memcmp(ReadBuf, WriteBuf, ret) == 0);

    stream->release();
    cache->release();
}

int main()
{
    es::Cache* cache;
    es::Stream* stream;
    int result = -1;
    Object* root = NULL;

    esInit(&root);

    esReport("Check invalidate().\n");

    MemoryStream* backingStore = new MemoryStream(PAGE_SIZE);
    TEST(backingStore);

    // Write data to the memory stream.
    InitMemoryStream(backingStore, PAGE_SIZE, 0);

    // Write data and invalidate them.
    cache = es::Cache::createInstance(backingStore);
    TEST(cache);
    stream = cache->getStream();
    TEST(stream);

    memset(Invalidated, 0, PAGE_SIZE);
    result = stream->write(Invalidated, PAGE_SIZE, 0);
    TEST(result == PAGE_SIZE);
    cache->invalidate();

    stream->release();
    cache->release();

    // check the momory stream is not modified.
    cache = es::Cache::createInstance(backingStore);
    TEST(cache);
    stream = cache->getStream();
    TEST(stream);

    result = stream->read(ReadBuf, PAGE_SIZE, 0);
    TEST(result == PAGE_SIZE);

    TEST(memcmp(WriteBuf, ReadBuf, PAGE_SIZE) == 0);

    stream->release();
    cache->release();

    backingStore->release();

    esReport("done.\n");
}
