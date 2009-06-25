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
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static long IsTimeModified(es::File* file, long long& creationTime, long long& lastAccessTime,
    long long& lastWriteTime)
{
    long long creationTime2;
    long long lastAccessTime2;
    long long lastWriteTime2;

    creationTime2 = file->getCreationTime();
    lastAccessTime2 = file->getLastAccessTime();
    lastWriteTime2 = file->getLastWriteTime();

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

void test(Handle<es::Context> root)
{
    long long creationTime;
    long long lastAccessTime;
    long long lastWriteTime;
    long ret;

    Handle<es::File>       file = root->lookup("NOTICE");

    // check creation, last access and last write times.
    try
    {
        creationTime = file->getCreationTime();
        lastAccessTime = file->getLastAccessTime();
        lastWriteTime = file->getLastWriteTime();
    }
    catch (...)
    {
        TEST(false); // error.
    }

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
    Handle<es::Stream> stream = file->getStream();
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
    try
    {
        file->setCreationTime(now.getTicks());
        ret = 0;
    }
    catch (...)
    {
        ret = -1;
    }
    TEST(ret < 0);
    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));

    try
    {
        file->setLastAccessTime(now.getTicks());
        ret = 0;
    }
    catch (...)
    {
        ret = -1;
    }
    TEST(ret < 0);
    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));

    try
    {
        file->setLastWriteTime(now.getTicks());
        ret = 0;
    }
    catch (...)
    {
        ret = -1;
    }
    TEST(ret < 0);
    TEST(!IsTimeModified(file, creationTime, lastAccessTime, lastWriteTime));
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
