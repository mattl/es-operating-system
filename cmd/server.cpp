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
#include <string.h>
#include <es.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include <es/naming/IBinding.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

using namespace es;

ICurrentProcess* System();
Handle<IClassStore> classStore;

void* createInstance(const Guid& rclsid, const Guid& riid)
{
    return classStore->createInstance(rclsid, riid);
}

class Stream : public IStream, public IBinding
{
    Ref         ref;
    IInterface* object;

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

    IInterface* getObject()
    {
        return object;
    }

    void setObject(IInterface* element)
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

    int getName(char* name, int len)
    {
        ASSERT(0 <= len);
        unsigned count = (len < 7) ? len : 7;
        strncpy(name, "Stream", count);
        return count;
    }

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IInterface::iid())
        {
            objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IStream::iid())
        {
            objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IBinding::iid())
        {
            objectPtr = static_cast<IBinding*>(this);
            esReport("Stream::queryInterface: %p@%p\n", objectPtr, &objectPtr);
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

int main(int argc, char* argv[])
{
    esReport("Hello, world.\n");
    System()->trace(false);

    Handle<IContext> nameSpace = System()->getRoot();
    classStore = nameSpace->lookup("class");
    TEST(classStore);

    // Create a client process.
    Handle<IProcess> client;
    client = reinterpret_cast<IProcess*>(createInstance(CLSID_Process, IProcess::iid()));
    TEST(client);

    // Set the standard output to the client process.
    Stream stream;
    stream.write("Test stream.\n", 13);
    client->setOutput(&stream);

    // Start the client process.
    Handle<IFile> file = nameSpace->lookup("file/client.elf");
    TEST(file);
    client->start(file);

    // Wait for the client to exit
    client->wait();
}
