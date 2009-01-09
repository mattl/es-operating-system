/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>

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

    Handle<IContext> classStore(nameSpace->lookup("class"));

    Handle<IStream> disk = nameSpace->lookup("device/ata/channel0/device0");
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = IFatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    {
        Handle<IContext> root;

        root = fatFileSystem->getRoot();
        nameSpace->bind("file", root);

        // start server process.
        Handle<IProcess> serverProcess;
        serverProcess = IProcess::createInstance();
        TEST(serverProcess);
        Handle<IFile> serverElf = nameSpace->lookup("file/upcallTest.elf");
        TEST(serverElf);
        startProcess(nameSpace, serverProcess, serverElf);

        // start client process.
        Handle<IProcess> clientProcess;
        clientProcess = IProcess::createInstance();
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
