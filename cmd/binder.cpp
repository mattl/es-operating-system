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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include <es/naming/IBinding.h>
#include "binder.h"

using namespace es;

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

class Binder : public IBinding
{
    static int  id;

    Ref         ref;
    IInterface* object;
    char        name[14];

public:
    Binder() :
        object(0)
    {
        sprintf(name, "id%d", ++id);
    }

    ~Binder()
    {
        if (object)
        {
            object->release();
        }
    }

    IInterface* getObject()
    {
        return object;
    }

    void setObject(IInterface* element)
    {
        if (element)
        {
            element->addRef();
        }
        if (object)
        {
            object->release();
        }
        object = element;
    }

    int getName(char* name, int len)
    {
        unsigned count(strlen(this->name) + 1);
        if (len < count)
        {
            count = len;
        }
        strncpy(name, this->name, count);
        return count;
    }

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IInterface::iid())
        {
            objectPtr = static_cast<IBinding*>(this);
        }
        else if (riid == IBinding::iid())
        {
            objectPtr = static_cast<IBinding*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

int Binder::id = 0;

int main(int argc, char* argv[])
{
    esReport("This is the Binder server process.\n");
    System()->trace(true);

    Handle<IContext> nameSpace = System()->getRoot();
    Handle<IClassStore> classStore = nameSpace->lookup("class");
    TEST(classStore);

    // Register Binder factory.
    Handle<IClassFactory> binderFactory(new(ClassFactory<Binder>));
    classStore->add(CLSID_Binder, binderFactory);

    // Create a client process.
    Handle<IProcess> client;
    client = reinterpret_cast<IProcess*>(
        classStore->createInstance(CLSID_Process, client->iid()));
    TEST(client);

    // Start the client process.
    Handle<IFile> file = nameSpace->lookup("file/binderClient.elf");
    TEST(file);
    client->start(file);

    // Wait for the client to exit
    client->wait();

    // Unregister Binder factory.
    classStore->remove(CLSID_Binder);

    System()->trace(false);
}
