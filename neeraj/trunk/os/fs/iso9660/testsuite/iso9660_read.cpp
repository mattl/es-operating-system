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
#include "vdisk.h"
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

struct CheckList
{
    char name[256];
    int count;
};

static CheckList FileList[] =
{
    {"-",     0},
    {"!",     0},
    {"!abc",  0},
    {"#",     0},
    {"#abc",  0},
    {"$",     0},
    {"$abc",  0},
    {"%",     0},
    {"%abc",  0},
    {"&",     0},
    {"&abc",  0},
    {",",     0},
    {"@",     0},
    {"@abc",  0},
    {"[]",    0},
    {"[abc]", 0},
    {"^",     0},
    {"^abc",  0},
    {"_",     0},
    {"_abc",  0},
    {"`",     0},
    {"`abc",  0},
    {"{}",    0},
    {"{abc}", 0},
    {"~",     0},
    {"~abc",  0},
    {"+",     0},
    {"+abc",  0},
    {"=",     0},
    {"=abc",  0},
    {"1",     0},
    {"1a",    0},
    {"a                     b                     c", 0},
    {"a b c", 0},
    {"a.b.c", 0},
    {"-abc",  0},
    {"abc_defgh", 0},
    {"abc-defgh", 0},
    {"k.l.m",  0},
    {"u..v..w", 0}
};

void test(Handle<es::Context> root)
{
    Handle<es::Context>    dir = root->lookup("test1");
    TEST(dir);
    Handle<es::Iterator>   iter;
    long long           size = 0;
    long long           ret;

    // check to access files in the directory.
    iter = dir->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
#ifdef VERBOSE
        esReport("\"%s\" (%d)\n", name, strlen(name));
#endif // VERBOSE
        bool found = false;
        int count = sizeof(FileList)/sizeof(FileList[0]) - 1;
        while (0 <= count)
        {
            if (strcmp(FileList[count].name, name) == 0)
            {
                found = true;
                FileList[count].count++;
                TEST(FileList[count].count == 1);
                break;
            }
            --count;
        }
        TEST(found);

        Handle<es::File> file = binding;
        Handle<es::Stream>     stream = file->getStream();
        char* buf = new char[32];
        ret = stream->read(buf, sizeof(buf));
        TEST(ret == 4); // test
        delete [] buf;
    }

    // check if all files in the file list were accessed.
    int count = sizeof(FileList)/sizeof(FileList[0]) - 1;
    while (0 <= count)
    {
        TEST(FileList[count].count == 1);
        --count;
    }

    // read data form a longname file.
    iter = 0;
    dir = 0;
    dir = root->lookup("test2");
    iter = dir->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
#ifdef VERBOSE
        esReport("\"%s\" (%d)\n", name, strlen(name));
#endif // VERBOSE
        TEST(strcmp(name, "01234567890123456789012345678901234567890123456789"
                          "01234567890123456789012345678901234567890123456789"
                          "01234567") == 0); // 108 characters.
        Handle<es::File> file = binding;
        Handle<es::Stream>     stream = file->getStream();
        char* buf = new char[32];
        ret = stream->read(buf, sizeof(buf));
        TEST(ret == 4); // test
        delete [] buf;
    }
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
