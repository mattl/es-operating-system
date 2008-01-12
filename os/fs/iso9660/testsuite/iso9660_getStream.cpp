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
#include <errno.h>
#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include "vdisk.h"
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

struct CheckList
{
    char* name;
    int count;
};

static void SetData(u8* buf, long long size)
{
    buf[size-1] = 0;
    while (--size)
    {
        *buf++ = 'a' + size % 26;
    }
}

void test(Handle<IContext> root)
{
    Handle<IFile>    dir;
    dir = root->lookup("data");
    TEST(dir && dir->isDirectory());

    // check the stream of a directory is not available.
    try
    {
        Handle<IStream> dirStream = dir->getStream();
        TEST(!dirStream);
    }
    catch (SystemException<EPERM>& e)
    {

    }

    // check the stream of a file is available.
    Handle<IContext> data = dir;
    Handle<IFile> file = data->lookup("image");
    TEST(file && file->isFile());
    Handle<IStream> stream = file->getStream();
    TEST(stream);
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
    TEST(disk);
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);
    TEST(0 < diskSize);

    Handle<IFileSystem> isoFileSystem;
    isoFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_IsoFileSystem, IFileSystem::iid()));
    TEST(isoFileSystem);
    isoFileSystem->mount(disk);
    {
        Handle<IContext> root;

        root = isoFileSystem->getRoot();
        TEST(root);
        test(root);
    }
    isoFileSystem->dismount();

    esReport("done.\n");
}
