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

#include <string.h>
#include <es.h>
#include <es/endian.h>
#include <es/naming/IContext.h>
#include "vesa.h"
#include "cache.h"

// #define VERBOSE

#define MIN(a, b)   (((a) <= (b)) ? (a) : (b))
#define MAX(a, b)   (((b) <= (a)) ? (a) : (b))

u32 Vesa::data[32] =
{
    0xc0000000, // 1100000000000000
    0xa0000000, // 1010000000000000
    0x90000000, // 1001000000000000
    0x88000000, // 1000100000000000
    0x84000000, // 1000010000000000
    0x82000000, // 1000001000000000
    0x81000000, // 1000000100000000
    0x80800000, // 1000000010000000
    0x83c00000, // 1000001111000000
    0xb2000000, // 1011001000000000
    0xd2000000, // 1101001000000000
    0x89000000, // 1000100100000000
    0x09000000, // 0000100100000000
    0x04800000, // 0000010010000000
    0x04800000, // 0000010010000000
    0x03000000, // 0000001100000000
};

u32 Vesa::mask[32] =
{
    0xc0000000, // 1100000000000000
    0xe0000000, // 1110000000000000
    0xf0000000, // 1111000000000000
    0xf8000000, // 1111100000000000
    0xfc000000, // 1111110000000000
    0xfe000000, // 1111111000000000
    0xff000000, // 1111111100000000
    0xff800000, // 1111111110000000
    0xffc00000, // 1111111111000000
    0xbe000000, // 1011111000000000
    0xde000000, // 1101111000000000
    0x8f000000, // 1000111100000000
    0x0f000000, // 0000111100000000
    0x07800000, // 0000011110000000
    0x07800000, // 0000011110000000
    0x03000000, // 0000001100000000
};

u16 Vesa::xHotSpot(0);
u16 Vesa::yHotSpot(0);

using namespace LittleEndian;

Vesa::
Vesa(u8* vbeInfoBlock, u8* modeInfoBlock, u8* font, IContext* device) :
    vbeInfoBlock(vbeInfoBlock),
    modeInfoBlock(modeInfoBlock),
    font(font)
{
    xResolution = word(modeInfoBlock + XResolution);
    yResolution = word(modeInfoBlock + YResolution);
    bitsPerPixel = byte(modeInfoBlock + BitsPerPixel);
    redFieldPosition = byte(modeInfoBlock + RedFieldPosition);
    greenFieldPosition = byte(modeInfoBlock + GreenFieldPosition);
    blueFieldPosition = byte(modeInfoBlock + BlueFieldPosition);
    physBasePtr = (u8*) dword(modeInfoBlock + PhysBasePtr);
    memset(physBasePtr, 0xff, xResolution * yResolution * (bitsPerPixel / 8));
    size = xResolution * yResolution * bitsPerPixel / 8;

    xPosition = xResolution / 2;
    yPosition = yResolution / 2;

#ifdef VERBOSE
    esReport("VbeInfoBlock\n");
    esDump(vbeInfoBlock, 512);

    u16* modes = (u16*) (word(vbeInfoBlock + VideoModePtr) |
                         (word(vbeInfoBlock + VideoModePtr + 2) << 4));
    while (*modes != 0xffff)
    {
        esReport("%x\n", *modes++);
    }

    esReport("ModeInfoBlock\n");
    esDump(modeInfoBlock, 256);

    esReport("%dx%d %d bit color [%d:%d:%d] 0x%p 0x%p\n",
             xResolution,
             yResolution,
             bitsPerPixel,
             redFieldPosition,
             greenFieldPosition,
             blueFieldPosition,
             physBasePtr,
             font);
#endif // VERBOSE

    device->bind("framebuffer", static_cast<IStream*>(this));
    device->bind("cursor", static_cast<ICursor*>(this));
}

int Vesa::
show()
{
    int show = count.increment();
    if (show == 1)
    {
        saveBackground();
        drawCursor();
    }
    return show;
}

int Vesa::
hide()
{
    int show = count.decrement();
    if (show == 0)
    {
        restoreBackground();
    }
    return show;
}

void Vesa::
move(int dx, int dy)
{
    setPosition(dx + xPosition, dy + yPosition);
}

void Vesa::
getPosition(int& x, int& y)
{
    x = xPosition;
    y = yPosition;
}

void Vesa::
setPosition(int x, int y)
{
    if (x == xPosition && y == yPosition)
    {
        return;
    }

    if (0 < count)
    {
        restoreBackground();
    }

    if (x < 0)
    {
        x = 0;
    }
    else if (xResolution <= x)
    {
        x = xResolution - 1;
    }
    xPosition = (u16) x;

    if (y < 0)
    {
        y = 0;
    }
    else if (yResolution <= y)
    {
        y = yResolution - 1;
    }
    yPosition = (u16) y;

    if (0 < count)
    {
        saveBackground();
        drawCursor();
    }
}

void Vesa::
setPattern(u32 data[32], u32 mask[32], u16 xHotSpot, u16 yHotSpot)
{
    if (0 < count)
    {
        restoreBackground();
    }
    memmove(this->data, data, sizeof this->data);
    memmove(this->mask, mask, sizeof this->mask);
    this->xHotSpot = xHotSpot;
    this->yHotSpot = yHotSpot;
    if (0 < count)
    {
        drawCursor();
    }
}

