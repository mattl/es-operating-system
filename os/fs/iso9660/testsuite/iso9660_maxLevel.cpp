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

void test(Handle<IContext> level1)
{
    // On Joliet compliant media, however,
    // the number of levels in the hierarchy may exceed eight.
    //
    // Joliet compliant media shall comply with the remainder of
    // ISO 9660 section 6.8.2.1, so that for each file recorded,
    // the sum of the following shall not exceed 240:
    //   the length of the file identifier;
    //   the length of the directory identifiers of all relevant directories;
    //   the number of relevant directories.
    // [Joliet Specification]

    // test to read the file "level8.txt"
    // in the directory at level 7 as shown below.
    // root directory/level2/level3/level4/level5/level6/level7/level8.txt

    long pathLen = 0;

    TEST(level1);
    Handle<IContext>    level2 = level1->lookup("level2");
    TEST(level2);
    Handle<IContext>    level3 = level2->lookup("level3");
    TEST(level3);
    Handle<IContext>    level4 = level3->lookup("level4");
    TEST(level4);
    Handle<IContext>    level5 = level4->lookup("level5");
    TEST(level5);
    Handle<IContext>    level6 = level5->lookup("level6");
    TEST(level6);
    Handle<IContext>    level7 = level6->lookup("level7");
    TEST(level7);

    pathLen += strlen("level2") + 1;
    pathLen += strlen("level3") + 1;
    pathLen += strlen("level4") + 1;
    pathLen += strlen("level5") + 1;
    pathLen += strlen("level6") + 1;
    pathLen += strlen("level7") + 1;
    pathLen += strlen("level8.txt");
    TEST(pathLen <= 240);

    Handle<IFile>       fileLevel8 = level7->lookup("level8.txt");
    TEST(fileLevel8);

    char buf[32];

    Handle<IStream>  stream = fileLevel8->getStream();
    TEST(stream);

    long ret = stream->read(buf, sizeof(buf));
    TEST(ret == 4 && strcmp(buf, "test") == 0);

    level2 = 0;
    level3 = 0;
    level4 = 0;
    level5 = 0;
    level6 = 0;
    level7 = 0;
    fileLevel8 = 0;
    stream = 0;
    pathLen = 0;

    // check the directory "level8" in the directory at level 7
    // cannot be accessed.
    // root directory/level2x/level3/level4/level5/level6/level7/level8/
    level2 = level1->lookup("level2x");
    TEST(level2);
    level3 = level2->lookup("level3");
    TEST(level3);
    level4 = level3->lookup("level4");
    TEST(level4);
    level5 = level4->lookup("level5");
    TEST(level5);
    level6 = level5->lookup("level6");
    TEST(level6);
    level7 = level6->lookup("level7");
    TEST(level7);

    pathLen += strlen("level2x") + 1;
    pathLen += strlen("level3") + 1;
    pathLen += strlen("level4") + 1;
    pathLen += strlen("level5") + 1;
    pathLen += strlen("level6") + 1;
    pathLen += strlen("level7") + 1;
    pathLen += strlen("level8") + 1;

    Handle<IContext> level8 = level7->lookup("level8");
    TEST(pathLen <= 240);
    TEST(level8);
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
    TEST(disk);
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
