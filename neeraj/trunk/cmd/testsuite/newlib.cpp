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
#include <es.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>



int esInit(Object** nameSpace);
es::Stream* esReportStream();

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void test(Handle<es::Context> nameSpace)
{
    Handle<es::Iterator>   iter;
    Handle<es::File>       file;
    Handle<es::Stream>     stream;
    long long           size = 0;

    file = nameSpace->lookup("file/newlib.elf");
    size = file->getSize();
    esReport("server size: %lld\n", size);

    Handle<es::Process> process;
    process = es::Process::createInstance();
    TEST(process);
    process->setRoot(nameSpace);
    process->setInput(esReportStream());
    process->setOutput(esReportStream());
    process->setError(esReportStream());
    process->start(file, "");
    process->wait();
    esReport("server process exited.\n");
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

        test(nameSpace);

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
