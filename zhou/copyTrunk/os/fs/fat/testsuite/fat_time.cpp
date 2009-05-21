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
#include <es/exception.h>
#include <es/dateTime.h>
#include "vdisk.h"
#include "fatStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static void SetData(u8* buf, long long size)
{
    buf[size-1] = 0;
    while (--size)
    {
        *buf++ = 'a' + size % 26;
    }
}

static long TestReadWrite(es::Stream* stream)
{
    u8* writeBuf;
    u8* readBuf;
    long long size = 1024LL;
    long ret = 0;

    writeBuf = new u8[size];
    readBuf = new u8[size];
    memset(readBuf, 0, size);

    SetData(writeBuf, size);
    ret = stream->write(writeBuf, size);
    TEST(ret == size);

    stream->setPosition(0);
    ret = stream->read(readBuf, size);
    TEST(ret == size);
    TEST (memcmp(writeBuf, readBuf, size) == 0);

ERR:
    delete [] writeBuf;
    delete [] readBuf;
    return ret;
}

static long TestFileSystem(Handle<es::Context> root)
{
    long long creationTime;
    long long lastAccessTime;
    long long lastWriteTime;

    Handle<es::File>       file;

    const char* filename = "test.txt";

    // create file.
    DateTime d = DateTime::getNow();
    file = root->bind(filename, 0);
    DateTime end = DateTime::getNow();

    DateTime start = DateTime(d.getYear(), d.getMonth(), d.getDay(),
        d.getHour(), d.getMinute(), d.getSecond(), d.getMillisecond() - d.getMillisecond() % 10);

#ifdef VERBOSE
    DateTime dc(creationTime);
    esReport("creation %d/%d/%d %d:%d:%d.%d\n", dc.getYear(), dc.getMonth(),
                 dc.getDay(), dc.getHour(), dc.getMinute(),
                     dc.getSecond(), dc.getMillisecond());

    esReport("start    %d/%d/%d %d:%d:%d.%d\n", start.getYear(), start.getMonth(),
                 start.getDay(), start.getHour(), start.getMinute(),
                     start.getSecond(), start.getMillisecond());

    esReport("end      %d/%d/%d %d:%d:%d.%d\n", end.getYear(), end.getMonth(),
                 end.getDay(), end.getHour(), end.getMinute(),
                     end.getSecond(), end.getMillisecond());

#endif // VERBOSE

    // check creation time.
    creationTime = file->getCreationTime();

    TEST(start.getTicks() <= creationTime &&
         creationTime <= end.getTicks());

    // read and write data to the file.
    Handle<es::Stream> stream = file->getStream();

    d = DateTime::getNow();
    TestReadWrite(stream);
    end = DateTime::getNow();

    // check the last access time and the last write time.
    DateTime day = DateTime(d.getYear(), d.getMonth(), d.getDay(), 0, 0, 0);
    start = DateTime(d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(),
        d.getSecond() - d.getSecond() % 2);

    lastAccessTime = file->getLastAccessTime();
    lastWriteTime = file->getLastWriteTime();

    TEST(day.getTicks() <= lastAccessTime &&
         lastAccessTime <= end.getTicks());
    TEST(start.getTicks() <= lastWriteTime &&
         lastWriteTime <= end.getTicks());

#ifdef VERBOSE
    DateTime da(lastAccessTime);
    esReport("last access time %d/%d/%d %d:%d:%d.%d\n", da.getYear(), da.getMonth(),
                 da.getDay(), da.getHour(), da.getMinute(),
                     da.getSecond(), da.getMillisecond());
    DateTime dw(lastWriteTime);
    esReport("start            %d/%d/%d %d:%d:%d.%d\n", start.getYear(), start.getMonth(),
                 start.getDay(), start.getHour(), start.getMinute(),
                     start.getSecond(), start.getMillisecond());
    esReport("lastWriteTime    %d/%d/%d %d:%d:%d.%d\n", dw.getYear(), dw.getMonth(),
                 dw.getDay(), dw.getHour(), dw.getMinute(), dw.getSecond(), dw.getMillisecond());
    esReport("end              %d/%d/%d %d:%d:%d.%d\n", end.getYear(), end.getMonth(),
                 end.getDay(), end.getHour(), end.getMinute(),
                     end.getSecond(), end.getMillisecond());
#endif

    // read the file, and check the last write time.
    u8 buf[512];

    d = DateTime::getNow();
    stream->read(buf, sizeof(buf));
    end = DateTime::getNow();

    start = DateTime(d.getYear(), d.getMonth(), d.getDay(),
        d.getHour(), d.getMinute(), d.getSecond());

    long long lastWriteTime2;
    lastWriteTime2 = file->getLastWriteTime();
    TEST(lastWriteTime == lastWriteTime2);

    long long creationTime2;
    creationTime2 = file->getCreationTime();
    TEST(creationTime == creationTime2);

    d = DateTime::getNow();
    DateTime now = DateTime(d.getYear(), d.getMonth(), d.getDay(),
        d.getHour(), d.getMinute(), d.getSecond(), d.getMillisecond() - d.getMillisecond() % 10);

    // check setCreationTime, setLastAccessTime and setLastWriteTime.
    file->setCreationTime(now.getTicks());
    creationTime = file->getCreationTime();
    TEST(now.getTicks() == creationTime);

    now = DateTime(d.getYear(), d.getMonth(), d.getDay(), 0, 0, 0, 0);
    file->setLastAccessTime(now.getTicks());
    lastAccessTime = file->getLastAccessTime();
    TEST(now.getTicks() == lastAccessTime);

    now = DateTime(d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(),
        d.getSecond() - d.getSecond() % 2);
    file->setLastWriteTime(now.getTicks());
    lastWriteTime = file->getLastWriteTime();

    TEST(now.getTicks() == lastWriteTime);

    return 0;
}

int main(void)
{
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/floppy");
#else
    Handle<es::Stream> disk = new VDisk(static_cast<char*>("2hd.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<es::FileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = es::FatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<es::Context> root;

        root = fatFileSystem->getRoot();
        long ret = TestFileSystem(root);
        TEST(ret == 0);
        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    fatFileSystem = es::FatFileSystem::createInstance();
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
