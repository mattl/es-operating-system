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
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void test(Handle<IContext> root)
{
    Handle<IIterator>   iter;
    Handle<IFile>       file;
    Handle<IStream>     stream;
    long long           size = 0;

    // List the root directory
    iter = root->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<IBinding> binding(iter->next());
        binding->getName(name, sizeof name);
#ifdef VERBOSE
        esReport("'%s'\n", name);
#endif // VERBOSE
    }

    // Read
    file = root->lookup("NOTICE");
    TEST(file);
    size = file->getSize();
#ifdef VERBOSE
    esReport("Size: %lld\n", size);
#endif // VERBOSE
    u8* buf = new u8[size];
    stream = file->getStream();
    TEST(stream);
    stream->read(buf, size);
#ifdef VERBOSE
    esReport("%.*s\n", (int) size, buf);
#endif // VERBOSE
    delete [] buf;
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterIsoFileSystemClass(classStore);

#ifdef __es__
    Handle<IStream> disk = nameSpace->lookup("device/ata/channel1/device0");
#else
    IStream* disk = new VDisk(static_cast<char*>("isotest.iso"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);


    Handle<IFileSystem> isoFileSystem;
    esCreateInstance(CLSID_IsoFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&isoFileSystem));
    TEST(isoFileSystem);
    isoFileSystem->mount(disk);
    {
        Handle<IContext> root;

        isoFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        TEST(root);

        Handle<IBinding> binding = root;
        TEST(binding);

        Handle<IInterface> interface = binding->getObject();
        TEST(interface);

        Handle<IContext> object = interface;
        TEST(object);
        TEST(object == root);

        // setObject() returns an error.
        long rc = binding->setObject(interface);
        TEST(rc < 0);
    }
    isoFileSystem->dismount();

    esReport("done.\n");
}
