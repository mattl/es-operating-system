/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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
#include <es.h>
#include <es/hashtable.h>
#include <es/base/IAlarm.h>
#include <es/base/IMonitor.h>
#include <es/base/IProcess.h>
#include <es/base/IThread.h>

using namespace es;

int main()
{
    Hashtable<const char*, int> h(100);

    printf("h.size() : %d\n", h.size());

    int i;
    bool contains;

    h.add(es::Process::iid(), 1);
    h.add(es::Thread::iid(), 2);
    h.add(es::Monitor::iid(), 3);

    i = h.get(es::Process::iid());
    printf("h.get(es::Process::iid()) : %d\n", i);
    ASSERT(i == 1);
    i = h.get(es::Thread::iid());
    printf("h.get(es::Thread::iid()) : %d\n", i);
    ASSERT(i == 2);
    i = h.get(es::Monitor::iid());
    printf("h.get(es::Monitor::iid()) : %d\n", i);
    ASSERT(i == 3);
    try
    {
        i = h.get(es::Alarm::iid());
    }
    catch (Exception& error)
    {
        ASSERT(error.getResult() == ENOENT);
        printf("h.get(es::Alarm::iid()) : exception %d\n", error.getResult());
    }

    contains = h.remove(es::Monitor::iid());
    printf("h.remove(es::Monitor::iid()) : %d\n", contains);
    ASSERT(contains == true);
    contains = h.remove(es::Monitor::iid());
    printf("h.remove(es::Monitor::iid()) : %d\n", contains);
    ASSERT(contains == false);
    contains = h.contains(es::Monitor::iid());
    printf("h.contains(es::Monitor::iid()) : %d\n", contains);
    ASSERT(contains == false);

    try
    {
        h.add(es::Process::iid(), 5);
    }
    catch (Exception& error)
    {
        ASSERT(error.getResult() == EEXIST);
        printf("h.get(es::Process::iid()) : exception %d\n", error.getResult());
    }

    h.clear();
    contains = h.contains(es::Process::iid());
    printf("h.contains(es::Process::iid()) : %d\n", contains);
    ASSERT(contains == false);

    printf("done.\n");
}
