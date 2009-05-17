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

#include <stdio.h>
#include <es.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    Object* root = NULL;
    esInit(&root);

    int rc;
    unsigned int key1;
    unsigned int key2;
    unsigned int key3;
    rc = esCreateThreadKey(&key1, 0);
    esReport("esCreateThreadKey: %d %u\n", rc, key1);
    TEST(rc == 0);
    TEST(0 <= key1);
    rc = esCreateThreadKey(&key2, 0);
    esReport("esCreateThreadKey: %d %u\n", rc, key2);
    TEST(rc == 0);
    TEST(0 <= key2);
    TEST(key1 != key2);

    rc = esDeleteThreadKey(key1);
    TEST(rc == 0);

    rc = esCreateThreadKey(&key1, 0);
    esReport("esCreateThreadKey: %d %u\n", rc, key1);
    TEST(rc == 0);
    TEST(0 <= key1);

    rc = esCreateThreadKey(&key3, 0);
    esReport("esCreateThreadKey: %d %u\n", rc, key3);
    TEST(rc == 0);
    TEST(0 <= key3);

    rc = esSetThreadSpecific(key1, (const void*) 0x1234);
    TEST(rc == 0);
    void* ptr = esGetThreadSpecific(key1);
    esReport("esGetThreadSpecific: %x\n", ptr);
    TEST(ptr == (const void*) 0x1234);

    esReport("done.\n");
}
