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
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    Object* root;
    esInit(&root);

    unsigned addr;
    addr = 0x12345001;
    esReport("offset: %x\n", Page::pageOffset(addr));
    esReport("base: %x\n", Page::pageBase(addr));
    TEST(Page::pageOffset(addr) == 0x0001);
    TEST(Page::pageBase(addr) == 0x12345000);

    addr = 0x12345fff;
    esReport("offset: %x\n", Page::pageOffset(addr));
    esReport("base: %x\n", Page::pageBase(addr));
    TEST(Page::pageOffset(addr) == 0xfff);
    TEST(Page::pageBase(addr) == 0x12345000);

    void* ptr;
    ptr = (void*) 0x12345001;
    esReport("rount: %p\n", Page::round(ptr));
    esReport("trunc: %p\n", Page::trunc(ptr));
    TEST(Page::round(ptr) == (void*) 0x12346000);
    TEST(Page::trunc(ptr) == (void*) 0x12345000);

    esReport("done.\n");
}
