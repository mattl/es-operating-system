/*
 * Copyright 2008, 2009 Google Inc.
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/IInterfaceStore.h>
#include <es/base/IProcess.h>
#include "location.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

es::CurrentProcess* System();

class Location : public es::Location
{
    static int  id;

    Ref         ref;
    es::Point   point;
    char        name[14];

public:
    Location()
    {
        point.x = point.y = 0;
        sprintf(name, "id%d", ++id);
        name[13] = '\0';
    }

    ~Location()
    {
    }

    void set(const es::Point* point)
    {
        this->point = *point;
    }

    void get(es::Point* point)
    {
        *point = this->point;
    }

    void move(const es::Point* direction)
    {
        point.x += direction->x;
        point.y += direction->y;
    }

    void setName(const char* name)
    {
        strncpy(this->name, name, 13);
    }

    const char* getName(void* name, int len)
    {
        unsigned count(strlen(this->name) + 1);
        if (len < count)
        {
            count = len;
        }
        strncpy(static_cast<char*>(name), this->name, count);
        return static_cast<char*>(name);
    }

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Location*>(this);
        }
        else if (strcmp(riid, es::Location::iid()) == 0)
        {
            objectPtr = static_cast<es::Location*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
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

    // [Constructor]
    class Constructor : public es::Location::Constructor
    {
    public:
        es::Location* createInstance();
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    static Constructor constructor;
};

es::Location* Location::Constructor::createInstance()
{
    return new Location;
}

Object* Location::Constructor::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Location::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::Location::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Location::Constructor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Location::Constructor::addRef()
{
    return 1;
}

unsigned int Location::Constructor::release()
{
    return 1;
}

Location::Constructor Location::constructor;

int Location::id = 0;

int main(int argc, char* argv[])
{
    esReport("This is the Location server process.\n");
    System()->trace(true);

    Handle<es::Context> nameSpace = System()->getRoot();

    // Register es::Location interface.
    Handle<es::InterfaceStore> interfaceStore = nameSpace->lookup("interface");
    TEST(interfaceStore);
    interfaceStore->add(ILocationInfo, ILocationInfoSize);

    // Register Location factory.
    Handle<es::Context> classStore = nameSpace->lookup("class");
    TEST(classStore);
    classStore->bind(es::Location::iid(), es::Location::getConstructor());

    // Create a client process.
    Handle<es::Process> client;
    client = es::Process::createInstance();
    TEST(client);

    // Start the client process.
    Handle<es::File> file = nameSpace->lookup("file/locationClient.elf");
    TEST(file);
    client->start(file);

    // Wait for the client to exit
    client->wait();

    // Unregister Location factory.
    classStore->unbind(es::Location::iid());

    System()->trace(false);
}
