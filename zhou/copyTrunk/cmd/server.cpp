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

class Stream : public es::Stream, public es::Binding
{
    Ref         ref;
    Object* object;

public:
    Stream() :
        object(0)
    {
    }

    ~Stream()
    {
        if (object)
        {
            object->release();
        }
    }

    long long getPosition()
    {
        return 0;
    }

    void setPosition(long long pos)
    {
    }

    long long getSize()
    {
        return 0;
    }

    void setSize(long long size)
    {
    }

    int read(void* dst, int count)
    {
        return 0;
    }

    int read(void* dst, int count, long long offset)
    {
        return 0;
    }

    int write(const void* src, int count)
    {
        if (count == 1)
        {
            char c = *(const char*) src;
            if (c == ',')
            {
                esThrow(EBUSY);
            }
        }
        return esReport("%.*s", count, src);
    }

    int write(const void* src, int count, long long offset)
    {
        return esReport("%.*s", count, src);
    }

    void flush()
    {
    }

    Object* getObject()
    {
        return object;
    }

    void setObject(Object* element)
    {
        esReport("Stream::setObject(%p)\n", element);
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

    const char* getName(void* name, int len)
    {
        if (len < 0)
        {
            return 0;
        }
        unsigned count = (len < 7) ? len : 7;
        strncpy(static_cast<char*>(name), "Stream", count);
        return static_cast<char*>(name);
    }

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else if (strcmp(riid, es::Stream::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else if (strcmp(riid, es::Binding::iid()) == 0)
        {
            objectPtr = static_cast<es::Binding*>(this);
            esReport("Stream::queryInterface: %p@%p\n", objectPtr, &objectPtr);
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
};

int main(int argc, char* argv[])
{
    esReport("Hello, world.\n");
    System()->trace(false);

    Handle<es::Context> nameSpace = System()->getRoot();

    // Create a client process.
    Handle<es::Process> client;
    client = es::Process::createInstance();
    TEST(client);

    // Set the standard output to the client process.
    Stream stream;
    stream.write("Test stream.\n", 13);
    client->setOutput(&stream);

    // Start the client process.
    Handle<es::File> file = nameSpace->lookup("file/client.elf");
    TEST(file);
    client->start(file);

    // Wait for the client to exit
    client->wait();
}
