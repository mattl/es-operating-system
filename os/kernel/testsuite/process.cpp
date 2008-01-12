/*
 * Copyright (c) 2006, 2007
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

#include <stdarg.h>
#include <es.h>
#include <es/broker.h>
#include <es/handle.h>
#include <es/reflect.h>
#include <es/naming/IContext.h>
#include "core.h"
#include "interfaceStore.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    IInterface* system(0);
    esInit(&system);

    Handle<IContext> nameSpace(system);
    Handle<IPageable> framebuffer(nameSpace->lookup("device/framebuffer"));
    TEST(framebuffer);
    long long size;
    size = framebuffer->getSize();

    Process* process = new Process;
    process->load();

    process->trace(true);

    u8* ptr = (u8*) process->map(0, size,
                                 ICurrentProcess::PROT_READ | ICurrentProcess::PROT_WRITE,
                                 ICurrentProcess::MAP_SHARED,
                                 framebuffer, 0);
    esReport("map: %p\n", ptr);
    process->dump();
    TEST(ptr);
    TEST(Process::USER_MIN <= ptr && ptr < Process::USER_MAX);

    *ptr = 1;
    esReport("%u\n", *ptr);

    memset(ptr, 0x00, size);
    memset(ptr, 0xff, size);

    process->unmap(ptr, size);
    process->load();

    esReport("done.\n");
}
