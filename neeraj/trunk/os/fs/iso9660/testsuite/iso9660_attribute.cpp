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
#include <es/exception.h>
#include <es/handle.h>
#include "vdisk.h"
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))


struct AttributeEntry
{
    char         name[32];
    unsigned int attr;
};

static AttributeEntry AttrList[] =
{
   {"R", es::File::ReadOnly}, // "ReadOnly",
   {"H", es::File::Hidden},   // "Hidden",
   {"D", es::File::Directory} // "Directory",
};

static AttributeEntry DirList[] =
{
    {"dir",              es::File::ReadOnly | es::File::Directory | es::File::Hidden},
    {"dir2",             es::File::ReadOnly | es::File::Directory},
    {"System",           es::File::ReadOnly | es::File::Directory},
    {"hiden2",           es::File::ReadOnly | es::File::Hidden},
    {"system_file",      es::File::ReadOnly | es::File::Hidden},
    {"archive",          es::File::ReadOnly},
    {"file01",           es::File::ReadOnly},
    {"file02",           es::File::ReadOnly},
    {"file03",           es::File::ReadOnly},
    {"hidden",           es::File::ReadOnly},
    {"none",             es::File::ReadOnly},
    {"readonly_archive", es::File::ReadOnly}
};

static void PrintAttribute(unsigned int attr)
{
    esReport("[");
    int i;
    for (i = 0; i < (int) (sizeof(AttrList)/sizeof(AttrList[0])); ++i)
    {
        esReport("%s", attr & AttrList[i].attr ? AttrList[i].name : "_");
    }
    esReport("]");
}

void test(Handle<es::Context> root)
{
    Handle<es::Context>    dir = root->lookup("attribute");
    TEST(dir);

    // test getAttribute().
    unsigned int attr;
    long long n = 0;
    int i;
    Handle<es::Iterator>   iter;
    iter = dir->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
        ++n;
        Handle<es::File> file = binding->getObject();

        try
        {
            attr = file->getAttributes();
        }
        catch (Exception& error)
        {
            TEST(false);
        }
#ifdef VERBOSE
        PrintAttribute(attr);
        esReport("\"%s\"\n", name);
#endif // VERBOSE
        bool found = false;
        for (i = 0; i < sizeof(DirList)/sizeof(DirList[0]); ++i)
        {
            if (strcmp(name, DirList[i].name) == 0)
            {
                TEST(attr == DirList[i].attr);
                found = true;
                break;
            }
        }
        TEST(found);

        if (DirList[i].attr & es::File::ReadOnly)
        {
            TEST(file->canRead());
            TEST(!file->canWrite());
        }

        if (DirList[i].attr & es::File::Directory)
        {
            TEST(file->isDirectory());
            TEST(!file->isFile());
        }
        else
        {
            TEST(!file->isDirectory());
            TEST(file->isFile());
        }

        if (DirList[i].attr & es::File::Hidden)
        {
            TEST(file->isHidden());
        }
    }

    // test setAttribute().
    Handle<es::File> file = dir->lookup("file01");
    TEST(file);

    attr = (es::File::ReadOnly | es::File::Hidden);
    int ret = 0;
    try
    {
        file->setAttributes(attr);
    }
    catch (Exception& error)
    {
        ret = -1;
    }
    TEST(ret < 0);

    try
    {
        attr = file->getAttributes();
        ret = 0;
    }
    catch (Exception& error)
    {
        ret = -1;
    }
    TEST(ret == 0);

    TEST(attr == es::File::ReadOnly);
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
