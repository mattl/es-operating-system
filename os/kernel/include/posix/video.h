/*
 * Copyright 2008 Chis Dan Ionut
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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

#ifndef GOOGLE_ES_KERNEL_POSIX_VIDEO_H_INCLUDED
#define GOOGLE_ES_KERNEL_POSIX_VIDEO_H_INCLUDED

#include <es/base/IStream.h>
#include <es/base/IPageable.h>
#include <es/device/ICursor.h>
#include <es/naming/IContext.h>
#include <es/types.h>
#include <X11/Xlib.h>
#include "posix/core.h"

class VideoBuffer : public IStream, public ICursor, public IPageable 
{
    u16 xResolution;
    u16 yResolution;
    u8 bitsPerPixel;
    long size;
  
    //mouse cursor
    static u32 data[32];
    static u32 mask[32];
    u16 xPosition;
    u16 yPosition;
    int showCursor;
    char background[32][32 * 4];
    
    bool isInitialized; 
    void initX11();
    void drawCursor();
    void saveBackground();
    void restoreBackground();
    
public:
    VideoBuffer(IContext* device);
    ~VideoBuffer();
    
    //IStream
    long long getPosition();
    void setPosition(long long position);
    
    long long getSize();
    void setSize(long long size);
    
    int read(void* buf, int bufLength);
    int read(void* buf, int bufLength, long long offset);
    
    int write(const void* src, int srcLength);
    int write(const void* src, int srcLength, long long offset);
    
    void flush();
    
    //IPageable
    unsigned long long get(long long offset);
    void put(long long offset, unsigned long long pte);
    
    //ICursor
    int show();
    int hide();
    void move(int dx, int dy);
    void getPosition(int* x, int* y);
    void setPosition(int x, int y);
    void setPattern(const u32 pattern[32], const u32 mask[32], unsigned short xHotSpot, unsigned short yHotSpot);
  
    //IInterface
    void* queryInterface(const Guid& riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // GOOGLE_ES_KERNEL_POSIX_VIDEO_H_INCLUDED
