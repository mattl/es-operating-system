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

#include <es.h>
#include <es/handle.h>
#include <es/dateTime.h>
#include <es/base/IInterface.h>
#include "ataController.h"
#include "uart.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

u8 buf[4096] __attribute__ ((aligned (4096)));

void testDisk(Handle<IStream> disk)
{
    long count;

    count = disk->read(buf, 512, 0);
    TEST(count == 512);
    esDump(buf, 512);
    count = disk->write(buf, 512, 512);
    TEST(count == 512);

    esReport("!\n");
    for (int i = 0; i < 1024*1024/512; ++i)
    {
        count = disk->read(buf, 512, 0);
    }
    esReport("!\n");
}

void PrintPartitions(IContext* context)
{
    char name[16];
    IDiskManagement::Partition params;
    esReport("boot type offset   size\n");
    Handle<IIterator> iter = context->list("");
    Handle<IBinding> binding;
    while ((binding = iter->next()))
    {
        Handle<IDiskManagement> diskManagement = binding->getObject();
        TEST(diskManagement);
        diskManagement->getLayout(&params);
        TEST(0 < binding->getName(name, sizeof(name)));
        esReport("%02x   %02x   %08llx %08llx %s\n",
                 params.bootIndicator,
                 params.partitionType,
                 params.startingOffset,
                 params.partitionLength,
                 name);

        Handle<IStream> stream = context->lookup(name);
        TEST(stream);
        long long size;
        size = stream->getSize();
        TEST(params.partitionLength == size);
        TEST(params.bootIndicator == 0x00 || params.bootIndicator == 0x80);
        TEST(params.partitionType != 0x00);
    }
    esReport("\n");
}

int main()
{
    IInterface* nameSpace;
    esInit(&nameSpace);

    Handle<IContext> root(nameSpace);
    Handle<IIterator> iterator = root->list("device/ata");
    while (iterator->hasNext())
    {
        char name[16];

        Handle<IBinding> binding = iterator->next();
        binding->getName(name, sizeof name);
        esReport("%s\n", name);

        Handle<IContext> channel = binding->getObject();
        Handle<IIterator> iterator = channel->list("");
        while (iterator->hasNext())
        {
            Handle<IBinding> binding = iterator->next();
            binding->getName(name, sizeof name);
            esReport("    %s\n", name);
        }
    }

    Handle<IStream> disk(root->lookup("device/ata/channel0/device0"));
    testDisk(disk);

    Handle<IContext> partiton(root->lookup("device/ata/channel0/device0"));
    if (partiton)
    {
        esReport("partiton test\n");
        PrintPartitions(partiton);
    }

    esPanic(__FILE__, __LINE__, "done.\n");
}
