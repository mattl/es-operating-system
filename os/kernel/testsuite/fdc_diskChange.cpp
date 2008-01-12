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

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "8237a.h"
#include "fdc.h"
#include "io.h"

u8 sec[512];

int main()
{
    long ret;

    IInterface* nameSpace;
    esInit(&nameSpace);

    Handle<IContext> root(nameSpace);
    Handle<IStream> floppy = root->lookup("device/floppy");

    memset(sec, 0, 512);

    floppy->setPosition(0x2600);
    ret = floppy->read(sec, 512);
    esReport("read: %d\n", ret);

    esDump(sec, 512);

    IDiskManagement::Geometry geo;
    Handle<IDiskManagement> dm(floppy);
    dm->getGeometry(&geo);
    esReport("%d %d %d %d %lld\n",
             geo.heads,
             geo.cylinders,
             geo.sectorsPerTrack,
             geo.bytesPerSector,
             geo.diskSize);

    // This program reads floppy every 1 sec, and shows results.
    // Try inserting and removing a floppy disk and
    // check if the disk change is recognized.
    int count = 20;
    while (count--)
    {
        floppy->setPosition(0x2600);
        try
        {
            ret = floppy->read(sec, 512);
            esReport("%d: read %d\n", count, ret);

            ret = floppy->write(sec, 512);
            esReport("%d: write %d\n", count, ret);
        }
        catch (...)
        {
            esReport("Exception\n");
        }
        esSleep(10000000);
    }

    esSleep(50000000);      // for motor stop

    // esReport("done.\n");    // for testing
}
