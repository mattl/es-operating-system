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
#include <es/handle.h>
#include <es/dateTime.h>
#include <es/object.h>
#include "ataController.h"
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

u8 buf[2048];

void testDisk(Handle<es::Stream> disk, long count)
{
    esReport("disk->read: ");
    long rc = disk->read(buf, count, 0x8000);
    esReport("%d\n", rc);
    TEST(count == rc);
    esDump(buf, rc);
}

int main()
{
    Object* nameSpace;
    esInit(&nameSpace);

    Handle<es::Context> root(nameSpace);
    Handle<es::Iterator> iterator = root->list("device/ata");
    while (iterator->hasNext())
    {
        char name[16];

        Handle<es::Binding> binding = iterator->next();
        binding->getName(name, sizeof name);
        esReport("%s\n", name);

        Handle<es::Context> channel = binding->getObject();
        Handle<es::Iterator> iterator = channel->list("");
        while (iterator->hasNext())
        {
            Handle<es::Binding> binding = iterator->next();
            binding->getName(name, sizeof name);
            esReport("    %s\n", name);
        }
    }

    Handle<es::Stream> disk(root->lookup("device/ata/channel1/device0"));
    TEST(disk.get());

    Handle<es::Disk> dm(disk);
    TEST(dm);
    esReport("%d %d %d %d %lld\n",
             dm->getHeads(),
             dm->getCylinders(),
             dm->getSectorsPerTrack(),
             dm->getBytesPerSector(),
             dm->getDiskSize());

    if (0 < dm->getBytesPerSector())
    {
        testDisk(disk, dm->getBytesPerSector());
    }

    esPanic(__FILE__, __LINE__, "done.\n");
}
