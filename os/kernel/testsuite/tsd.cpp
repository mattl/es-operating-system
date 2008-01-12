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

#include <stdio.h>
#include <es.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    IInterface* root = NULL;
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
