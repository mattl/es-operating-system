/*
 * Copyright 2008 Google Inc.
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
#include <es/uuid.h>
#include <es/clsid.h>

using namespace es;

int main()
{
    Hashtable<Guid, int> h(100);

    printf("h.size() : %d\n", h.size());

    int i;
    bool contains;

    h.add(CLSID_Process, 1);
    h.add(CLSID_CacheFactory, 2);
    h.add(CLSID_Monitor, 3);

    i = h.get(CLSID_Process);
    printf("h.get(CLSID_Process) : %d\n", i);
    ASSERT(i == 1);
    i = h.get(CLSID_CacheFactory);
    printf("h.get(CLSID_CacheFactory) : %d\n", i);
    ASSERT(i == 2);
    i = h.get(CLSID_Monitor);
    printf("h.get(CLSID_Monitor) : %d\n", i);
    ASSERT(i == 3);
    try
    {
        i = h.get(CLSID_Alarm);
    }
    catch (Exception& error)
    {
        ASSERT(error.getResult() == ENOENT);
        printf("h.get(CLSID_Alarm) : exception %d\n", error.getResult());
    }

    contains = h.remove(CLSID_Monitor);
    printf("h.remove(CLSID_Monitor) : %d\n", contains);
    ASSERT(contains == true);
    contains = h.remove(CLSID_Monitor);
    printf("h.remove(CLSID_Monitor) : %d\n", contains);
    ASSERT(contains == false);
    contains = h.contains(CLSID_Monitor);
    printf("h.contains(CLSID_Monitor) : %d\n", contains);
    ASSERT(contains == false);

    try
    {
        h.add(CLSID_Process, 5);
    }
    catch (Exception& error)
    {
        ASSERT(error.getResult() == EEXIST);
        printf("h.get(CLSID_Process) : exception %d\n", error.getResult());
    }

    h.clear();
    contains = h.contains(CLSID_Process);
    printf("h.contains(CLSID_Process) : %d\n", contains);
    ASSERT(contains == false);

    printf("done.\n");
}
