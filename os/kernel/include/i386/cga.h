/*
 * Copyright 2011 Esrille Inc.
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

#ifndef NINTENDO_ES_KERNEL_I386_CGA_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_CGA_H_INCLUDED

#include <es/types.h>
#include <es/ref.h>
#include <es/base/IStream.h>



class Cga : public es::Stream
{
    static const u32 BASE = 0x800b8000;

    static const int CTL_ADDR = 0x3d4;      // Address register
    static const int CTL_DATA = 0x3d5;      // Data IO register
    static const u8 CURSOR_ADDR_HIGH = 0xe; // High byte of cursor address
    static const u8 CURSOR_ADDR_LOW = 0xf;  // Low  byte of cursor address

    Ref     ref;
    char*   cga;

    void putChar(int c);

public:
    Cga();

    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // NINTENDO_ES_KERNEL_I386_CGA_H_INCLUDED
