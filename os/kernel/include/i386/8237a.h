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

#ifndef NINTENDO_ES_KERNEL_I386_8237A_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_8237A_H_INCLUDED

#include <es/ref.h>
#include <es/types.h>
#include <es/device/IDmac.h>
#include "thread.h"

class Dmac
{
    static const u8 ADDR = 0;
    static const u8 COUNT = 1;
    static const u8 COMMAND = 8;
    static const u8 REQUEST = 9;
    static const u8 SINGLE_MASK = 10;
    static const u8 MODE = 11;
    static const u8 CLEAR_BYTE_POINTER = 12;
    static const u8 MASTER_CLEAR = 13;
    static const u8 CLEAR_MASK = 14;
    static const u8 ALL_MASK = 15;
    static const u8 pageOffset[4];

    Ref     ref;
    Lock    spinLock;
    u8      base;
    u8      page;
    int     shift;

public:
    class Chan : public es::Dmac
    {
        ::Dmac* dmac;
        u8      chan;

    public:
        // es::Dmac
        void setup(const void* addr, int count, u8 mode);
        void start();
        void stop();
        bool isDone();
        int getCount();

        // Object
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();

        friend class ::Dmac;
    };


public:
    Dmac(u8 base, u8 page, int shift);
    void setup(u8 chan, u32 buffer, int len, u8 mode);

    Chan chan[4];

    friend class Chan;
};

#endif // NINTENDO_ES_KERNEL_I386_8237A_H_INCLUDED
