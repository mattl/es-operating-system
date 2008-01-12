/*
 * Copyright (c) 2006, 2007
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
#include <string.h>
#include <es.h>
#include <es/clsid.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/device/IFileSystem.h>

#include "../IEventQueue.h"

int esInit(IInterface** nameSpace);
IStream* esReportStream();

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void startProcess(Handle<IContext> root, Handle<IProcess> process, Handle<IFile> file)
{
    TEST(root);
    TEST(process);
    TEST(file);

    long long  size = 0;

    size = file->getSize();
    esReport("size: %lld\n", size);

    process->setRoot(root);
    process->setIn(esReportStream());
    process->setOut(esReportStream());
    process->setError(esReportStream());
    process->start(file);
    esSleep(20000000LL); // [check] workaround.
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);

    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

    Handle<IStream> disk = nameSpace->lookup("device/ata/channel0/device0");
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);
    {
        Handle<IContext> root;

        fatFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        nameSpace->bind("file", root);

        // start event manager process.
        Handle<IProcess> eventProcess;
        esCreateInstance(CLSID_Process, IID_IProcess,
                         reinterpret_cast<void**>(&eventProcess));
        TEST(eventProcess);
        Handle<IFile> eventElf = nameSpace->lookup("file/eventManager.elf");
        TEST(eventElf);
        startProcess(nameSpace, eventProcess, eventElf);

        // start console process.
        Handle<IProcess> consoleProcess;
        esCreateInstance(CLSID_Process, IID_IProcess,
                         reinterpret_cast<void**>(&consoleProcess));
        TEST(consoleProcess);
        Handle<IFile> consoleElf = nameSpace->lookup("file/console.elf");
        TEST(consoleElf);
        startProcess(nameSpace, consoleProcess, consoleElf);

        // start console client process.
        Handle<IProcess> consoleClientProcess;
        esCreateInstance(CLSID_Process, IID_IProcess,
                         reinterpret_cast<void**>(&consoleClientProcess));
        TEST(consoleClientProcess);

        Handle<IFile> consoleClientElf = nameSpace->lookup("file/consoleClient.elf");
        TEST(consoleClientElf);
        startProcess(nameSpace, consoleClientProcess, consoleClientElf);

        consoleClientProcess->wait();
        esReport("console client process exited.\n");

        consoleProcess->wait();
        esReport("console process exited.\n");
        eventProcess->wait();
        esReport("event manager process exited.\n");

        fatFileSystem->getFreeSpace(freeSpace);
        fatFileSystem->getTotalSpace(totalSpace);
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);

        nameSpace->unbind("file");
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esSleep(10000000);
    esReport("done.\n");
}
