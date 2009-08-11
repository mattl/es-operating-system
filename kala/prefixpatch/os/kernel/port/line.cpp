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
#include <es/object.h>
#include <es/exception.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include "line.h"

Line::
Line(es::Callback* callback, u8 bits, u8 channels, u16 rate) :
    callback(callback),
    bits(bits),
    channels(channels),
    rate(rate),
    ring(buffer, sizeof buffer)
{
    monitor = es::Monitor::createInstance();
}

Line::
~Line()
{
    if (monitor)
    {
        monitor->release();
    }
}

u8 Line::
getBitsPerSample()
{
    return bits;
}

u8 Line::
getChannels()
{
    return channels;
}

u16 Line::
getSamplingRate()
{
    return rate;
}

void Line::
setBitsPerSample(u8 bits)
{
    this->bits = bits;
}

void Line::
setChannels(u8 channels)
{
    this->channels = channels;
}

void Line::
setSamplingRate(u16 rate)
{
    this->rate = rate;
}

int Line::
invoke(int)
{
    monitor->notify();  // XXX Fix timing issue
    return 0;
}

void Line::
flush()
{
}

long long Line::
getPosition()
{
    return 0;
}

long long Line::
getSize()
{
    return 0;
}

int Line::
read(void* dst, int count, long long offset)
{
    esThrow(EACCES);
}

void Line::
setPosition(long long pos)
{
}

void Line::
setSize(long long size)
{
}

int Line::
write(const void* src, int count, long long offset)
{
    esThrow(EACCES);
}

Object* Line::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, es::AudioFormat::iid()) == 0)
    {
        objectPtr = static_cast<es::AudioFormat*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Line::
addRef()
{
    return ref.addRef();
}

unsigned int Line::
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

InputLine::
InputLine(es::Callback* callback, u8 bits, u8 channels, u16 rate) :
    Line(callback, bits, channels, rate)
{
}

InputLine::
~InputLine()
{
}

int InputLine::
read(void* dst, int count)
{
    callback->invoke(0);
    {
        Synchronized<es::Monitor*> method(monitor);

        if (count <= 0)
        {
            return 0;
        }

        while (ring.getUsed() == 0)
        {
            monitor->wait(10000);
        }
    }
    Lock::Synchronized method(spinLock);
    return ring.read(dst, count);
}

int InputLine::
write(const void* src, int count)
{
    Lock::Synchronized method(spinLock);
    count = ring.write(src, count);
    invoke(count);
    return count;
}

OutputLine::
OutputLine(es::Callback* callback, u8 bits, u8 channels, u16 rate) :
    Line(callback, bits, channels, rate)
{
}

OutputLine::
~OutputLine()
{
}

int OutputLine::
read(void* dst, int count)
{
    Lock::Synchronized method(spinLock);
    count = ring.read(dst, count);
    invoke(count);
    return count;
}

int OutputLine::
write(const void* src, int count)
{
    {
        Synchronized<es::Monitor*> method(monitor);

        if (count <= 0)
        {
            return 0;
        }

        while (ring.getUsed() == sizeof buffer)
        {
            monitor->wait(10000);
        }
    }
    callback->invoke(1);
    Lock::Synchronized method(spinLock);
    return ring.write(src, count);
}
