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

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

const u8 Pattern[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

int main()
{
    long long size;

    IInterface* root = 0;
    esInit(&root);

    ICacheFactory* cacheFactory = 0;
    esCreateInstance(CLSID_CacheFactory,
                     IID_ICacheFactory,
                     reinterpret_cast<void**>(&cacheFactory));

    MemoryStream* backingStore = new MemoryStream(0);


    // create, write and release.
    int i;
    for (i = 0; i < 100; ++i)
    {
#ifdef VERBOSE
        esReport("%d\n", i);
#endif // VERBOSE
        ICache* cache = cacheFactory->create(backingStore);
        IStream* stream = cache->getStream();

        stream->write(Pattern, 8);
        size = stream->getSize();
        TEST(size == 8);

        stream->flush();
        stream->release();
        cache->release();
    }

    delete backingStore;
    esReport("done.\n");
}
