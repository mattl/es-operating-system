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

#include <stdarg.h>
#include <es.h>
#include <es/broker.h>
#include <es/handle.h>
#include <es/reflect.h>
#include <es/naming/IContext.h>
#include "core.h"

Reflect::Interface* getInterface(const Guid* iid);

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

long long systemCall(void* self, void* base, int m, va_list ap)
{
    esReport("%s(%d, %d, %p)\n", __func__, (void**) self - (void**) base, m, ap);

    long long result;

    __asm__ __volatile__ (
        "int    $65"
        : "=A"(result) : "a"(self), "d"(m), "c"(ap), "S"(base));
    return result;
}

//
// XXX The following table must be mapped into user process address space
//     The kernel needs to know the address of Ptbl.
//
Broker<systemCall, 100> System;

int main()
{
    IInterface* system(0);
    esInit(&system);

    Handle<IContext> nameSpace(system);
    Handle<IPageable> framebuffer(nameSpace->lookup("device/framebuffer"));
    TEST(framebuffer);
    long long size;
    size = framebuffer->getSize();

    Reflect::Interface* interface = getInterface(&IInterface::interfaceID());
    esReport("%p, %s\n", interface, interface->getName());

    Process* process = new Process;
    process->load();

    IInterface* object = (IInterface*) &(System.getInterfaceTable()[0]);
    esReport("object: %p\n", object);
    int rc;
    rc = object->addRef();
    esReport("addRef: %x\n", rc);
    IContext* root;
    rc = object->queryInterface(IInterface::interfaceID(), (void**) &root);
    esReport("queryInterface: %x\n", rc);

    rc = root->release();
    esReport("release: %x\n", rc);

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
