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
    Iso9660FileSystem::initializeConstructor();
    Handle<IContext> nameSpace(ns);

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
    isoFileSystem = IIso9660FileSystem::createInstance();
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
