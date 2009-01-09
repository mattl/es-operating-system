/*
 * Copyright 2008, 2009 Google Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <es/reflect.h>
#include <es/base/IInterface.h>
#include <es/base/IAlarm.h>
#include <es/base/ICache.h>
#include <es/base/IPageSet.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>
#include <es/device/IIso9660FileSystem.h>
#include <es/device/IPartition.h>

namespace es
{
    Reflect::Interface& getInterface(const char* iid);
    IInterface* getConstructor(const char* iid);
    extern unsigned char* defaultInterfaceInfo[];
    extern size_t defaultInterfaceCount;
}  // namespace es

extern es::ICurrentProcess* System();

using namespace es;

int main(int argc, char* argv[])
{
    printf("%p\n", getConstructor(IAlarm::iid()));
    printf("%p\n", IAlarm::getConstructor());
    if (getConstructor(IAlarm::iid()) != reinterpret_cast<IInterface*>(IAlarm::getConstructor()))
    {
        return EXIT_FAILURE;
    }

    IAlarm* alarm = IAlarm::createInstance();
    alarm->isPeriodic();
    alarm->release();

    printf("%p\n", getConstructor(ICache::iid()));
    printf("%p\n", getConstructor(IMonitor::iid()));
    printf("%p\n", getConstructor(IPageSet::iid()));
    printf("%p\n", getConstructor(IProcess::iid()));
    printf("%p\n", getConstructor(IFatFileSystem::iid()));
    printf("%p\n", getConstructor(IIso9660FileSystem::iid()));
    printf("%p\n", getConstructor(IPartition::iid()));

    printf("done.\n");
    return EXIT_SUCCESS;
}
