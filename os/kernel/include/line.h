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

#ifndef NINTENDO_ES_KERNEL_LINE_H_INCLUDED
#define NINTENDO_ES_KERNEL_LINE_H_INCLUDED

#include <es/ref.h>
#include <es/ring.h>
#include <es/base/IStream.h>
#include <es/base/ICallback.h>
#include <es/base/IMonitor.h>
#include <es/device/IAudioFormat.h>
#include "spinlock.h"

class Line : public IStream, public IAudioFormat, public ICallback
{
    Ref         ref;

    u8          bits;
    u8          channels;
    u16         rate;

protected:
    IMonitor*   monitor;
    ICallback*  callback;

    Lock        spinLock;
    u8          buffer[8 * 1024];
    Ring        ring;

public:
    Line(ICallback* callback, u8 bits = 16, u8 channels = 2, u8 rate = 44100);
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

    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);
};

class InputLine : public Line
{
public:
    InputLine(ICallback* callback, u8 bits = 16, u8 channels = 2, u8 rate = 44100);
    virtual ~InputLine();
    int read(void* dst, int count);
    int write(const void* src, int count);
};

class OutputLine : public Line
{
public:
    OutputLine(ICallback* callback, u8 bits = 16, u8 channels = 2, u8 rate = 44100);
    virtual ~OutputLine();
    int read(void* dst, int count);
    int write(const void* src, int count);
};

#endif  // NINTENDO_ES_KERNEL_LINE_H_INCLUDED
