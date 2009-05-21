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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/IProcess.h>
#include <es/naming/IBinding.h>



#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

es::CurrentProcess* System();

class Binder : public es::Binding
{
    static int  id;

    Ref         ref;
    Object* object;
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

    Object* getObject()
    {
        return object;
    }

    void setObject(Object* element)
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

    void* queryInterface(const char* riid)
    {
        void* objectPtr;
        if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Binding*>(this);
        }
        else if (strcmp(riid, es::Binding::iid()) == 0)
        {
            objectPtr = static_cast<es::Binding*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<Object*>(objectPtr)->addRef();
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

    Handle<es::Context> nameSpace = System()->getRoot();
    Handle<es::Context> classStore = nameSpace->lookup("class");
    TEST(classStore);

    // Register Binder factory.
    classStore->bind(IBinder::iid(), Binder::constructor);

    // Create a client process.
    Handle<es::Process> client;
    client = es::Process::createInstance();
    TEST(client);

    // Start the client process.
    Handle<es::File> file = nameSpace->lookup("file/binderClient.elf");
    TEST(file);
    client->start(file);

    // Wait for the client to exit
    client->wait();

    // Unregister Binder factory.
    classStore->unbind(IBinder::iid());

    System()->trace(false);
}
