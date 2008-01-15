/*
 * Copyright 2008 Google Inc.
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
#include <es/base/IInterface.h>
#include "ataController.h"
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

u8 buf[2048];

void testDisk(Handle<IStream> disk, long count)
{
    esReport("disk->read: ");
    long rc = disk->read(buf, count, 0x8000);
    esReport("%d\n", rc);
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
    TEST(disk.get());

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
