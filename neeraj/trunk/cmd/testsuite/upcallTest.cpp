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



int esInit(Object** nameSpace);
es::Stream* esReportStream();

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void startProcess(Handle<es::Context> root, Handle<es::Process> process, Handle<es::File> file)
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
    Object* ns = 0;
    esInit(&ns);

    Handle<es::Context> nameSpace(ns);

    Handle<es::Context> classStore(nameSpace->lookup("class"));

    Handle<es::Stream> disk = nameSpace->lookup("device/ata/channel0/device0");
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<es::FileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = es::FatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    {
        Handle<es::Context> root;

        root = fatFileSystem->getRoot();
        nameSpace->bind("file", root);

        // start server process.
        Handle<es::Process> serverProcess;
        serverProcess = es::Process::createInstance();
        TEST(serverProcess);
        Handle<es::File> serverElf = nameSpace->lookup("file/upcallTest.elf");
        TEST(serverElf);
        startProcess(nameSpace, serverProcess, serverElf);

        // start client process.
        Handle<es::Process> clientProcess;
        clientProcess = es::Process::createInstance();
        TEST(clientProcess);
        Handle<es::File> clientElf = nameSpace->lookup("file/upcallTestClient.elf");
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
