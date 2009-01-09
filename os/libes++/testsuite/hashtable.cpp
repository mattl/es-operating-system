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

    h.add(IProcess::iid(), 1);
    h.add(IThread::iid(), 2);
    h.add(IMonitor::iid(), 3);

    i = h.get(IProcess::iid());
    printf("h.get(IProcess::iid()) : %d\n", i);
    ASSERT(i == 1);
    i = h.get(IThread::iid());
    printf("h.get(IThread::iid()) : %d\n", i);
    ASSERT(i == 2);
    i = h.get(IMonitor::iid());
    printf("h.get(IMonitor::iid()) : %d\n", i);
    ASSERT(i == 3);
    try
    {
        i = h.get(IAlarm::iid());
    }
    catch (Exception& error)
    {
        ASSERT(error.getResult() == ENOENT);
        printf("h.get(IAlarm::iid()) : exception %d\n", error.getResult());
    }

    contains = h.remove(IMonitor::iid());
    printf("h.remove(IMonitor::iid()) : %d\n", contains);
    ASSERT(contains == true);
    contains = h.remove(IMonitor::iid());
    printf("h.remove(IMonitor::iid()) : %d\n", contains);
    ASSERT(contains == false);
    contains = h.contains(IMonitor::iid());
    printf("h.contains(IMonitor::iid()) : %d\n", contains);
    ASSERT(contains == false);

    try
    {
        h.add(IProcess::iid(), 5);
    }
    catch (Exception& error)
    {
        ASSERT(error.getResult() == EEXIST);
        printf("h.get(IProcess::iid()) : exception %d\n", error.getResult());
    }

    h.clear();
    contains = h.contains(IProcess::iid());
    printf("h.contains(IProcess::iid()) : %d\n", contains);
    ASSERT(contains == false);

    printf("done.\n");
}
