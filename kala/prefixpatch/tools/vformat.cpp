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
#include <stdlib.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/device/IFatFileSystem.h>
#include <es/naming/IContext.h>
#include <es/handle.h>
#include "vdisk.h"
#include "fatStream.h"

int main(int argc, char* argv[])
{
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

    if (argc < 2)
    {
        esReport("usage: %s disk_image [cylinders] [heads] [sectorsPerTrack]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Handle<es::Stream> disk;
    if (argc < 5)
    {
        disk = new VDisk(static_cast<char*>(argv[1]));
    }
    else
    {
        unsigned int cylinders = atoi(argv[2]);
        unsigned int heads = atoi(argv[3]);
        unsigned int sectorsPerTrack = atoi(argv[4]);
        disk = new VDisk(static_cast<char*>(argv[1]), cylinders, heads, sectorsPerTrack);
    }
    Handle<es::FileSystem> fatFileSystem;
    fatFileSystem = es::FatFileSystem::createInstance();
    try
    {
        fatFileSystem->mount(disk);
    }
    catch (SystemException<EINVAL>& e)
    {
    }
    fatFileSystem->format();
    fatFileSystem->dismount();
    fatFileSystem = 0;
}
