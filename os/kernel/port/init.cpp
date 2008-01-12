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

#include <ctype.h>
#include <errno.h>
#include <es.h>
#include <es/exception.h>
#include "core.h"
#include "thread.h"

int esReport(const char* spec, ...)
{
    va_list list;
    int count;

    va_start(list, spec);
    count = esReportv(spec, list);
    va_end(list);
    return count;
}

void esDump(const void* ptr, s32 len)
{
    int j;
    int i;
    int n;

    for (j = 0; j < len; j += 16)
    {
        n = len - j;
        if (16 < n)
        {
            n = 16;
        }

        esReport("%08x: ", (const u8*) ptr + j);
        for (i = 0; i < n; i++)
        {
            esReport("%02x ", ((const u8*) ptr)[j + i]);
        }

        for (; i < 16; i++)
        {
            esReport("   ");
        }

        esReport("  ");
        for (i = 0; i < n; i++)
        {
            esReport("%c", (isprint)(((const u8*) ptr)[j + i]) ? ((const u8*) ptr)[j + i] : '.');
        }

        esReport("\n");
    }
}

#ifdef __es__

// If malloc uses synchronization primitives other than SpinLock,
// the following monitorFactory lock type must also be changed.
static SpinLock monitorFactory;

int
esOnce(esOnceControl* control, void (*func)(void))
{
    if (!control->done)
    {
        int value = 0;

        __asm__ __volatile__ (
            "xchgl  %0, %1\n"
            : "=a"(value), "=m"(control->started) : "0"(value), "m"(control->started));

        if (value == -1)
        {
            if (func)
            {
                func();
            }
            control->done = 1;
        }
        else
        {
            while (!control->done)
            {
                Thread* current(Thread::getCurrentThread());
                if (current)
                {
                    current->sleep(10000);
                }
            }
        }
    }
    return 0;
}

// Must create monitor atomically
void
esCreateMonitor(esMonitor* monitor)
{
    Thread* current(Thread::getCurrentThread());
    if (!current)
    {
        monitor->monitor = 0;
        return;
    }

    monitor->monitor = new Monitor();
}

void
esLockMonitor(esMonitor* monitor)
{
    Thread* current(Thread::getCurrentThread());
    if (!current)
    {
        monitor->monitor = 0;
        return;
    }

    if (!monitor->monitor)
    {
        SpinLock::Synchronized method(monitorFactory);
        if (monitor->monitor == 0)
        {
            monitor->monitor = new Monitor();
        }
    }

    static_cast<Monitor*>(monitor->monitor)->lock();
}

int
esTryLockMonitor(esMonitor* monitor)
{
    Thread* current(Thread::getCurrentThread());
    if (!current)
    {
        monitor->monitor = 0;
        return 0;
    }

    if (!monitor->monitor)
    {
        SpinLock::Synchronized method(monitorFactory);
        if (monitor->monitor == 0)
        {
            monitor->monitor = new Monitor();
        }
    }

    if (static_cast<Monitor*>(monitor->monitor)->tryLock())
    {
        return 0;
    }
    else
    {
        return EBUSY;
    }
}

void
esUnlockMonitor(esMonitor* monitor)
{
    Thread* current(Thread::getCurrentThread());
    if (!current)
    {
        monitor->monitor = 0;
        return;
    }

    if (!monitor->monitor)
    {
        SpinLock::Synchronized method(monitorFactory);
        if (monitor->monitor == 0)
        {
            monitor->monitor = new Monitor();
        }
    }

    static_cast<Monitor*>(monitor->monitor)->unlock();
}

void _exit(int i)
{
    Core::shutdown();
    for (;;)
    {
        __asm__ __volatile__ ("hlt");
    }
}

#endif // __es__