void Vesa::
saveBackground()
{
    int x;
    int y;
    int len;

    len = 32;
    x = xPosition - xHotSpot;
    if (xResolution < x + len)
    {
        len = xResolution - x;
    }
    if (x < 0)
    {
        len -= -x;
        x = 0;
    }
    ASSERT(0 < len && len <= 32);
    y = yPosition - yHotSpot;
    int i = 0;
    if (y < 0)
    {
        i = -y;
        y = 0;
    }
    len *= (bitsPerPixel / 8);
    for (; i < 32 && y < yResolution; ++i, ++y)
    {
        memmove(background[i], physBasePtr + (xResolution * y + x) * (bitsPerPixel / 8), len);
    }
}

void Vesa::
restoreBackground()
{
    int x;
    int y;
    int len;

    len = 32;
    x = xPosition - xHotSpot;
    if (xResolution < x + len)
    {
        len = xResolution - x;
    }
    if (x < 0)
    {
        len -= -x;
        x = 0;
    }
    ASSERT(0 < len && len <= 32);
    y = yPosition - yHotSpot;
    int i = 0;
    if (y < 0)
    {
        i = -y;
        y = 0;
    }
    len *= (bitsPerPixel / 8);
    for (; i < 32 && y < yResolution; ++i, ++y)
    {
        memmove(physBasePtr + (xResolution * y + x) * (bitsPerPixel / 8), background[i], len);
    }
}

void Vesa::
drawCursor()
{
    int x;
    int y;
    int len;
    int offset;

    len = 32;
    offset = 0;
    x = xPosition - xHotSpot;
    if (xResolution < x + len)
    {
        len = xResolution - x;
    }
    if (x < 0)
    {
        offset = x;
        len -= -x;
        x = 0;
    }
    ASSERT(0 < len && len <= 32);
    y = yPosition - yHotSpot;
    int i = 0;
    if (y < 0)
    {
        i = -y;
        y = 0;
    }
    for (; i < 32 && y < yResolution; ++i, ++y)
    {
        u8* ptr = physBasePtr + (xResolution * y + x) * (bitsPerPixel / 8);
        u32 d = data[i];
        u32 m = mask[i];
        for (int j = offset; j < offset + len; ++j, ptr += (bitsPerPixel / 8))
        {
            u32 bit = 0x80000000 >> j;
            u8 r, g, b;
            if ((m & bit) && (d & bit))
            {
                // opaque black
                r = g = b = 0x00;
            }
            else if (!(m & bit) && (d & bit))
            {
                r = 0xff - ptr[redFieldPosition / 8];
                g = 0xff - ptr[greenFieldPosition / 8];
                b = 0xff - ptr[blueFieldPosition / 8];
            }
            else if ((m & bit) && !(d & bit))
            {
                // opaque white
                r = g = b = 0xff;
            }
            else
            {
                continue;
            }
            ptr[redFieldPosition / 8] = r;
            ptr[greenFieldPosition / 8] = g;
            ptr[blueFieldPosition / 8] = b;
        }
    }
}

long long Vesa::
getPosition()
{
    return 0;
}

void Vesa::
setPosition(long long pos)
{
}

long long Vesa::
getSize()
{
    return this->size;
}

void Vesa::
setSize(long long size)
{
}

int Vesa::
read(void* dst, int count)
{
}

int Vesa::
read(void* dst, int count, long long offset)
{
    if (offset < 0)
    {
        count -= offset;
        offset = 0;
    }
    if (size <= offset + count)
    {
        count = size - offset;
    }
    if (count <= 0)
    {
        return 0;
    }
    ASSERT(0 <= offset && offset < size);
    if (0 < this->count)
    {
        restoreBackground();
        memmove(dst, physBasePtr + offset, count);
        drawCursor();
    }
    else
    {
        memmove(dst, physBasePtr + offset, count);
    }
    return count;
}

int Vesa::
write(const void* src, int count)
{
}

int Vesa::
write(const void* src, int count, long long offset)
{
    if (offset < 0)
    {
        count -= offset;
        offset = 0;
    }
    if (size <= offset + count)
    {
        count = size - offset;
    }
    if (count <= 0)
    {
        return 0;
    }
    ASSERT(0 <= offset && offset < size);
    if (0 < this->count)
    {
        restoreBackground();
        memmove(physBasePtr + offset, src, count);
        saveBackground();
        drawCursor();
    }
    else
    {
        memmove(physBasePtr + offset, src, count);
    }
    return count;
}

void Vesa::
flush()
{
}

unsigned long long Vesa::
get(long long offset)
{
    if (offset < 0 || size <= offset)
    {
        return 0;
    }
    offset = Page::pageBase(offset);
    return reinterpret_cast<unsigned long>(physBasePtr + offset) |
           Page::PTEVALID | Page::PTETHROUGH | Page::PTEUNCACHED;
}

void Vesa::
put(long long offset, unsigned long long pte)
{
}

bool Vesa::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_ICursor)
    {
        *objectPtr = static_cast<ICursor*>(this);
    }
    else if (riid == IID_IStream)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<ICursor*>(this);
    }
    else if (riid == IID_IPageable)
    {
        *objectPtr = static_cast<IPageable*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Vesa::
addRef(void)
{
    return ref.addRef();
}

unsigned int Vesa::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
