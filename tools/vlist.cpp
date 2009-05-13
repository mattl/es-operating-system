/*
 * Copyright 2008, 2009 Google Inc.
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
#include <es/dateTime.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/base/IFile.h>
#include <es/device/IFatFileSystem.h>
#include <es/naming/IContext.h>
#include <es/handle.h>
#include "vdisk.h"
#include "fatStream.h"

void list(Handle<es::Context> root, const char* filename)
{
    filename = filename ? filename : "";
    Handle<es::Iterator> iter = root->list(filename);
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
        Handle<es::File> file(binding->getObject());
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
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

    if (argc < 2)
    {
        esReport("usage: %s disk_image [directory]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Handle<es::Stream> disk = new VDisk(static_cast<char*>(argv[1]));
    Handle<es::FileSystem> fatFileSystem;
    fatFileSystem = es::FatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    fatFileSystem->checkDisk(false);
    esReport("\n");

    {
        Handle<es::Context> root;

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
