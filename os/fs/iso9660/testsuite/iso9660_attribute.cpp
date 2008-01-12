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
#include "vdisk.h"
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))


struct AttributeEntry
{
    char*         name;
    unsigned int attr;
};

static AttributeEntry AttrList[] =
{
   {"R", IFile::ReadOnly}, // "ReadOnly",
   {"H", IFile::Hidden},   // "Hidden",
   {"D", IFile::Directory} // "Directory",
};

static AttributeEntry DirList[] =
{
    {"dir",              IFile::ReadOnly | IFile::Directory | IFile::Hidden},
    {"dir2",             IFile::ReadOnly | IFile::Directory},
    {"System",           IFile::ReadOnly | IFile::Directory},
    {"hiden2",           IFile::ReadOnly | IFile::Hidden},
    {"system_file",      IFile::ReadOnly | IFile::Hidden},
    {"archive",          IFile::ReadOnly},
    {"file01",           IFile::ReadOnly},
    {"file02",           IFile::ReadOnly},
    {"file03",           IFile::ReadOnly},
    {"hidden",           IFile::ReadOnly},
    {"none",             IFile::ReadOnly},
    {"readonly_archive", IFile::ReadOnly}
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

void test(Handle<IContext> root)
{
    Handle<IContext>    dir = root->lookup("attribute");
    TEST(dir);

    // test getAttribute().
    unsigned int attr;
    long long n = 0;
    int i;
    Handle<IIterator>   iter;
    iter = dir->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<IBinding> binding(iter->next());
        binding->getName(name, sizeof name);
        ++n;
        Handle<IFile> file = binding->getObject();

        TEST(file->getAttributes(attr) == 0);
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

        if (DirList[i].attr & IFile::ReadOnly)
        {
            TEST(file->canRead());
            TEST(!file->canWrite());
        }

        if (DirList[i].attr & IFile::Directory)
        {
            TEST(file->isDirectory());
            TEST(!file->isFile());
        }
        else
        {
            TEST(!file->isDirectory());
            TEST(file->isFile());
        }

        if (DirList[i].attr & IFile::Hidden)
        {
            TEST(file->isHidden());
        }
    }

    // test setAttribute().
    Handle<IFile> file = dir->lookup("file01");
    TEST(file);

    attr = (IFile::ReadOnly | IFile::Hidden);
    int ret = file->setAttributes(attr);
    TEST(ret < 0);

    ret = file->getAttributes(attr);
    TEST(ret == 0);

    TEST(attr == IFile::ReadOnly);
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
