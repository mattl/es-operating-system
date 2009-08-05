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

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "8237a.h"
#include "core.h"
#include "fdc.h"

u8 sec[512];

int main()
{
    long ret;

    Object* nameSpace;
    esInit(&nameSpace);

    Handle<es::Context> root(nameSpace);
    Handle<es::Stream> floppy = root->lookup("device/floppy");

    memset(sec, 0, 512);

    floppy->setPosition(0x2600);
    ret = floppy->read(sec, 512);
    esReport("read: %d\n", ret);

    esDump(sec, 512);

    Handle<es::Disk> dm(floppy);
    esReport("%d %d %d %d %lld\n",
             dm->getHeads(),
             dm->getCylinders(),
             dm->getSectorsPerTrack(),
             dm->getBytesPerSector(),
             dm->getDiskSize());

    esReport("---------------------------------------------------------\n"
             "This program reads floppy every 1 sec, and shows results.\n"
             "Try inserting and removing a floppy disk and check if \n"
             "the disk change is recognized.\n"
             "---------------------------------------------------------\n");
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
