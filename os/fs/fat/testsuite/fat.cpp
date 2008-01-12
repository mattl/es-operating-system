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

#include <new>
#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include "vdisk.h"
#include "fatStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void test(Handle<IContext> root)
{
    Handle<IIterator>   iter;
    Handle<IFile>       file;
    Handle<IStream>     stream;
    Handle<IContext>    ctx;
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
        Handle<IBinding> binding(iter->next());
        binding->getName(name, sizeof name);
        esReport("'%s'\n", name);
        ++n;
    }
    TEST(n == 4);

    // Read
    stream = file->getStream();
    u8* buf = new u8[size];
    n = stream->read(buf, size);
    TEST(n == 0);

    // Write
    stream->write("0123456789\n", 11);
    size = stream->getSize();
    TEST(size == 11);
    u8* buf2 = new u8[size];
    stream->setPosition(0);
    stream->read(buf2, size);
    TEST(memcmp(buf2, "0123456789\n", 11) == 0);
    stream->flush();

    // Remove
    n = 0;
    root->unbind("TAR.txt");
    root->destroySubcontext("Subfolder");
    iter = root->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<IBinding> binding(iter->next());
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
    u8* buf3 = new u8[size];
    stream->setPosition(0);
    stream->read(buf3, size);
    TEST(memcmp(buf3, "Hello, world.\n", 14) == 0);
    stream->flush();
    stream = 0;
    file = 0;
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

#ifdef __es__
    Handle<IStream> disk = nameSpace->lookup("device/floppy");
#else
    Handle<IStream> disk = new VDisk(static_cast<char*>("2hd.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = reinterpret_cast<IFileSystem*>(
            esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<IContext> root;

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

    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
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
