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

#include <es.h>
#include "heap.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

u8 buffer[1024 * 1024];

Arena arena;

int main()
{
    Object* root = 0;
    esInit(&root);

    void*  a;
    void*  b;
    void*  c;

    arena.free(buffer, sizeof buffer);

    //
    // Test alloc()
    //
    a = arena.alloc(128, 512);
    TEST(reinterpret_cast<size_t>(a) % 512 == 0);

    b = arena.alloc(128, 256);
    TEST(reinterpret_cast<size_t>(b) % 256 == 0);

    arena.free(b, 128);
    arena.free(a, 128);
    TEST(arena.size() == sizeof buffer);

    //
    // Test allocLast()
    //
    a = arena.allocLast(128, 512);
    TEST(reinterpret_cast<size_t>(a) % 512 == 0);

    b = arena.allocLast(128, 256);
    TEST(reinterpret_cast<size_t>(b) % 256 == 0);

    arena.free(b, 128);
    arena.free(a, 128);
    TEST(arena.size() == sizeof buffer);

    //
    // Test Heap{}
    //
    Heap heap(arena);
    void* d[12];
    void* l;
    int i;

    // Small object
    for (i = 0; i < 12; ++i)
    {
        d[i] = heap.alloc(800);
        TEST(reinterpret_cast<size_t>(d[i]) % Arena::ALIGN == 0);
    }
    for (i = 0; i < 12; ++i)
    {
        heap.free(d[i]);
    }
    TEST(arena.size() == sizeof buffer);

    // Large object
    l = heap.alloc(4 * 1024);
    heap.free(l);
    TEST(arena.size() == sizeof buffer);

    esReport("done.\n");
}
