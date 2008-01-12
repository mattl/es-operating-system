/*
 * Copyright (c) 2007
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

#ifndef NINTENDO_ES_SELECTOR_H_INCLUDED
#define NINTENDO_ES_SELECTOR_H_INCLUDED

#include <es.h>
#include <es/collection.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include <es/base/IProcess.h>
#include <es/base/ISelectable.h>

using namespace es;

ICurrentProcess* System();

class Selector
{
    struct Item
    {
        ISelectable*    selectable;
        void          (*handler)(void*);
        void*           param;

    };
    friend bool operator==(const Item& a, const Item& b);

    IMonitor*           monitor;
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
        Synchronized<IMonitor*> method(monitor);

        bool result = monitor->wait(timeout);
        Iterator iter = list.begin();
        while (iter.hasNext())
        {
            Item next = iter.next();
            next.handler(next.param);
        }
        return result;
    }
    int add(ISelectable* selectable, void (*handler)(void*), void* param)
    {
        Synchronized<IMonitor*> method(monitor);

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
    int remove(ISelectable* selectable)
    {
        Synchronized<IMonitor*> method(monitor);

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
