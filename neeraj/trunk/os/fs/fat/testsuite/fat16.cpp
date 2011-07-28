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
#include "fatStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void test(Handle<es::Context>    root)
{
    Handle<es::Iterator>   iter;
    Handle<es::File>       file;
    Handle<es::Stream>     stream;
    Handle<es::Context>    ctx;
    long long           size = 0;
    long long           n;

    file = root;
    size = file->getSize();
    esReport("root size: %lld\n", size);

    // Create
    file = root->bind("bar.TXT", 0);
    TEST(file);
    file = root->bind("TAR.txt", 0);
    TEST(file);
    file = root->bind("Foo.txt", 0);
    TEST(file);
    size = file->getSize();
    TEST(size == 0);

    // Create sub-directory
    root->createSubcontext("Subcontext");
    root->rename("Subcontext", "Subfolder");

    // List the root directory
    n = 0;
    iter = root->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
        esReport("'%s'\n", name);
        ++n;
    }
    TEST(n == 4);

    // Read
    stream = file->getStream();
    u8 buf[20];
    n = stream->read(buf, sizeof buf);
    TEST(n == 0);

    // Write
    stream->write("0123456789\n", 11);
    size = stream->getSize();
    TEST(size == 11);
    stream->setPosition(0);
    memset(buf, 0, sizeof buf);
    stream->read(buf, sizeof buf);
    TEST(memcmp(buf, "0123456789\n", 11) == 0);
    stream->flush();

    // Remove
    n = 0;
    root->unbind("TAR.txt");
    root->destroySubcontext("Subfolder");
    iter = root->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
        esReport("'%s'\n", name);
        ++n;
    }
    TEST(n == 2);

    // Not found
    file = root->lookup("Unknown.txt");
    TEST(!file);

    // Lookup then write
    file = root->lookup("bar.TXT");
    stream = file->getStream();
    stream->write("Hello, world.\n", 14);
    size = stream->getSize();
    TEST(size == 14);
    stream->setPosition(0);
    memset(buf, 0, sizeof buf);
    stream->read(buf, sizeof buf);
    TEST(memcmp(buf, "Hello, world.\n", 14) == 0);
    stream->flush();
    stream = 0;
    file = 0;
}

int main()
{
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/ata/channel0/device0");
#else
    Handle<es::Stream> disk = new VDisk(static_cast<char*>("fat16_5MB.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<es::FileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = es::FatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<es::Context> root;

        root = fatFileSystem->getRoot();
        test(root);
        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    fatFileSystem = es::FatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    esReport("\nChecking the file system...\n");
    TEST(fatFileSystem->checkDisk(false));
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esReport("done.\n\n");
}
