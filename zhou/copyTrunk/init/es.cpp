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

#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/device/IFileSystem.h>

#include <string.h>
#include <es.h>
#include <es/dateTime.h>
#include <es/endian.h>
#include <es/handle.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/base/IService.h>
#include <es/base/IStream.h>
#include <es/base/IThread.h>
#include <es/device/IFatFileSystem.h>
#include <es/device/INetworkInterface.h>
#include <es/naming/IContext.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include <es/net/arp.h>
#include <es/net/dhcp.h>
#include <es/net/dns.h>
#include <es/net/udp.h>

#include "fatStream.h"
#include "iso9660Stream.h"



int esInit(Object** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);
extern void esRegisterDHCPClient(es::Context* context);

es::Stream* esReportStream();

void startProcess(Handle<es::Context> root, Handle<es::Process> process, Handle<es::File> file)
{
    ASSERT(root);
    ASSERT(process);
    ASSERT(file);

    long long size = 0;

    size = file->getSize();
    esReport("size: %lld\n", size);

    process->setRoot(root);
    process->setCurrent(root);
    process->setInput(esReportStream());
    process->setOutput(esReportStream());
    process->setError(esReportStream());
    process->start(file);
}

void init(Handle<es::Context> root)
{
    Handle<es::Iterator>   iter;
    Handle<es::File>       file;
    long long           size = 0;

    // get console.
    Handle<es::Stream> console = 0;
    while (!console)
    {
        console = root->lookup("device/console");
        esSleep(10000000 / 60);
    }

    file = root->lookup("file/esjs.elf");
    if (!file)
    {
        esReport("Could not open \"esjs.elf\"\n");
        return;
    }
    size = file->getSize();
    esReport("main size: %lld\n", size);

    Handle<es::Process> process;
    process = es::Process::createInstance();
    ASSERT(process);
    process->setRoot(root);
    process->setCurrent(root);
    process->setInput(console);
    process->setOutput(console);
    process->setError(console);
    process->start(file, "esjs file/shell.js");
    process->wait();
    esReport("esjs exited.\n");
}

int initNetwork(Handle<es::Context> context)
{
    // Get DIX interface
    Handle<es::NetworkInterface> ethernetInterface = context->lookup("device/ethernet");
    if (!ethernetInterface)
    {
        return -1;
    }

    esRegisterInternetProtocol(context);

    // Lookup resolver object
    Handle<es::Resolver> resolver = context->lookup("network/resolver");

    // Lookup internet config object
    Handle<es::InternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    ethernetInterface->start();
    int dixID = config->addInterface(ethernetInterface);
    esReport("dixID: %d\n", dixID);

    esRegisterDHCPClient(context);

    Handle<es::Service> service = context->lookup("network/interface/2/dhcp");
    if (service)
    {
        service->start();
    }

#if 0
    esSleep(120000000);

    Handle<es::InternetAddress> host = config->getAddress(dixID);
    if (host)
    {
        InAddr addr;

        host->getAddress(&addr, sizeof(InAddr));
        u32 h = ntohl(addr.addr);
        esReport("host: %d.%d.%d.%d\n", (u8) (h >> 24), (u8) (h >> 16), (u8) (h >> 8), (u8) h);
    }
#endif

    if (service)
    {
        // service->stop();
    }
    // ethernetInterface->stop();
    return 0;
}

int main(int argc, char* argv[])
{
    Object* ns = 0;
    esInit(&ns);
    Handle<es::Context> nameSpace(ns);

    initNetwork(nameSpace);

    FatFileSystem::initializeConstructor();
    Iso9660FileSystem::initializeConstructor();

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
        Handle<es::Context> root = fatFileSystem->getRoot();

        nameSpace->bind("file", root);

        // start event manager process.
        Handle<es::Process> eventProcess;
        eventProcess = es::Process::createInstance();
        Handle<es::File> eventElf = nameSpace->lookup("file/eventManager.elf");
        ASSERT(eventElf);
        startProcess(nameSpace, eventProcess, eventElf);

        // start console process.
        Handle<es::Process> consoleProcess;
        consoleProcess =es::Process::createInstance();
        Handle<es::File> consoleElf = nameSpace->lookup("file/console.elf");
        ASSERT(consoleElf);
        startProcess(nameSpace, consoleProcess, consoleElf);

        init(nameSpace);

        consoleProcess->kill();
        eventProcess->kill();

        esSleep(10000000);
    }

    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);

    fatFileSystem->dismount();
    fatFileSystem = 0;
    esSleep(10000000);
}
