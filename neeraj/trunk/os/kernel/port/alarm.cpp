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

#include <es.h>
#include <es/dateTime.h>
#include "alarm.h"

#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LLu
#endif

Lock            Alarm::spinLock;
Alarm::Queue    Alarm::queues[2];

Alarm::
Alarm() :
    ref(1),
    callback(0),
    flags(Enabled),
    interval(0),
    start(0),
    nextTick(0),
    current(0)
{
}

Alarm::
~Alarm()
{
    if (current)
    {
        Lock::Synchronized method(spinLock);

        cancel();
    }
    if (callback)
    {
        callback->release();
        callback = 0;
    }
}

bool Alarm::
getPeriodic()
{
    return getFlag(Periodic);
}

bool Alarm::
getEnabled()
{
    return getFlag(Enabled);
}

bool Alarm::
isExpired(long long now)
{
    return (now - nextTick < 0) ? false : true;
}

void Alarm::
setCallback(es::Callback* callback)
{
    es::Callback* prev = 0;

    if (callback)
    {
        callback->addRef();
    }
    {
        Lock::Synchronized method(spinLock);

        prev = this->callback;
        this->callback = callback;
    }
    if (prev)
    {
        prev->release();
    }
}

void Alarm::
setPeriodic(bool periodic)
{
    Lock::Synchronized method(spinLock);

    periodic ? setFlag(Periodic) : clearFlag(Periodic);
}

void Alarm::
setEnabled(bool enabled)
{
    Lock::Synchronized method(spinLock);

    if (enabled)
    {
        cancel();
        setFlag(Enabled);
        set(this);
    }
    else
    {
        clearFlag(Enabled);
        cancel();
    }
}

long long Alarm::
getInterval()
{
    return this->interval;
}

void Alarm::
setInterval(long long interval)
{
    if (0 <= interval)
    {
        Lock::Synchronized method(spinLock);

        cancel();
        this->interval = interval;
        set(this);
    }
}

long long Alarm::
getStartTime()
{
    return this->start;
}

void Alarm::
setStartTime(long long time)
{
    Lock::Synchronized method(spinLock);

    cancel();
    this->start = time;
    set(this);
}

void Alarm::
cancel()
{
    if (current)
    {
        current->remove(this);
        current = 0;
    }
    nextTick = 0;
}

Object* Alarm::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Alarm::iid()) == 0)
    {
        objectPtr = static_cast<es::Alarm*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Alarm::
addRef()
{
    return ref.addRef();
}

unsigned int Alarm::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

void Alarm::
set(Alarm* alarm)
{
    long long start;

    start = alarm->getStartTime();
    if (0 < start)
    {
        queues[0].add(alarm);
    }
    else
    {
        queues[1].add(alarm);
    }
}

void Alarm::
update()
{
    long long now;
    long long delta;
    long long delta2;

    now = DateTime::getNow().getTicks();
    delta = queues[0].getDelta(now);
    now = DateTime::getNow().getTicks();    // XXX use getMonotonicTime()
    delta2 = queues[1].getDelta(now);
    if (delta2 < delta)
    {
        delta = delta2;
    }
    // XXX set timer interrupt here if necessary
}

void Alarm::
invoke()
{
    queues[0].process(DateTime::getNow().getTicks());
    queues[1].process(DateTime::getNow().getTicks());   // XXX use getMonotonicTime()
}

bool Alarm::
check()
{
    return queues[0].check() && queues[1].check();
}

void Alarm::
Queue::process(long long ticks)
{
    Alarm* alarm;

    do
    {
        {
            Lock::Synchronized method(spinLock);
            if ((alarm = queue.getFirst()))
            {
                if (!alarm->isExpired(ticks))
                {
                    alarm = 0;
                }
                else
                {
                    alarm = queue.removeFirst();
                    alarm->current = 0;
                    if (alarm->getPeriodic() && 0 < alarm->interval)
                    {
                        add(alarm);
                    }
                    else
                    {
                        alarm->nextTick = 0;
                    }
                }
            }
        }
        if (alarm && alarm->callback)
        {
            alarm->callback->invoke(0);
        }
    } while (alarm);

    {
        Lock::Synchronized method(spinLock);
        if (!queue.isEmpty())
        {
            update();
        }
    }
}

void Alarm::
Queue::add(Alarm* alarm)
{
    if (!alarm ||
        !alarm->getEnabled() ||
        alarm->interval <= 0)
    {
        return;
    }

    ASSERT(!alarm->current);
    alarm->current = this;

    // Calculate the next expiration tick.
    long long now;
    if (alarm->start <= 0)
    {
        now = DateTime::getNow().getTicks();    // XXX use getMonotonicTime()
        if (alarm->nextTick == 0)
        {
            // Set epoch to the current monotonic time.
            alarm->nextTick = now + alarm->interval;
        }
    }
    else
    {
        now = DateTime::getNow().getTicks();
        alarm->nextTick = alarm->start + alarm->interval;
    }
    if (alarm->getPeriodic())
    {
        if (0 <= now - alarm->nextTick)
        {
            alarm->nextTick += alarm->interval * ((now - alarm->nextTick) / alarm->interval + 1);
        }
    }

    Alarm* next;
    List<Alarm, &Alarm::link>::Iterator iter = queue.begin();
    while ((next = iter.next()))
    {
        if (next->nextTick <= alarm->nextTick)
        {
            continue;
        }
        iter.previous();    // Insert alarm before next
        break;
    }
    iter.add(alarm);
    if (queue.getFirst() == alarm)
    {
        update();
    }
}

void Alarm::
Queue::remove(Alarm* alarm)
{
    Alarm* first = queue.getFirst();
    queue.remove(alarm);
    if (alarm == first)
    {
        update();
    }
}

long long Alarm::
Queue::getDelta(long long now)
{
    Alarm* alarm = queue.getFirst();
    return alarm ? (alarm->nextTick - now) : LLONG_MAX;
}

bool Alarm::
Queue::check()
{
    Lock::Synchronized method(spinLock);

    Alarm* next;
    List<Alarm, &Alarm::link>::Iterator iter = queue.begin();
    while ((next = iter.next()))
    {
        ASSERT((void*) 0x80000000 <= next);
    }
    return true;
}

es::Alarm* Alarm::
Constructor::createInstance()
{
    return new Alarm;
}

Object* Alarm::
Constructor::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Alarm::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::Alarm::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Alarm::Constructor*>(this);
    }
    else
    {
        return 0;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Alarm::
Constructor::addRef()
{
    return 1;
}

unsigned int Alarm::
Constructor::release()
{
    return 1;
}

void Alarm::
initializeConstructor()
{
    static Constructor constructor;
    es::Alarm::setConstructor(&constructor);
}
