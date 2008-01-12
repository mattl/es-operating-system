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
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include "location.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

int main(int argc, char* argv[])
{
    esReport("This is the Binder client process.\n");
    System()->trace(true);

    Handle<IContext> nameSpace = System()->getRoot();
    Handle<IClassStore> classStore = nameSpace->lookup("class");
    TEST(classStore);

    // Create binder objects.
    Handle<ILocation> location[2];
    for (int i(0); i < 2; ++i)
    {
        bool result =
            classStore->createInstance(CLSID_Location,
                                       location[i]->interfaceID(),
                                       reinterpret_cast<void**>(&location[i]));
        TEST(result);

        char name[14];
        location[i]->getName(name, sizeof(name));
        esReport("%s\n", name);

        sprintf(name, "foo%d", i);
        location[i]->setName(name);
        memset(name, 0, sizeof(name));
        location[i]->getName(name, sizeof(name));
        esReport("%s\n", name);

        Point point = { i, -i };
        location[i]->move(&point);
        memset(&point, 0, sizeof(Point));
        location[i]->get(&point);
        esReport("point: (%d, %d)\n", point.x, point.y);
    }
    System()->trace(false);
}
