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

using namespace es;

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
    process->setInput(esReportStream());
    process->setOutput(esReportStream());
    process->setError(esReportStream());
    process->start(file);
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

    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);
    {
        Handle<IContext> root;

        root = fatFileSystem->getRoot();
        nameSpace->bind("file", root);

        // start server process.
        Handle<IProcess> serverProcess;
        serverProcess = reinterpret_cast<IProcess*>(
            esCreateInstance(CLSID_Process, IProcess::iid()));
        TEST(serverProcess);
        Handle<IFile> serverElf = nameSpace->lookup("file/upcallTest.elf");
        TEST(serverElf);
        startProcess(nameSpace, serverProcess, serverElf);

        // start client process.
        Handle<IProcess> clientProcess;
        clientProcess = reinterpret_cast<IProcess*>(
            esCreateInstance(CLSID_Process, IProcess::iid()));
        TEST(clientProcess);
        Handle<IFile> clientElf = nameSpace->lookup("file/upcallTestClient.elf");
        TEST(clientElf);

        startProcess(nameSpace, clientProcess, clientElf);

        clientProcess->wait();
        esReport("client process exited.\n");

        serverProcess->wait();
        esReport("server process exited.\n");

        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);

        nameSpace->unbind("file");
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esSleep(10000000);
    esReport("done.\n");
}
