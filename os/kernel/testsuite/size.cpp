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

#include <es.h>
#include <es/base/ICache.h>
#include <es/exception.h>
#include <iostream>
#include "memoryStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    long long pageTableSize = 32 * 1024LL;            // 32KB
    long long hugeSize = 1024 * 1024 * 1024 * 1024LL; //  1TB

    IInterface* root = NULL;

    esInit(&root);

    esReport("Check getSize() and setSize().\n");

    ICacheFactory* cacheFactory = 0;
    esCreateInstance(CLSID_CacheFactory,
                     IID_ICacheFactory,
                     reinterpret_cast<void**>(&cacheFactory));

    MemoryStream* backingStore = new MemoryStream(16);
    TEST(backingStore);

    ICache* cache = cacheFactory->create(backingStore);
    TEST(cache);

    IStream* stream = cache->getStream();
    TEST(stream);

    long long size;
    long long newsize = 0;
    while (newsize <= pageTableSize +  4 * 1024)
    {
        try
        {
            stream->setSize(newsize);
            size = stream->getSize();
            TEST(size == newsize);
            newsize += 4 * 1024;
        }
        catch (...)
        {
            esReport("Caught an exception.\n");
            return 1;
        }
    }

    while (0 <= newsize)
    {
        try
        {
            stream->setSize(newsize);
            size = stream->getSize();
            TEST(size == newsize);
            newsize -= 4 * 1024;
        }
        catch (...)
        {
            esReport("Caught an exception.\n");
            return 1;
        }
    }

    newsize = hugeSize;
    try
    {
        stream->setSize(newsize);
        newsize = stream->getSize();
        TEST(size == newsize);
    }
    catch (SystemException<ENOSPC>& e)
    {
        // No space left on device
        esReport("%d\n", e.getResult());
        size = 1024LL;
        stream->setSize(size);
    }
    // check ICache->getSize() and setSize().
    long long cacheSize;

    try
    {
        size = stream->getSize();
        cacheSize = cache->getSize();
        TEST(size == cacheSize);

        stream->setSize(--size);
        cacheSize = cache->getSize();
        TEST(size == cacheSize);

        cache->setSize(--cacheSize);
        size = stream->getSize();
        TEST(size == cacheSize);

        stream->release();
        cache->release();
    }
    catch (...)
    {
        esReport("Caught an exception.\n");
        return 1;
    }

    esReport("done.\n");
}
