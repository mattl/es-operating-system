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
#include <es/object.h>
#include <es/base/IAlarm.h>
#include <es/base/ICache.h>
#include <es/base/IPageSet.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>
#include <es/device/IIso9660FileSystem.h>

namespace es
{
    Reflect::Interface& getInterface(const char* iid);
    Object* getConstructor(const char* iid);
    extern unsigned char* defaultInterfaceInfo[];
    extern size_t defaultInterfaceCount;
}  // namespace es

extern es::CurrentProcess* System();

int main(int argc, char* argv[])
{
    printf("%p\n", es::getConstructor(es::Alarm::iid()));
    printf("%p\n", es::Alarm::getConstructor());
    if (es::getConstructor(es::Alarm::iid()) != reinterpret_cast<Object*>(es::Alarm::getConstructor()))
    {
        return EXIT_FAILURE;
    }

    es::Alarm* alarm = es::Alarm::createInstance();
    alarm->getPeriodic();
    alarm->release();

    printf("%p\n", es::getConstructor(es::Cache::iid()));
    printf("%p\n", es::getConstructor(es::Monitor::iid()));
    printf("%p\n", es::getConstructor(es::PageSet::iid()));
    printf("%p\n", es::getConstructor(es::Process::iid()));
    printf("%p\n", es::getConstructor(es::FatFileSystem::iid()));
    printf("%p\n", es::getConstructor(es::Iso9660FileSystem::iid()));

    printf("done.\n");
    return EXIT_SUCCESS;
}
