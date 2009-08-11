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

static long TestFileSystem(Handle<es::Context> root)
{
    const char* dirList[] =
    {
        "ﾃﾞｨﾚｸﾄﾘ",
        "ディレクトリ",
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

    esReport("Create Directories\n");

    int i;
    for (i = 0; i < sizeof(dirList)/sizeof(dirList[0]); ++i)
    {
        int len = strlen(dirList[i]);
        esReport("create \"%s\" (len %d)\n", dirList[i], len);

        Handle<es::File> file = root->lookup(dirList[i]);
        if (!file)
        {
            file = root->createSubcontext(dirList[i]);
            TEST(file->isDirectory());
            TEST(file->canRead());
            TEST(file->canWrite());
            TEST(!file->isHidden());
            TEST(!file->isFile());
        }
        TEST(file);

        char created[512];
        file->getName(created, sizeof(created));
        TEST(strcmp(created, dirList[i]) == 0);
    }

    Handle<es::File> dir = root->createSubcontext("abc");
    TEST(dir);
    dir = 0;
    dir = root->createSubcontext("abc/def");
    TEST(dir);
    dir = 0;
    dir = root->createSubcontext("abc\\ghi");
    TEST(dir);
    dir = 0;

    Handle<es::Context> abc  = root->lookup("abc");
    TEST(abc);
    Handle<es::Iterator>   iter;
    iter = abc->list("");
    long n = 0;
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
        TEST(strcmp(name, "def") == 0 || strcmp(name, "ghi") == 0);
        ++n;
    }
    TEST(n == 2);

    // create the directory that already exists.
    dir = root->createSubcontext("abc");
    TEST(!dir);

    // check wrong name.
    const char* wrongDirList[] =
    {
        "*",
        "<",
        ">",
        "\"",
        "|",
        ":",
        "?",
        "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"
        "ffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
        "kkkkkkkkkkllllllllllmmmmmmmmmmnnnnnnnnnnoooooooooo"
        "ppppppppppqqqqqqqqqqrrrrrrrrrrsssssssssstttttttttt"
        "uuuuuuuuuuvvvvvvvvvvwwwwwwwwwwxxxxxxxxxxyyyyyyyyyy"
        "zzzzzz", // 256,
        "nonexistent/dir",
        "nonexistent\\dir"
    };

    for (i = 0; i < sizeof(wrongDirList)/sizeof(wrongDirList[0]); ++i)
    {
        int len = strlen(wrongDirList[i]);
        esReport("create \"%s\" (len %d)\n", wrongDirList[i], len);

        try
        {
            Handle<es::File> file = root->createSubcontext(wrongDirList[i]);
            TEST(!file);
        }
        catch (SystemException<EACCES>& e)
        {
            TEST(strcmp(wrongDirList[i], "*") == 0 ||
                 strcmp(wrongDirList[i], "<") == 0 ||
                 strcmp(wrongDirList[i], ">") == 0 ||
                 strcmp(wrongDirList[i], "\"") == 0 ||
                 strcmp(wrongDirList[i], "|") == 0 ||
                 strcmp(wrongDirList[i], ":") == 0 ||
                 strcmp(wrongDirList[i], "?") == 0);
        }
        catch (SystemException<ENAMETOOLONG>& e)
        {
            TEST(strcmp(wrongDirList[i],
                "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"
                "ffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                "kkkkkkkkkkllllllllllmmmmmmmmmmnnnnnnnnnnoooooooooo"
                "ppppppppppqqqqqqqqqqrrrrrrrrrrsssssssssstttttttttt"
                "uuuuuuuuuuvvvvvvvvvvwwwwwwwwwwxxxxxxxxxxyyyyyyyyyy"
                "zzzzzz") == 0);
        }
    }

    return 0;
}

int main(void)
{
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/ata/channel0/device0");
#else
    Handle<es::Stream> disk = new VDisk(static_cast<char*>("fat32.img"));
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
        TEST (ret == 0);
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
