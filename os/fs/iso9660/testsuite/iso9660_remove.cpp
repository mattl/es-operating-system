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
