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

#include <new>
#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include "vdisk.h"
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void test(Handle<es::Context> root)
{
    Handle<es::Iterator>   iter;
    Handle<es::File>       file;
    Handle<es::Stream>     stream;
    long long           size = 0;

    // List the root directory
    iter = root->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
        esReport("'%s'\n", name);
    }

    // Read
    file = root->lookup("NOTICE");
    TEST(file);
    size = file->getSize();
    esReport("Size: %lld\n", size);
    u8* buf = new u8[size];
    stream = file->getStream();
    stream->read(buf, size);
    esReport("%.*s\n", (int) size, buf);
}

int main(int argc, char* argv[])
{
    Object* ns = 0;
    esInit(&ns);
    Iso9660FileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/ata/channel1/device0");
#else
    es::Stream* disk = new VDisk(static_cast<char*>("isotest.iso"));
#endif
    TEST(disk);
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);
    TEST(0 < diskSize);

    Handle<es::FileSystem> isoFileSystem;
    isoFileSystem = es::Iso9660FileSystem::createInstance();
    TEST(isoFileSystem);
    isoFileSystem->mount(disk);
    {
        Handle<es::Context> root;

        root = isoFileSystem->getRoot();
        TEST(root);
        test(root);
    }
    isoFileSystem->dismount();

    esReport("done.\n");
}
