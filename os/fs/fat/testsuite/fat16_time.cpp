/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
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

static long TestFileSystem(Handle<IContext> root)
{
    long long creationTime;
    long long lastAccessTime;
    long long lastWriteTime;

    Handle<IFile>       file;

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
    file->getCreationTime(creationTime);

    TEST(start.getTicks() <= creationTime &&
         creationTime <= end.getTicks());

    // read and write data to the file.
    Handle<IStream> stream = file->getStream();

    d = DateTime::getNow();
    TestReadWrite(stream);
    end = DateTime::getNow();

    // check the last access time and the last write time.
    DateTime day = DateTime(d.getYear(), d.getMonth(), d.getDay(), 0, 0, 0);
    start = DateTime(d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(),
        d.getSecond() - d.getSecond() % 2);

    file->getLastAccessTime(lastAccessTime);
    file->getLastWriteTime(lastWriteTime);

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
    file->getLastWriteTime(lastWriteTime2);
    TEST(lastWriteTime == lastWriteTime2);

    long long creationTime2;
    file->getCreationTime(creationTime2);
    TEST(creationTime == creationTime2);

    d = DateTime::getNow();
    DateTime now = DateTime(d.getYear(), d.getMonth(), d.getDay(),
        d.getHour(), d.getMinute(), d.getSecond(), d.getMillisecond() - d.getMillisecond() % 10);

    // check setCreationTime, setLastAccessTime and setLastWriteTime.
    file->setCreationTime(now.getTicks());
    file->getCreationTime(creationTime);
    TEST(now.getTicks() == creationTime);

    now = DateTime(d.getYear(), d.getMonth(), d.getDay(), 0, 0, 0, 0);
    file->setLastAccessTime(now.getTicks());
    file->getLastAccessTime(lastAccessTime);
    TEST(now.getTicks() == lastAccessTime);

    now = DateTime(d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(),
        d.getSecond() - d.getSecond() % 2);
    file->setLastWriteTime(now.getTicks());
    file->getLastWriteTime(lastWriteTime);

    TEST(now.getTicks() == lastWriteTime);

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
    Handle<IStream> disk = nameSpace->lookup("device/ata/channel0/device0");
#else
    Handle<IStream> disk = new VDisk(static_cast<char*>("fat16_5MB.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    fatFileSystem->getFreeSpace(freeSpace);
    fatFileSystem->getTotalSpace(totalSpace);
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<IContext> root;

        fatFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        TestFileSystem(root);
        fatFileSystem->getFreeSpace(freeSpace);
        fatFileSystem->getTotalSpace(totalSpace);
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);
    fatFileSystem->getFreeSpace(freeSpace);
    fatFileSystem->getTotalSpace(totalSpace);
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    esReport("\nChecking the file system...\n");
    TEST(fatFileSystem->checkDisk(false));
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esReport("done.\n\n");
}
