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
    // remove a directory which contains a file.
    Handle<es::File> dir = root->lookup("data");
    TEST(dir);
    dir = 0;

    long ret = root->destroySubcontext("data");
    TEST(ret < 0);

    dir = root->lookup("data");
    TEST(dir);
    dir = 0;

    // remove a file.
    Handle<es::File> file = root->lookup("NOTICE");
    TEST(file);
    file = 0;

    ret = root->unbind("NOTICE");
    TEST(ret < 0);

    file = root->lookup("NOTICE");
    TEST(file);
    file = 0;

    // remove an empty directory.
    Handle<es::Context> dirs = root->lookup("dirs");
    TEST(dirs);

    Handle<es::File> emptyDir = dirs->lookup("dir01");
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
