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

#define VERBOSE

#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/ref.h>
#include <es/base/ICache.h>
#include "core.h"
#include "memoryStream.h"

#define PAGE_SIZE   4096
#define NON_RESERVED_PAGE  4

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static void setData(u8* buf, long size, u8 shift)
{
    for (long i = 0; i < size; ++i)
    {
        *buf++ = 'A' + (i + shift) % 26;
    }
}

int main()
{
    Object* root = NULL;

    esInit(&root);

    unsigned long long maxPage = PageTable::getFreeCount();
#ifdef VERBOSE
    esReport("maxPage: %lu\n", maxPage);
#endif // VERBOSE
    // Reserved all pages except for NON_RESERVER_PAGE pages.
    Handle<es::PageSet> pageSet = es::PageSet::createInstance();
    unsigned long long reserved = maxPage - NON_RESERVED_PAGE;
    pageSet->reserve(reserved);

    // The tests are carried out using the non-reserved pages.
    maxPage = NON_RESERVED_PAGE;
    int size1 = maxPage * PAGE_SIZE * 2;
    int size2 = maxPage * PAGE_SIZE * 2;
    u8* buf1;
    u8* buf2;

    try
    {
        buf1 = new u8[size1];
        setData(buf1, size1, 1);
        buf2 = new u8[size1];
        setData(buf2, size1, 2);
    }
    catch (...)
    {
        esReport("Caught an exception.\n");
        return 1;
    }

    MemoryStream* backingStore;
    MemoryStream* backingStore2;
    es::Stream* stream1;
    es::Stream* stream2;
    es::Cache* cache1;
    es::Cache* cache2;
    unsigned long long pageCount1;
    unsigned long long pageCount2;

    try
    {
        backingStore = new MemoryStream;
        TEST(backingStore);
        cache1 = es::Cache::createInstance(backingStore);
        TEST(cache1);
        stream1 = cache1->getStream();
        TEST(stream1);
        cache1->setSectorSize(512);

        backingStore2 = new MemoryStream;
        TEST(backingStore2);
        cache2 = es::Cache::createInstance(backingStore2);
        TEST(cache2);
        stream2 = cache2->getStream();
        TEST(stream2);
        cache1->setSectorSize(512);

        pageCount1 = cache1->getPageCount();
        pageCount2 = cache2->getPageCount();
    }
    catch (...)
    {
        esReport("Caught an exception.\n");
        return 1;
    }

#ifdef VERBOSE
    esReport("page count1: %d, page count2: %d\n", pageCount1, pageCount2);
#endif // VERBOSE

    long rc;

    try
    {
        // Associate all pages to cache 1.
        rc = stream1->write(buf1, size1, 0);
        TEST(rc == size1);
        stream1->flush();
    }
    catch (...)
    {
        esReport("Caught an exception.\n");
        return 1;
    }

    pageCount1 = cache1->getPageCount();
#ifdef VERBOSE
    esReport("page count1: %d\n", pageCount1);
#endif // VERBOSE
    TEST(pageCount1 == maxPage);

    // Steal one page from cache 1.
    rc = stream2->write(buf2, size2, 0);
    TEST(rc == size2);
    stream2->flush();
    pageCount2 = cache2->getPageCount();
#ifdef VERBOSE
    esReport("page count2: %d\n", pageCount2);
#endif // VERBOSE
    TEST(0 < pageCount2);

    // Check current pages associated to cache1.
    pageCount1 = cache1->getPageCount();
#ifdef VERBOSE
    esReport("page count1: %d \n", pageCount1);
#endif // VERBOSE
    TEST(pageCount1 < maxPage);

    // Verify cache1 content
    rc = stream1->read(buf1, size1, 0);
    TEST(rc == size1);
    setData(buf2, size1, 1);
#ifdef VERBOSE
    for (long offset = 0; offset < size1; offset += PAGE_SIZE)
    {
        esReport("%08x: %02x %02x\n", offset, buf1[offset], buf2[offset]);
    }
#endif // VERBOSE
    TEST(memcmp(buf1, buf2, rc) == 0);

    // Verify cache2 content
    rc = stream2->read(buf2, size2, 0);
    TEST(rc == size2);
    setData(buf1, size2, 2);
#ifdef VERBOSE
    for (long offset = 0; offset < size2; offset += PAGE_SIZE)
    {
        esReport("%08x: %02x %02x\n", offset, buf1[offset], buf2[offset]);
    }
#endif // VERBOSE
    TEST(memcmp(buf1, buf2, rc) == 0);

    stream1->release();
    cache1->release();
    stream2->release();
    cache2->release();

    esReport("done.\n");
}
