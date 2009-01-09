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
#include "fatStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static int AttrList[] =
{
    IFile::ReadOnly,
    IFile::Hidden,
    IFile::System,
    IFile::Directory,
    IFile::Archive
};

static char AttrName[7][8] =
{
    "R", // "ReadOnly",
    "H", // "Hidden",
    "S", // "System",
    "D", // "Directory",
    "A", // "Archive",
    "unknown",
};

enum
{
    ERROR = -1,
    READ_ERROR = -2,
    WRITE_ERROR = -3
};

static void PrintAttribute(unsigned int attr)
{
    esReport("[");
    int i;
    for (i = 0; i < (int) (sizeof(AttrList)/sizeof(AttrList[0])); ++i)
    {
        esReport("%s", attr & AttrList[i] ? AttrName[i] : "_");
    }
    esReport("]");
}

static void SetData(u8* buf, long long size)
{
    buf[size-1] = 0;
    while (--size)
    {
        *buf++ = 'a' + size % 26;
    }
}

static long TestReadWrite(IStream* stream)
{
    u8* writeBuf;
    u8* readBuf;
    long long size = 1024LL;
    long ret = 0;

    writeBuf = new u8[size];
    readBuf = new u8[size];
    memset(readBuf, 0, size);

    SetData(writeBuf, size);

    try
    {
        ret = stream->write(writeBuf, size);
        TEST(ret == size);
    }
    catch (SystemException<EACCES>)
    {
        delete [] writeBuf;
        delete [] readBuf;
        return WRITE_ERROR;
    }

    stream->setPosition(0);
    ret = stream->read(readBuf, size);
    TEST(ret == size);

    TEST (memcmp(writeBuf, readBuf, size) == 0);

    delete [] writeBuf;
    delete [] readBuf;
    return ret;
}

static int CheckFileAttributes(IFile* file, unsigned int newAttr)
{
    PrintAttribute(newAttr);
    int ret = 0;
    try
    {
        file->setAttributes(newAttr);
    }
    catch (Exception& error)
    {
        ret = -1;
    }

    if (newAttr & IFile::Directory)
    {
#if 0
        if (ret == 0)
        {
            esReport("A file must not have a directory attribute.\n");
            return -1;
        }
#endif
        newAttr &= ~IFile::Directory;
    }
    if (ret < 0)
    {
        return -1;
    }

    if (newAttr & IFile::ReadOnly)
    {
        TEST(!file->canWrite());
    }

    if (newAttr & IFile::Hidden)
    {
        TEST(file->isHidden());
    }

    TEST(file->canRead());
    TEST(!file->isDirectory());
    TEST(file->isFile());

    unsigned int attr;
    try
    {
        attr = file->getAttributes();
    }
    catch (Exception& error)
    {
        return -1;
    }
    TEST(attr == newAttr);

    Handle<IStream> stream = file->getStream();

    ret = TestReadWrite(stream);
    if (attr & IFile::ReadOnly)
    {
        if (ret != WRITE_ERROR)
        {
            esReport(" ERROR\n");
            esReport("Although the file is read-only, write command succeeded.\n");
            return -1;
        }
    }
    else if (ret < 0)
    {
        return -1;
    }

    esReport(" OK\n");

    return 0;
}

static int CheckDirectoryAttributes(Handle<IFile> dir, unsigned int newAttr)
{
    PrintAttribute(newAttr);
    int ret = 0;
    try
    {
        dir->setAttributes(newAttr);
    }
    catch (Exception& error)
    {
        ret = -1;
    }

    if (!(newAttr & IFile::Directory))
    {
#if 0
        if (ret == 0)
        {
            esReport("A directory must have a directory attribute.\n");
            return -1;
        }
#endif
        newAttr |= IFile::Directory;
    }
    if (ret < 0)
    {
        return -1;
    }

    unsigned int attr;
    try
    {
        attr = dir->getAttributes();
    }
    catch (Exception& error)
    {
        return -1;
    }
    TEST(attr == newAttr);

    if (newAttr & IFile::ReadOnly)
    {
        TEST(!dir->canWrite());
        Handle<IContext> testDir = dir;
        try
        {
            Handle<IBinding> file = testDir->bind("test.txt", 0);
            TEST(!file);
        }
        catch (SystemException<EACCES>)
        {
        }
    }

    if (newAttr & IFile::Hidden)
    {
        TEST(dir->isHidden());
    }

    TEST(dir->canRead());
    TEST(dir->isDirectory());
    TEST(!dir->isFile());

    esReport(" OK\n");

    return 0;
}

static unsigned int GetNextAttribute(unsigned int attr)
{
    unsigned int all = (IFile::ReadOnly | IFile::Hidden |
                         IFile::System | IFile::Directory |
                         IFile::Archive);

    ++attr;
    while (attr <= all)
    {
        if (attr & all && !(attr & ~all))
        {
            return attr;
        }
        ++attr;
    }

    return 0;
}

static long TestFileSystem(Handle<IContext> root)
{
    unsigned int newAttr;

    Handle<IFile>       file;

    const char* filename = "test";

    file = root->bind(filename, 0);

    newAttr = 0;

    TEST(CheckFileAttributes(file, newAttr) == 0);
    while ((newAttr = GetNextAttribute(newAttr)))
    {
        TEST(CheckFileAttributes(file, newAttr) == 0);
    }

    Handle<IFile>    dir;
    dir = root->createSubcontext("testDir");

    newAttr = 0;
    TEST(CheckDirectoryAttributes(dir, newAttr) == 0);
    while ((newAttr = GetNextAttribute(newAttr)))
    {
        TEST(CheckDirectoryAttributes(dir, newAttr) == 0);
    }
    return 0;
}

int main(void)
{
    IInterface* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<IContext> nameSpace(ns);

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

    fatFileSystem = IFatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<IContext> root;

        root = fatFileSystem->getRoot();
        long ret = TestFileSystem(root);
        TEST (ret == 0);
        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
    }

    fatFileSystem->dismount();
    fatFileSystem = 0;

    fatFileSystem = IFatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    esReport("\nChecking the file system...\n");
    TEST(fatFileSystem->checkDisk(false));
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esReport("done.\n\n");
    return 0;
}
