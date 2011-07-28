/*
 * Copyright 2008, 2009 Google Inc.
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

#include <es/ref.h>
#include <es/base/IMonitor.h>

class NullMonitor : public es::IMonitor
{
    Ref             ref;

public:
    NullMonitor()
    {
    }

    ~NullMonitor()
    {
    }

    // IMonitor
    void lock()
    {
    }

    bool tryLock()
    {
        return true;
    }

    void unlock()
    {
    }

    bool wait()
    {
        return false;
    }

    bool wait(s64 timeout)
    {
        return false;
    }

    void notify()
    {
    }

    void notifyAll()
    {
    }

    // IInterface
    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (strcmp(riid, es::IMonitor::iid()) == 0)
        {
            objectPtr = static_cast<es::IMonitor*>(this);
        }
        else if (strcmp(riid, es::IInterface::iid()) == 0)
        {
            objectPtr = static_cast<es::IMonitor*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<es::IInterface*>(objectPtr)->addRef();
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
