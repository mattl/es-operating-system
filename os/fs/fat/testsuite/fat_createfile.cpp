/*
 * Copyright 2008 Google Inc.
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

static void TestFileSystem(Handle<IContext>  root)
{
    const char* nameList[] =
    {
        "ﾌｧｲﾙ",
        "ファイル",
        "a1",
        "a2",
        "a3",
        "a4",
        "a5",
        "a6",
        "a7",
        "a8",
        "a9",
        "a10",
        "a11",
        "a12",
        "a13",
        "a14",
        "a15",
        "a16",

        "@abc",
        "@",
        "_abc",
        "_",
        "`abc",
        "`",
        "~abc",
        "~",
        "+abc",
        "+",
        "=abc",
        "=",
        "-abc",
        "-",
        "1",
        "1a",
        "a",
        "abc_defgh",
        "abc-defgh",
        "bcd",
        "a.b.c",
        "a b c",
        "a                     b                     c",
        "^",
        "^",
        "!",
        "!abc",
        "#",
        "#abc",
        "$",
        "$abc",
        "%",
        "%abc",
        "&",
        "&abc",
        "{}",
        "{abc}",
        ";abc",
        ";",
        ";;;",
        ";;;abc",
        "[]",
        "[abc]",
        ",",
        "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"
        "ffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
        "kkkkkkkkkkllllllllllmmmmmmmmmmnnnnnnnnnnoooooooooo"
        "ppppppppppqqqqqqqqqqrrrrrrrrrrsssssssssstttttttttt"
        "uuuuuuuuuuvvvvvvvvvvwwwwwwwwwwxxxxxxxxxxyyyyyyyyyy"
        "zzzzz", // 255
        "...a",
        "..opq",
        ".bcd",
        // ".e.f.g.",
        ".e.f.g",
        // "hij.",
        // "k.l.m.",
        "k.l.m",
        // "rst..",
        // "u..v..w..",
        "u..v..w",
        // "xyz..."
    };

    esReport("Create Files\n");

    int i;
    for (i = 0; i < sizeof(nameList)/sizeof(nameList[0]); ++i)
    {
        int len = strlen(nameList[i]);
        esReport("create \"%s\" (len %d)\n", nameList[i], len);

        Handle<IFile> file = root->lookup(nameList[i]);
        if (!file)
        {
            file = root->bind(nameList[i], 0);
            TEST(file->isFile());
            TEST(!file->isDirectory());
            TEST(file->canRead());
            TEST(file->canWrite());
            TEST(!file->isHidden());

            Handle<IStream>     stream;
            stream = file->getStream();
            long ret = stream->write("0123456789\n", 11);
            TEST(ret == 11);

            //stream->flush();
        }
        TEST(file);

        char created[512];
        file->getName(created, sizeof(created));
        TEST(strcmp(created, nameList[i]) == 0);
    }

    // create a file with the path.
    root->createSubcontext("dir");
    Handle<IFile> file = root->bind("dir/abc", 0);
    TEST(file);
    file = 0;

    // create the file that already exists.
    file = root->bind("abc", 0);
    TEST(file);
    file = 0;
    file = root->bind("abc", 0);
    TEST(!file);
    file = root->bind("dir", 0);
    TEST(!file);

    // check wrong name.
    const char* wrongNameList[] =
    {
//        "\\",
//        "/",
        "\"",
        "*",
        "<",
        ">",
        "|",
        ":",
        "?",
        "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"
        "ffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
        "kkkkkkkkkkllllllllllmmmmmmmmmmnnnnnnnnnnoooooooooo"
        "ppppppppppqqqqqqqqqqrrrrrrrrrrsssssssssstttttttttt"
        "uuuuuuuuuuvvvvvvvvvvwwwwwwwwwwxxxxxxxxxxyyyyyyyyyy"
        "zzzzzz" // 256
    };

    for (i = 0; i < sizeof(wrongNameList)/sizeof(wrongNameList[0]); ++i)
    {
        int len = strlen(wrongNameList[i]);
        esReport("create \"%s\" (len %d)\n", wrongNameList[i], len);

        Handle<IFile> file = root->lookup(wrongNameList[i]);
        if (!file)
        {
            try
            {
                file = root->bind(wrongNameList[i], 0);
            }
            catch (SystemException<EACCES>)
            {
            }
            catch (SystemException<ENAMETOOLONG>)
            {
            }
        }
        TEST(!file);
    }
}

int main(void)
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

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

    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<IContext> root;

        root = fatFileSystem->getRoot();
        TestFileSystem(root);
        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
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
