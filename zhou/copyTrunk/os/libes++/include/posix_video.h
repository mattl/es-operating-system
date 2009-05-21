/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2008 Chis Dan Ionut
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

#ifndef GOOGLE_ES_LIBES_POSIX_VIDEO_H_INCLUDED
#define GOOGLE_ES_LIBES_POSIX_VIDEO_H_INCLUDED

#include <es/interlocked.h>
#include <es/ref.h>
#include <es/types.h>
#include <es/base/IMonitor.h>
#include <es/base/IStream.h>
#include <es/base/IPageable.h>
#include <es/device/ICursor.h>
#include <es/naming/IContext.h>
#include "posix_system.h"

namespace es
{

namespace posix
{

class VideoBuffer : public es::Cursor, public Stream
{
    u16     xResolution;
    u16     yResolution;
    u8      bitsPerPixel;
    u8      redFieldPosition;
    u8      greenFieldPosition;
    u8      blueFieldPosition;
    u8*     base;
    long    size;

    // mouse cursor
    es::Monitor*    monitor;
    Interlocked     count;
    static u32      data[32];
    static u32      mask[32];
    static u16      xHotSpot;
    static u16      yHotSpot;
    u16             xPosition;
    u16             yPosition;
    u32             background[32][32];

    void init();

    void drawCursor();
    void saveBackground();
    void restoreBackground();

    void draw();

public:
    VideoBuffer(es::Context* device);
    ~VideoBuffer();

    // IStream
    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    // ICursor
    int show();
    int hide();
    void move(int dx, int dy);
    void getPosition(int* x, int* y);
    void setPosition(int x, int y);
    void setPattern(const unsigned int data[32], const unsigned int mask[32], u16 xHotSpot, u16 yHotSpot);

    // IPageable
    unsigned long long get(long long offset);
    void put(long long offset, unsigned long long pte);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    static void* start(void* param);
    static void display();
};

}   // namespace posix

}   // namespace es

#endif // GOOGLE_ES_LIBES_POSIX_VIDEO_H_INCLUDED
