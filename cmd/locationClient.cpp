/*
 * Copyright 2008 Google Inc.
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

#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include "location.h"

using namespace es;

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
        location[i] = reinterpret_cast<IProcess*>(
            classStore->createInstance(CLSID_Location,
                                       location[i]->iid()));
        TEST(location[i]);

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
