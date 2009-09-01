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

#include "../IEventQueue.h"



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
    esSleep(20000000LL); // [check] workaround.
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

        // start event manager process.
        Handle<es::Process> eventProcess;
        eventProcess = es::Process::createInstance();
        TEST(eventProcess);
        Handle<es::File> eventElf = nameSpace->lookup("file/eventManager.elf");
        TEST(eventElf);
        startProcess(nameSpace, eventProcess, eventElf);

        // start console process.
        Handle<es::Process> consoleProcess;
        consoleProcess =es::Process::createInstance();
        TEST(consoleProcess);
        Handle<es::File> consoleElf = nameSpace->lookup("file/console.elf");
        TEST(consoleElf);
        startProcess(nameSpace, consoleProcess, consoleElf);

        // start console client process.
        Handle<es::Process> consoleClientProcess;
        consoleClientProcess =es::Process::createInstance();
        TEST(consoleClientProcess);

        Handle<es::File> consoleClientElf = nameSpace->lookup("file/consoleClient.elf");
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
