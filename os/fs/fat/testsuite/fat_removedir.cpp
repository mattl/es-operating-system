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

#include <new>
#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include "vdisk.h"
#include "fatStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static long TestFileSystem(Handle<IContext> root)
{
    long ret;
    const char* dirList[] =
    {
        "dir1",
        "dir2",
        "dir3"
    };

    const char* fileList[] =
    {
        "test.txt",
    };

    Handle<IContext> dir;

    // create
    int i;
    for (i = 0; i < sizeof(dirList)/sizeof(dirList[0]); ++i)
    {
        dir = root->createSubcontext(dirList[i]);
    }

    for (i = 0; i < sizeof(dirList)/sizeof(dirList[0]); ++i)
    {
        ret = root->destroySubcontext(dirList[i]);
    }

    long long n = 0;
    Handle<IIterator>   iter;
    iter = root->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<IBinding> binding(iter->next());
        binding->getName(name, sizeof name);
        esReport("'%s'\n", name);
        ++n;
    }

    TEST(n==0);

    dir = root;
    for (i = 0; i < sizeof(dirList)/sizeof(dirList[0]); ++i)
    {
        dir = dir->createSubcontext(dirList[i]);
        TEST(dir);
    }

    Handle<IContext> dir0 = root->lookup(dirList[0]);
    TEST(dir0);
    Handle<IContext> dir1 = dir0->lookup(dirList[1]);
    TEST(dir1);
    Handle<IContext> dir2 = dir1->lookup(dirList[2]);
    TEST(dir2);

    ret = root->destroySubcontext(dirList[0]);
    TEST(ret < 0);
    ret = dir0->destroySubcontext(dirList[1]);
    TEST(ret < 0);

    ret = dir1->destroySubcontext(dirList[2]);
    TEST(ret == 0);
    ret = dir0->destroySubcontext(dirList[1]);
    TEST(ret == 0);
    ret = root->destroySubcontext(dirList[0]);
    TEST(ret == 0);

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

    TEST(n==0);

    return 0;
}

int main(void)
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
        TestFileSystem(root);
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
