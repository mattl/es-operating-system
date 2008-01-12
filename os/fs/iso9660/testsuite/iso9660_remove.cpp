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
    // remove a directory which contains a file.
    Handle<IFile> dir = root->lookup("data");
    TEST(dir);
    dir = 0;

    long ret = root->destroySubcontext("data");
    TEST(ret < 0);

    dir = root->lookup("data");
    TEST(dir);
    dir = 0;

    // remove a file.
    Handle<IFile> file = root->lookup("NOTICE");
    TEST(file);
    file = 0;

    ret = root->unbind("NOTICE");
    TEST(ret < 0);

    file = root->lookup("NOTICE");
    TEST(file);
    file = 0;

    // remove an empty directory.
    Handle<IContext> dirs = root->lookup("dirs");
    TEST(dirs);

    Handle<IFile> emptyDir = dirs->lookup("dir01");
    TEST(emptyDir);
    emptyDir = 0;

    ret = dirs->destroySubcontext("dir01");
    TEST(ret < 0);

    emptyDir = dirs->lookup("dir01");
    TEST(emptyDir);
    emptyDir = 0;
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
        test(root);
    }
    isoFileSystem->dismount();

    esReport("done.\n");
}
