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
