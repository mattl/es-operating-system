/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include <stdio.h>
#include <stdlib.h>
#include <es.h>
#include <es/hashtable.h>
#include <es/uuid.h>
#include <es/clsid.h>

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
