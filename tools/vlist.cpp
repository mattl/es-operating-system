/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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
#include <stdio.h>
#include <stdlib.h>
#include <es.h>
#include <es/clsid.h>
#include <es/dateTime.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/base/IClassStore.h>
#include <es/base/IStream.h>
#include <es/base/IFile.h>
#include <es/device/IFileSystem.h>
#include <es/naming/IContext.h>
#include <es/handle.h>
#include "vdisk.h"

void list(Handle<IContext> root, const char* filename)
{
    filename = filename ? filename : "";
    Handle<IIterator> iter = root->list(filename);
    while (iter->hasNext())
    {
        char name[1024];
        Handle<IBinding> binding(iter->next());
        binding->getName(name, sizeof name);
        Handle<IFile> file(binding->getObject());
        long long size;
        size = file->getSize();
        long long t;
        t = file->getLastWriteTime();
        DateTime d(t);
        esReport("%8lld %2d/%2d/%2d %02d:%02d:%02d %s\t\n",
                 size,
                 d.getYear(),
                 d.getMonth(),
                 d.getDay(),
                 d.getHour(),
                 d.getMinute(),
                 d.getSecond(),
                 name);
    }
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

    if (argc < 2)
    {
        esReport("usage: %s disk_image [directory]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Handle<IStream> disk = new VDisk(static_cast<char*>(argv[1]));
    Handle<IFileSystem> fatFileSystem;
    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);
    fatFileSystem->checkDisk(false);
    esReport("\n");

    {
        Handle<IContext> root;

        root = fatFileSystem->getRoot();
        list(root, argv[2]);
    }

    long long freeSpace;
    long long totalSpace;
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("\nFree space %lld, Total space %lld\n", freeSpace, totalSpace);

    fatFileSystem->dismount();
    fatFileSystem = 0;
}
