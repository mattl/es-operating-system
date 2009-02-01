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

#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/base/IProcess.h>
#include <es/naming/IBinding.h>
#include "binder.h"



#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

es::CurrentProcess* System();

int main(int argc, char* argv[])
{
    esReport("This is the Binder client process.\n");
    System()->trace(true);

    Handle<es::Context> nameSpace = System()->getRoot();
    Handle<IClassStore> classStore = nameSpace->lookup("class");
    TEST(classStore);

    // Create binder objects.
    Handle<es::Binding> binder[2];
    for (int i(0); i < 2; ++i)
    {
        binder[i] = reinterpret_cast<es::Binding*>(
            classStore->createInstance(CLSID_Binder,
                                       binder[i]->iid()));
        TEST(binder[i]);

        char name[14];
        binder[i]->getName(name, sizeof(name));
        esReport("%s\n", name);
    }
    System()->trace(false);
}
