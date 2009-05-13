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

u8 buf[4096] __attribute__ ((aligned (4096)));

void testDisk(Handle<es::Stream> disk)
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

void PrintPartitions(es::Context* context)
{
    char name[16];
    es::DiskManagement::Partition params;
    esReport("boot type offset   size\n");
    Handle<es::Iterator> iter = context->list("");
    Handle<es::Binding> binding;
    while ((binding = iter->next()))
    {
        Handle<es::DiskManagement> diskManagement = binding->getObject();
        TEST(diskManagement);
        diskManagement->getLayout(&params);
        TEST(0 < binding->getName(name, sizeof(name)));
        esReport("%02x   %02x   %08llx %08llx %s\n",
                 params.bootIndicator,
                 params.partitionType,
                 params.startingOffset,
                 params.partitionLength,
                 name);

        Handle<es::Stream> stream = context->lookup(name);
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

    Handle<es::Stream> disk(root->lookup("device/ata/channel0/device0"));
    testDisk(disk);

    Handle<es::Context> partiton(root->lookup("device/ata/channel0/device0"));
    if (partiton)
    {
        esReport("partiton test\n");
        PrintPartitions(partiton);
    }

    esPanic(__FILE__, __LINE__, "done.\n");
}
