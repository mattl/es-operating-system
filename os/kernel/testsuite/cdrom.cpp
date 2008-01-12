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

u8 buf[2048];

void testDisk(Handle<IStream> disk, long count)
{
    long rc = disk->read(buf, count, 0x8000);
    esReport("disk->read: %d\n", rc);
    TEST(count == rc);
    esDump(buf, rc);
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

    Handle<IStream> disk(root->lookup("device/ata/channel1/device0"));
    TEST(disk);

    IDiskManagement::Geometry geo;
    Handle<IDiskManagement> dm(disk);
    TEST(dm);
    dm->getGeometry(&geo);
    esReport("%d %d %d %d %lld\n",
             geo.heads,
             geo.cylinders,
             geo.sectorsPerTrack,
             geo.bytesPerSector,
             geo.diskSize);

    if (0 < geo.bytesPerSector)
    {
        testDisk(disk, geo.bytesPerSector);
    }

    esPanic(__FILE__, __LINE__, "done.\n");
}
