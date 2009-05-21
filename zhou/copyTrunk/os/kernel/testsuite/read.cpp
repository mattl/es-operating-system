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

#define PAGE_SIZE       (4 * 1024)
#define PAGE_TABLE_SIZE (16 * PAGE_SIZE)
#define BUF_SIZE        (PAGE_TABLE_SIZE)
static  u8 ReadBuf[BUF_SIZE];
static  u8 WriteBuf[BUF_SIZE];

static void SetData(u8* buf, long size)
{
    while (0 < size)
    {
        *buf++ = 'A' + size-- % 26;
    }
}

static long PacketRead(es::Stream* stream, long size, long offset, long packetSize)
{
    long ret;
    long len = 0;

    while (len < size)
    {
        if (size - len < packetSize)
        {
            packetSize = size - len;
        }
        ret = stream->read(ReadBuf+len, packetSize, offset);

        if (ret != packetSize)
        {
            ret = -1;
        }
#ifdef VERBOSE
        esReport("  read:  %5d (%d - %d) \n", ret, len, len+ret-1);
#endif // VERBOSE
        len += ret;
        offset += ret;
    }

    return len;
}

static long Verify(long size, long offset, MemoryStream* backingStore, long paketUnit)
{
    long ret =  -1;

    try
    {
        backingStore->setSize(size + offset);
    }
    catch (...)
    {
        esReport("Caught an exception.\n");
        return -1;
    }

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

#ifdef VERBOSE
    esReport("Test [size %d, offset %d]\n", size, offset);
#endif // VERBOSE

    // write at once.
    ret = stream->write(WriteBuf, size, offset);
    stream->flush();

    stream->release();
    cache->release();

#ifdef VERBOSE
    esReport("  write: %5d\n", ret);
#endif // VERBOSE

    // read, changing number of packets.
    cache = es::Cache::createInstance(backingStore);
    stream = cache->getStream();
    ret = PacketRead(stream, size, offset, paketUnit);
    if (ret != size)
    {
        ret = -1;
        goto ERROR;
    }

    if (memcmp(ReadBuf, WriteBuf, ret) != 0)
    {
        ret = -1;
    }
    else
    {
        ret = 0;
#ifdef VERBOSE
        esReport("done.\n");
#endif // VERBOSE
    }

ERROR:
    stream->release();
    cache->release();

    return ret;
}

int main()
{
    int result = -1;

    Object* root = NULL;

    esInit(&root);
    esReport("Check read() and write().\n");

    Handle<es::Context> context = root;

    long offset = 0;
    long size = PAGE_SIZE;

    MemoryStream* backingStore = new MemoryStream(0);
    if (!backingStore)
    {
        esReport("Bad alloc. (backingStore)\n");
        return 1;
    }

    // read one byte at a time.
    result = Verify(size, offset, backingStore, 1);
    if (result < 0)
    {
        goto ERROR;
    }

    // read 6KB at a time.
    size = 2 * PAGE_SIZE;
    result = Verify(size, offset, backingStore, 6 * 1024);
    if (result < 0)
    {
        goto ERROR;
    }

    // read 8KB at a time from the offset.
    offset = 100;
    size = 4 * PAGE_SIZE;
    result = Verify(size, offset, backingStore, 8 * 1024);
    if (result < 0)
    {
        goto ERROR;
    }

    // read 64KB at a time.
    offset = 0;
    size = BUF_SIZE;
    result = Verify(size, offset, backingStore, PAGE_TABLE_SIZE);
    if (result < 0)
    {
        goto ERROR;
    }

    backingStore->release();
    esReport("done.\n");
    return 0;

ERROR:
    backingStore->release();
    esReport("*** error ***\n");
    return 1;
}
