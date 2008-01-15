/*
 * Copyright 2008 Google Inc.
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
#include <es/clsid.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/device/IFileSystem.h>

#include "../IEventQueue.h"

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

    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);
    {
        Handle<IContext> root;

        root = fatFileSystem->getRoot();
        nameSpace->bind("file", root);

        // start event manager process.
        Handle<IProcess> eventProcess;
        eventProcess = reinterpret_cast<IProcess*>(
            esCreateInstance(CLSID_Process, IProcess::iid()));
        TEST(eventProcess);
        Handle<IFile> eventElf = nameSpace->lookup("file/eventManager.elf");
        TEST(eventElf);
        startProcess(nameSpace, eventProcess, eventElf);

        // start console process.
        Handle<IProcess> consoleProcess;
        consoleProcess = reinterpret_cast<IProcess*>(
            esCreateInstance(CLSID_Process, IProcess::iid()));
        TEST(consoleProcess);
        Handle<IFile> consoleElf = nameSpace->lookup("file/console.elf");
        TEST(consoleElf);
        startProcess(nameSpace, consoleProcess, consoleElf);

        // start console client process.
        Handle<IProcess> consoleClientProcess;
        consoleClientProcess = reinterpret_cast<IProcess*>(
            esCreateInstance(CLSID_Process, IProcess::iid()));
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
