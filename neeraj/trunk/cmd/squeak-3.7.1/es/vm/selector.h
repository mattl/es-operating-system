/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_SELECTOR_H_INCLUDED
#define NINTENDO_ES_SELECTOR_H_INCLUDED

#include <es.h>
#include <es/collection.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include <es/base/IProcess.h>
#include <es/base/ISelectable.h>



es::CurrentProcess* System();

class Selector
{
    struct Item
    {
        es::Selectable*    selectable;
        void          (*handler)(void*);
        void*           param;

    };
    friend bool operator==(const Item& a, const Item& b);

    es::Monitor*           monitor;
    Collection<Item>    list;

public:

    typedef Collection<Item>::Iterator  Iterator;

    Selector() : monitor(0)
    {
        monitor = System()->createMonitor();
    }
    ~Selector()
    {
        if (monitor)
        {
            monitor->release();
        }
    }
    bool wait(long long timeout = 0)
    {
        Synchronized<es::Monitor*> method(monitor);

        bool result = monitor->wait(timeout);
        Iterator iter = list.begin();
        while (iter.hasNext())
        {
            Item next = iter.next();
            next.handler(next.param);
        }
        return result;
    }
    int add(es::Selectable* selectable, void (*handler)(void*), void* param)
    {
        Synchronized<es::Monitor*> method(monitor);

esReport("Selector::%s %d, %p\n", __func__, __LINE__, selectable);
        if (selectable)
        {
            Item item = { selectable, handler, param };

            int result = selectable->add(monitor);
            if (0 <= result)
            {
                list.addLast(item);
            }
            handler(param);
            return result;
        }
        return 0;
    }
    int remove(es::Selectable* selectable)
    {
        Synchronized<es::Monitor*> method(monitor);

esReport("Selector::%s %d, %p\n", __func__, __LINE__, selectable);
        if (selectable)
        {
            Item item = { selectable, 0, 0 };

            int result = selectable->remove(monitor);
            if (0 <= result)
            {
                list.remove(item);
            }
            return result;
        }
        return 0;
    }
};

bool operator==(const Selector::Item& a, const Selector::Item& b)
{
    return a.selectable == b.selectable;
}

#endif // NINTENDO_ES_SELECTOR_H_INCLUDED
