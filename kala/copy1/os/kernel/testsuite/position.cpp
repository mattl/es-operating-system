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

#include <new>
#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/ref.h>
#include <es/base/ICache.h>
#include "core.h"
#include "memoryStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    Object* root = NULL;
    esInit(&root);
    Handle<es::Context> context = root;

    MemoryStream* backingStore = new(std::nothrow) MemoryStream(128*1024);
    TEST(backingStore);

    es::Cache* cache = es::Cache::createInstance(backingStore);
    TEST(cache);

    es::Stream* stream = cache->getStream();
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
