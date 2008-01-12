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

static long TestFileSystem(Handle<IContext>    root)
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

        Handle<IFile> file = root->lookup(dirList[i]);
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

    Handle<IFile> dir = root->createSubcontext("abc");
    TEST(dir);
    dir = 0;
    dir = root->createSubcontext("abc/def");
    TEST(dir);
    dir = 0;
    dir = root->createSubcontext("abc\\ghi");
    TEST(dir);
    dir = 0;

    Handle<IContext> abc  = root->lookup("abc");
    TEST(abc);
    Handle<IIterator>   iter;
    iter = abc->list("");
    long n = 0;
    while (iter->hasNext())
    {
        char name[1024];
        Handle<IBinding> binding(iter->next());
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
            Handle<IFile> file = root->createSubcontext(wrongDirList[i]);
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
