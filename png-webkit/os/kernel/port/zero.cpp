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

#include <es.h>
#include "zero.h"

Zero::Zero()
{
}

Zero::~Zero()
{
}

long long Zero::getPosition()
{
    return 0;
}

void Zero::setPosition(long long pos)
{
}

long long Zero::getSize()
{
    return 9223372036854775807LL;   // 2^63-1
}

void Zero::setSize(long long size)
{
}

int Zero::read(void* dst, int count)
{
    return 0;
}

int Zero::read(void* dst, int count, long long offset)
{
    // Page::fill() will zero clear the page.
    return 0;
}

int Zero::write(const void* src, int count)
{
    return 0;
}

int Zero::write(const void* src, int count, long long offset)
{
    return 0;
}

void Zero::flush()
{
}

Object* Zero::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Zero::addRef()
{
    return ref.addRef();
}

unsigned int Zero::release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
