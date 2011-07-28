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

#ifndef NINTENDO_ES_KERNEL_LINE_H_INCLUDED
#define NINTENDO_ES_KERNEL_LINE_H_INCLUDED

#include <es/ref.h>
#include <es/ring.h>
#include <es/base/IStream.h>
#include <es/base/ICallback.h>
#include <es/base/IMonitor.h>
#include <es/device/IAudioFormat.h>
#include "spinlock.h"

class Line : public es::Stream, public es::AudioFormat, public es::Callback
{
    Ref         ref;

    u8          bits;
    u8          channels;
    u16         rate;

protected:
    es::Monitor*   monitor;
    es::Callback*  callback;

    Lock        spinLock;
    u8          buffer[8 * 1024];
    Ring        ring;

public:
    Line(es::Callback* callback, u8 bits = 16, u8 channels = 2, u16 rate = 44100);
    virtual ~Line();

    int invoke(int);

    u8 getBitsPerSample();
    u8 getChannels();
    u16 getSamplingRate();
    void setBitsPerSample(u8 bits);
    void setChannels(u8 channels);
    void setSamplingRate(u16 rate);

    void flush();
    long long getPosition();
    long long getSize();
    virtual int read(void* dst, int count) = 0;
    int read(void* dst, int count, long long offset);
    void setPosition(long long pos);
    void setSize(long long size);
    virtual int write(const void* src, int count) = 0;
    int write(const void* src, int count, long long offset);

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

class InputLine : public Line
{
public:
    InputLine(es::Callback* callback, u8 bits = 16, u8 channels = 2, u16 rate = 44100);
    virtual ~InputLine();
    int read(void* dst, int count);
    int write(const void* src, int count);
};

class OutputLine : public Line
{
public:
    OutputLine(es::Callback* callback, u8 bits = 16, u8 channels = 2, u16 rate = 44100);
    virtual ~OutputLine();
    int read(void* dst, int count);
    int write(const void* src, int count);
};

#endif  // NINTENDO_ES_KERNEL_LINE_H_INCLUDED
