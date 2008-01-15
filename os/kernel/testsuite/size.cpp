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

#include <es.h>
#include <es/base/ICache.h>
#include <es/exception.h>
#include <iostream>
#include "core.h"
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
    cacheFactory = reinterpret_cast<ICacheFactory*>(
        esCreateInstance(CLSID_CacheFactory, ICacheFactory::iid()));

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
