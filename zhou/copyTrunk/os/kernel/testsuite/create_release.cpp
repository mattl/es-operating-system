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

const u8 Pattern[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

int main()
{
    long long size;

    Object* root = 0;
    esInit(&root);

    MemoryStream* backingStore = new MemoryStream(0);


    // create, write and release.
    int i;
    for (i = 0; i < 100; ++i)
    {
#ifdef VERBOSE
        esReport("%d\n", i);
#endif // VERBOSE
        es::Cache* cache = es::Cache::createInstance(backingStore);
        es::Stream* stream = cache->getStream();

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
