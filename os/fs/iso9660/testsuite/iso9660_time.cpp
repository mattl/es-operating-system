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
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static long IsTimeModified(IFile* file, long long& creationTime, long long& lastAccessTime,
    long long& lastWriteTime)
{
    long long creationTime2;
    long long lastAccessTime2;
    long long lastWriteTime2;

    file->getCreationTime(creationTime2);
    file->getLastAccessTime(lastAccessTime2);
    file->getLastWriteTime(lastWriteTime2);

    DateTime dc(creationTime2);
    DateTime da(lastAccessTime2);
    DateTime dw(lastWriteTime2);

#ifdef VERBOSE
    esReport("creation time    %d/%d/%d %d:%d:%d.%d\n",
        dc.getYear(), dc.getMonth(),
             dc.getDay(), dc.getHour(), dc.getMinute(),
                 dc.getSecond(), dc.getMillisecond());

    esReport("last access time %d/%d/%d %d:%d:%d.%d\n",
        da.getYear(), da.getMonth(),
             da.getDay(), da.getHour(), da.getMinute(),
                 da.getSecond(), da.getMillisecond());

    esReport("last write time  %d/%d/%d %d:%d:%d.%d\n",
        dw.getYear(), dw.getMonth(),
             dw.getDay(), dw.getHour(), dw.getMinute(),
                 dw.getSecond(), dw.getMillisecond());
#endif // VERBOSE

    if (creationTime != creationTime2 ||
        lastAccessTime != lastAccessTime2 ||
        lastWriteTime != lastWriteTime2)
    {
        return true;
    }
    return false;
}

void test(Handle<IContext> root)
{
    long long creationTime;
    long long lastAccessTime;
    long long lastWriteTime;
    long ret;

    Handle<IFile>       file = root->lookup("NOTICE");

    // check creation, last access and last write times.
    ret = file->getCreationTime(creationTime);
    TEST(ret == 0);
    ret = file->getLastAccessTime(lastAccessTime);
    TEST(ret == 0);
    ret = file->getLastWriteTime(lastWriteTime);
    TEST(ret == 0);

    TEST(creationTime <= lastAccessTime);
    TEST(creationTime <= lastWriteTime);

    DateTime now = DateTime::getNow();
    TEST(creationTime < now.getTicks());
    TEST(lastAccessTime < now.getTicks());
    TEST(lastWriteTime < now.getTicks());

#ifdef VERBOSE
    DateTime* dc = new DateTime(creationTime);
    DateTime* da = new DateTime(lastAccessTime);
    DateTime* dw = new DateTime(lastWriteTime);

    esReport("creation time    %d/%d/%d %d:%d:%d.%d\n",
        dc->getYear(), dc->getMonth(),
             dc->getDay(), dc->getHour(), dc->getMinute(),
                 dc->getSecond(), dc->getMillisecond());

    esReport("last access time %d/%d/%d %d:%d:%d.%d\n",
        da->getYear(), da->getMonth(),
             da->getDay(), da->getHour(), da->getMinute(),
                 da->getSecond(), da->getMillisecond());

    esReport("last write time  %d/%d/%d %d:%d:%d.%d\n",
        dw->getYear(), dw->getMonth(),
             dw->getDay(), dw->getHour(), dw->getMinute(),
                 dw->getSecond(), dw->getMillisecond());

    delete dc;
    delete da;
    delete dw;
#endif // VERBOSE

    u8 buf[1024];
    Handle<IStream> stream = file->getStream();
    stream->read(buf, sizeof(buf));

    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));

    try
    {
        stream->write(buf, sizeof(buf));
    }
    catch (...)
    {

    }
    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));

    // confirm methods to set times return errors.
    ret = file->setCreationTime(now.getTicks());
    TEST(ret < 0);
    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));

    ret = file->setLastAccessTime(now.getTicks());
    TEST(ret < 0);
    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));

    ret = file->setLastWriteTime(now.getTicks());
    TEST(ret < 0);
    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));
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
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> isoFileSystem;
    esCreateInstance(CLSID_IsoFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&isoFileSystem));
    TEST(isoFileSystem);
    isoFileSystem->mount(disk);
    {
        Handle<IContext> root;

        isoFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        TEST(root);
        test(root);
    }
    isoFileSystem->dismount();

    esReport("done.\n");
}
