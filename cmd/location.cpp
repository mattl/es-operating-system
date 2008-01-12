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
#include <es/base/IInterfaceStore.h>
#include <es/base/IProcess.h>
#include "location.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

class Location : public ILocation
{
    static int  id;

    Ref         ref;
    Point       point;
    char        name[14];

public:
    Location()
    {
        point.x = point.y = 0;
        sprintf(name, "id%d", ++id);
    }

    ~Location()
    {
    }

    void set(const Point* point)
    {
        this->point = *point;
    }

    void get(Point* point)
    {
        *point = this->point;
    }

    void move(const Point* direction)
    {
        point.x += direction->x;
        point.y += direction->y;
    }

    void setName(const char* name)
    {
        strncpy(this->name, name, sizeof name);
    }

    int getName(char* name, unsigned int len)
    {
        unsigned count(strlen(this->name) + 1);
        if (len < count)
        {
            count = len;
        }
        strncpy(name, this->name, count);
        return count;
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<ILocation*>(this);
        }
        else if (riid == IID_ILocation)
        {
            *objectPtr = static_cast<ILocation*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

int Location::id = 0;

int main(int argc, char* argv[])
{
    esReport("This is the Location server process.\n");
    System()->trace(true);

    Handle<IContext> nameSpace = System()->getRoot();

    // Register ILocation interface.
    Handle<IInterfaceStore> interfaceStore = nameSpace->lookup("interface");
    TEST(interfaceStore);
    interfaceStore->add(ILocationInfo, ILocationInfoSize);

    // Register Location factory.
    Handle<IClassStore> classStore = nameSpace->lookup("class");
    TEST(classStore);
    Handle<IClassFactory> binderFactory(new(ClassFactory<Location>));
    classStore->add(CLSID_Location, binderFactory);

    // Create a client process.
    Handle<IProcess> client;
    bool result =
        classStore->createInstance(CLSID_Process,
                                   client->interfaceID(),
                                   reinterpret_cast<void**>(&client));
    TEST(result);

    // Start the client process.
    Handle<IFile> file = nameSpace->lookup("file/locationClient.elf");
    TEST(file);
    client->start(file);

    // Wait for the client to exit
    client->wait();

    // Unregister Location factory.
    classStore->remove(CLSID_Location);

    System()->trace(false);
}
