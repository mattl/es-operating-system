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

#include <new>
#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/ref.h>
#include <es/clsid.h>
#include <es/interlocked.h>
#include <es/base/ICache.h>
#include "memoryStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    ICacheFactory* cacheFactory = 0;
    esCreateInstance(CLSID_CacheFactory,
                     IID_ICacheFactory,
                     reinterpret_cast<void**>(&cacheFactory));

    MemoryStream* backingStore = new(std::nothrow) MemoryStream(128*1024);
    TEST(backingStore);

    ICache* cache = cacheFactory->create(backingStore);
    TEST(cache);

    IStream* stream = cache->getStream();
    TEST(stream);

    long long size;
    size = stream->getSize();

    // check if the position can be set correctly.
    long long pos;
    long long current;
    for (pos = 0; pos <= size; pos += 1023)
    {
        stream->setPosition(pos);
        current = stream->getPosition();
        TEST(current == pos);
    }

    // confirm the position is less than the maximum size.
    stream->setPosition(size + 1);
    current = stream->getSize();
    TEST(current == size);

    stream->release();
    cache->release();
    esReport("done.\n");
}
