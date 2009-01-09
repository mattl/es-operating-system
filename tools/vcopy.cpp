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
#include <es/exception.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/base/IFile.h>
#include <es/device/IFatFileSystem.h>
#include <es/naming/IContext.h>
#include <es/handle.h>
#include "vdisk.h"
#include "fatStream.h"

u8 sec[512];

static IContext* checkDestinationPath(char* dst, Handle<IContext> root)
{
    Handle<IContext> currentDir = root;
    char buf[1024];
    ASSERT(strlen(dst)+1 <= sizeof(buf));
    memmove(buf, dst, strlen(dst)+1);

    char* path = buf;
    char* end = &buf[strlen(dst)];
    char* next;
    while (path < end)
    {
        next = strchr(path, '/');
        if (next)
        {
            *next = 0;
        }
        else
        {
            Handle<IFile> last = currentDir->lookup(path);
            if (!last || last->isFile())
            {
                return 0; // This indicates destination filename is specified.
            }
            currentDir = last;
            break;
        }

        Handle<IInterface> interface = currentDir->lookup(path);
        if (interface)
        {
            Handle<IFile> file = interface;
            if (file->isFile())
            {
                esReport("Error: invalid path. %s is a file.\n", path);
                esThrow(EINVAL);
                // NOT REACHED HERE
            }
            currentDir = interface;
        }
        else
        {
            Handle<IContext> dir = interface;
            currentDir = currentDir->createSubcontext(path);
            ASSERT(currentDir);
        }
        path = next + 1;
    }

    return currentDir; // This indicates destination directory is specified.
}

static const char* getFilename(const char* p)
{
    const char* head = p;
    p += strlen(p);
    while (head < --p)
    {
        if (strchr(":/\\", *p))
        {
            ++p;
            break;
        }
    }
    return p;
}

void copy(Handle<IContext> root, char* filename, char* dst)
{
    FILE* f = fopen(filename, "rb");
    if (f == 0)
    {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(EXIT_FAILURE);
    }

    const char* p;
    Handle<IContext> currentDir = checkDestinationPath(dst, root);
    if (currentDir)
    {
        p = getFilename(filename);
    }
    else
    {
        p = dst;
        currentDir = root;
    }

    Handle<IFile> file(currentDir->bind(p, 0));
    if (!file)
    {
        file = currentDir->lookup(p);
    }
    Handle<IStream> stream(file->getStream());
    stream->setSize(0);
    for (;;)
    {
        long n = fread(sec, 1, 512, f);
        if (n < 0)
        {
            exit(EXIT_FAILURE);
        }
        if (n == 0)
        {
            return;
        }
        do {
            long len = stream->write(sec, n);
            if (len <= 0)
            {
                exit(EXIT_FAILURE);
            }
            n -= len;
        } while (0 < n);
    }
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<IContext> nameSpace(ns);

    if (argc < 3)
    {
        esReport("usage: %s disk_image file [destination_path]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Handle<IStream> disk = new VDisk(static_cast<char*>(argv[1]));
    Handle<IFileSystem> fatFileSystem;
    fatFileSystem = IFatFileSystem::createInstance();
    fatFileSystem->mount(disk);

    long long freeSpace;
    long long totalSpace;
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);

    {
        Handle<IContext> root;
        root = fatFileSystem->getRoot();
        char* dst = (char*) (3 < argc ? argv[3] : "");
        copy(root, argv[2], dst);
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;
}
