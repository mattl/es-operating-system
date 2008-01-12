/*
 * Copyright (c) 2006, 2007
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

#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/formatter.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/ring.h>
#include <es/synchronized.h>
#include <es/types.h>
#include <es/usage.h>
#include <es/utf.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/base/IService.h>
#include "IEventQueue.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

class Console : public IStream, public IService
{
    static const int CR = '\r';
    static const int LF = '\n';
    static const int BS = 0x08; // backspace.

    IMonitor* monitor;
    Ref ref;
    bool available; // indicate if this console is available.

    // keystroke buffer.
    Ring ring;

    // framebuffer
    Handle<IPageable> framebufferMap;
    void* framebuffer;
    u8* screenCache; // copy of the framebuffer in the main memory.
    u32 framebufferSize;
    u32 lineSize;
    int bpp;           // bits per pixel
    bool updated;     // indicate if the screen is updated.
    u32 topLeftUpdated; // offset of the updated pixel in the screen cache.
    u32 bottomRightUpdated;

    // FreeType
    Handle<IPageable> fontMap;
    void* fontBuffer;
    FT_Library library;
    FT_Face    face;
    FT_Vector  currentPosition;

    // layout
    int screenWidth;
    int screenHeight;
    int dpi;
    int fontSize;
    int topMargin;
    int leftMargin;
    int lineHeight;
    int baselineSkip;

    FT_Int ascender;
    FT_Int descender;

    // scroll
    u32 scrollOffset;
    u32 screenSize;
    u32 scrollSize;
    u8* nextScreenTop;
    u8* newLine;

    /*
     * Left of the screen.
     * |
     * V
     * +--------------------------------- Top of the screen
     * |        A
     * |        | topMargin
     * |        V
     * |- - - - - - - - - - - - - - A A - -
     * |       ***                  | |
     * |      *   *                 | | ascender
     * |      *   *      lineHeight | V
     * |- - -  **** - - - - - - - - | A - - - - - - - -
     * |          *                 | | |descender|
     * |- - - - - * - - - - - - - - V V
     * |<---->
     * |leftMargin
     */

    struct Color
    {
        u8  red;
        u8  blue;
        u8  green;
    };
    Color fg; // foreground
    Color bg; // background

    // cursor parameters.
    bool cursor;
    FT_Int cursorWidth;
    int cursorBlinking;
    int blinkingInterval;

    // line buffer
    struct CharInfo
    {
        FT_ULong charCode;
        FT_Int   width;
    };

    class LineBuffer
    {
        CharInfo* list;
        int last;
        int pos;
    public:
        LineBuffer(int numChar) : last(0), pos(0)
        {
            list = new CharInfo[numChar];
        }

        FT_ULong add(FT_ULong charCode, FT_Int width)
        {
            list[pos].charCode = charCode;
            list[pos].width = width;
        }

        FT_ULong getCharCode()
        {
            return list[pos].charCode;
        }

        FT_Int getWidth()
        {
            return list[pos].width;
        }

        bool isLast()
        {
            return pos == last;
        }

        int getPosition()
        {
            return pos;
        }

        void setPosition(int pos)
        {
            this->pos = pos;
        }

        void forward()
        {
            ++pos;
            if (last < pos)
            {
                last = pos;
            }
        }

        void backward()
        {
            if (0 < pos)
            {
                --pos;
            }
        }

        void reset()
        {
            last = 0;
        }
    };
    LineBuffer* lineBuf; // keep information of the characters in the current line.

    void checkScreenUpdate(FT_Int x, FT_Int y, u8* tail);
    u8* drawBitmapGray(FT_Bitmap* bitmap, FT_Int x, FT_Int y);
    u8* drawBitmapMono(FT_Bitmap* bitmap, FT_Int x, FT_Int y);
    u8* drawBitmap(FT_Bitmap*  bitmap, FT_Int x, FT_Int y);
    void drawCursor(Color& color, FT_Int width);
    char* getCharCode(char* data, FT_ULong& charCode);
    int getScalableParameters();
    void hideCursor();
    void initializeFramebuffer();
    void initializeFreeType(Handle<IFile> font);
    void addNewLine();
    void paintScreen(u8* start, int size, Color& bg);
    FT_Int writeCharacter(FT_ULong charCode);

    void setLineHead()
    {
        currentPosition.x = leftMargin;
        lineBuf->setPosition(0);
    }

    void nextPosition(FT_Int shift)
    {
        if (currentPosition.x + shift < screenWidth)
        {
            currentPosition.x += shift;
            lineBuf->forward();
        }
    }

    void prevPosition()
    {
        lineBuf->backward();
        currentPosition.x -= lineBuf->getWidth();
        if (currentPosition.x < leftMargin)
        {
            currentPosition.x = leftMargin;
        }
    }

    inline void paint24(u8* start, u8* end, Color& color) // for 24-bit.
    {
        while (start < end)
        {
            *start++ = color.blue;
            *start++ = color.green;
            *start++ = color.red;
        }
    };

    inline void paint32(u8* start, u8* end, Color& color) // for 32-bit.
    {
        while (start < end)
        {
            *start++ = color.blue;
            *start++ = color.green;
            *start++ = color.red;
            ++start;
        }
    };

    inline int roundUp64(int x)
    {
        if (0 <= x)
        {
            return (((u32)(x) + 64 - 1) & ~(64 - 1));
        }
        else
        {
            return -(((u32)(-x) + 64 - 1) & ~(64 - 1));
        }
    }

public:
    Console(Handle<IFile> font, int fontSize, u8* keyBuffer, int keyBufferSize) :
        fontSize(fontSize), leftMargin(10), topMargin(5), screenWidth(1024), screenHeight(768),
            dpi(72), baselineSkip(0), ring(keyBuffer, keyBufferSize),
                cursorBlinking(0), cursor(true), blinkingInterval(30), cursorWidth(fontSize/2),
                  available(true), updated(false)
    {
        if (!font || fontSize <= 0 || !keyBuffer || keyBufferSize <= 0)
        {
            esReport("Console(): Could not initialize console.\n");
            return;
        }
        monitor = System()->createMonitor();

        // white
        fg.blue  = 0xff;
        fg.green = 0xff;
        fg.red   = 0xff;

        // black
        bg.blue  = 0x0;
        bg.green = 0x0;
        bg.red   = 0x0;

        initializeFramebuffer();
        initializeFreeType(font);
    }

    ~Console()
    {
        FT_Done_Face(face);
        FT_Done_FreeType(library);

        System()->unmap(fontBuffer, fontMap->getSize());
        System()->unmap(framebuffer, framebufferSize);

        delete [] lineBuf;
        delete [] screenCache;
        if (monitor)
        {
            monitor->release();
        }
    }

    void clearLine();
    void displayCursor();
    void erase();
    bool isAvailable()
    {
        Synchronized<IMonitor*> method(monitor);
        return available;
    }
    void keyInput(char letter);
    void refleshLine();
    bool suspend()
    {
        Synchronized<IMonitor*> method(monitor);
        while (!available)
        {
            monitor->wait();
        }
    }

    //
    // IStream
    //
    long long getPosition()
    {
        return -1;
    }

    void setPosition(long long pos)
    {
    }

    long long getSize()
    {
        return -1;
    }

    void setSize(long long size)
    {
    }

    int read(void* dst, int count);

    int read(void* dst, int count, long long offset)
    {
        return read(dst, count);
    }

    int write(const void* src, int count);

    int write(const void* src, int count, long long offset)
    {
        return write(src, count);
    }

    void flush()
    {
        Synchronized<IMonitor*> method(monitor);
        if (updated && available)
        {
            ASSERT(topLeftUpdated < bottomRightUpdated);
            memmove((u8*) framebuffer + topLeftUpdated, screenCache + topLeftUpdated, bottomRightUpdated - topLeftUpdated);
            updated = false;
            topLeftUpdated = framebufferSize;
            bottomRightUpdated = 0;
        }
    }

    //
    // IService
    //
    bool start()
    {
        Synchronized<IMonitor*> method(monitor);
        if (!available)
        {
            // refresh screen.
            updated = true;
            topLeftUpdated = 0;
            bottomRightUpdated = framebufferSize;

            available = true;
            monitor->notifyAll();
        }
        return true;
    }

    bool stop()
    {
        Synchronized<IMonitor*> method(monitor);
        available = false;
        return true;
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IStream)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IID_IService)
        {
            *objectPtr = static_cast<IService*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
    }

    unsigned int addRef(void)
    {
        return ref.addRef();
    }

    unsigned int release(void)
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

int Console::
write(const void* src, int count)
{
    Synchronized<IMonitor*> method(monitor);
    if (!available)
    {
        return -1;
    }

    FT_Error      error;
    char* data = (char*) src;
    char* end = data + count;

    error = FT_Set_Char_Size(face, fontSize * 64, 0, dpi, 0 );
    if (error != 0)
    {
        return -1;
    }

    FT_Int shift;
    FT_ULong charCode;
    while (data && data < end)
    {
        data = getCharCode(data, charCode);
        switch (charCode)
        {
          case CR:
            hideCursor();
            setLineHead();
            break;

          case LF:
            hideCursor();
            addNewLine();
            break;

          case BS:
            hideCursor();
            prevPosition();
            break;

          default:
            erase();
            shift = writeCharacter(charCode);
#if 1
            // adjust space.
            lineBuf->add(charCode, shift);
            if (!lineBuf->isLast())
            {
                refleshLine();
            }
#else
            if (!lineBuf->isLast())
            {
                shift = lineBuf->getWidth();
            }
            lineBuf->add(charCode, shift);
#endif
            nextPosition(shift);
            break;
        }
    }

    cursor = &fg;
    drawCursor(fg, cursorWidth);
    return count;
}

int Console::
read(void* dst, int count)
{
    Synchronized<IMonitor*> method(monitor);
    if (!available)
    {
        return -1;
    }

    int n;
    while ((n = ring.read(dst, count)) == 0)
    {
        monitor->wait();
    }
    return n;
}

void Console::
checkScreenUpdate(FT_Int topLeftX, FT_Int topLeftY, u8* bottomRight)
{
    // bottomRight is the pointer in the screen cache.
    // It points the pixel at the bottom-right of the updated screen area.
    // topLeftX, Y are the coordinates of the top-left of the updated screen area.
    if (bottomRight && bottomRightUpdated < bottomRight - screenCache)
    {
        bottomRightUpdated = bottomRight - screenCache;
        u32 topLeft = (bpp/8) * (topLeftX + screenWidth * topLeftY);
        if (topLeft < topLeftUpdated)
        {
            topLeftUpdated = topLeft;
        }
        updated = true;
        ASSERT(topLeftUpdated < bottomRightUpdated);
    }
}

u8* Console::
drawBitmapGray(FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
    const FT_Int y_max = y + bitmap->rows;
    const u8 factor = bpp/8;
    const u8* top = screenCache + factor * x;
    FT_Int index;
    FT_Int i;
    FT_Int j = 0;

    u8* ptr = 0;
    for (; y < y_max; ++y)
    {
        ptr = const_cast<u8*>(top + factor * screenWidth * y);
        index = j++ * bitmap->width;
        for (i = 0; i < bitmap->width; ++i, ++index)
        {
            if (screenWidth <= i + x || screenHeight <= y)
            {
                ptr += factor;
                continue;
            }
            switch (bpp)
            {
              case 24:
                *ptr++ = (bitmap->buffer[index] * fg.blue) >> 8;
                *ptr++ = (bitmap->buffer[index] * fg.green) >> 8;
                *ptr++ = (bitmap->buffer[index] * fg.red) >> 8;
                break;
              case 32:
                *ptr++ = (bitmap->buffer[index] * fg.blue) >> 8;
                *ptr++ = (bitmap->buffer[index] * fg.green) >> 8;
                *ptr++ = (bitmap->buffer[index] * fg.red) >> 8;
                ++ptr;
                break;
            }
        }
    }

    return ptr;
}

u8* Console::
drawBitmapMono(FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
    const u8 factor = bpp/8;
    const u8* topLeft = screenCache + factor * x;
    FT_Int  i;
    FT_Int  j;
    u8 pixelR;
    u8 pixelG;
    u8 pixelB;
    FT_Int  byte;
    FT_Int  bit;
    FT_Int  padding = bitmap->pitch * 8 - bitmap->width;
    FT_Int  offset;
    FT_Int  padded;

    u8* ptr = 0;
    for (j = 0; j < bitmap->rows; ++j)
    {
        ptr = const_cast<u8*>(topLeft + factor * screenWidth * (j + y));
        offset = j * bitmap->width;
        for (i = 0; i < bitmap->width; ++i, ++offset)
        {
            if (screenWidth <= i + x || screenHeight <= j + y)
            {
                ptr += factor;
                continue;
            }

            padded = offset + (offset / bitmap->width) * padding;
            byte = padded / 8;
            bit = 7 - padded % 8;
            if (bitmap->buffer[byte] & (1<<bit))
            {
                pixelB = fg.blue;
                pixelG = fg.green;
                pixelR = fg.red;
            }
            else
            {
                pixelB = bg.blue;
                pixelG = bg.green;
                pixelR = bg.red;
            }
            switch (bpp)
            {
              case 24:
                *ptr++ = pixelB;
                *ptr++ = pixelG;
                *ptr++ = pixelR;
                break;
              case 32:
                *ptr++ = pixelB;
                *ptr++ = pixelG;
                *ptr++ = pixelR;
                ++ptr;
                break;
            }
        }
    }
    return ptr;
}

u8* Console::
drawBitmap(FT_Bitmap*  bitmap, FT_Int x, FT_Int y)
{
    u8* bottomRightPixel = 0;
    switch (bitmap->pixel_mode)
    {
      case FT_PIXEL_MODE_NONE:
      case FT_PIXEL_MODE_GRAY2:
      case FT_PIXEL_MODE_GRAY4:
      case FT_PIXEL_MODE_LCD:
      case FT_PIXEL_MODE_LCD_V:
        esReport("*** pixel mode not supported. ***\n");
        break;
      case FT_PIXEL_MODE_MONO:
        bottomRightPixel = drawBitmapMono(bitmap, x, y);
        break;
      case FT_PIXEL_MODE_GRAY:
        bottomRightPixel = drawBitmapGray(bitmap, x, y);
        break;
    }

    return bottomRightPixel;
}

char* Console::
getCharCode(char* data, FT_ULong& charCode)
{
    if (*data & (1<<7))
    {
        u32 utf32;
        char* next = utf8to32(data, &utf32);
        charCode = utf32;
        return next;
    }
    else
    {
        charCode = *data; // ASCII
        return ++data;
    }
}

int Console::
getScalableParameters()
{
    FT_Set_Char_Size(face, fontSize * 64, 0, dpi, 0);
    FT_Error error = FT_Load_Char(face, ' ', FT_LOAD_RENDER); // get scalable parameters.
    ASSERT(error == 0);
    if (error != 0)
    {
        return -1;
    }

    FT_Size_Metrics* scaled = &face->size->metrics;
    ASSERT(0 < scaled->ascender);
    ascender = roundUp64(scaled->ascender) / 64;
    descender = roundUp64(scaled->descender) / 64;
    lineHeight = roundUp64(scaled->height) / 64;

    if (ascender <= 0 || 0 < descender || lineHeight <= 0)
    {
        return -1;
    }
    return 0;
}

void Console::
initializeFramebuffer()
{
    Handle<IContext> nameSpace = System()->getRoot();

    framebufferMap = nameSpace->lookup("device/framebuffer");
    framebufferSize = framebufferMap->getSize();
    framebuffer = System()->map(0, framebufferSize,
                             ICurrentProcess::PROT_READ | ICurrentProcess::PROT_WRITE,
                             ICurrentProcess::MAP_SHARED,
                             framebufferMap, 0);
    screenCache = new u8[framebufferSize];
    TEST(screenCache);

    bpp = 8 * (framebufferSize / (screenWidth * screenHeight));
    TEST(bpp == 24 || bpp == 32); // [check] 8 and 16bit mode are not supported.
    switch (bpp)
    {
      case 24:
        paint24(screenCache, screenCache + framebufferSize, bg);
        break;
      case 32:
        paint32(screenCache, screenCache + framebufferSize, bg);
        break;
    }

    updated = true;
    topLeftUpdated = 0;
    bottomRightUpdated = framebufferSize;
}

void Console::
initializeFreeType(Handle<IFile> font)
{
    fontMap = font->getPageable();
    long long size = font->getSize();
    fontBuffer = System()->map(0, size,
                             ICurrentProcess::PROT_READ | ICurrentProcess::PROT_WRITE,
                             ICurrentProcess::MAP_SHARED,
                             fontMap, 0);
    TEST(fontBuffer);

    FT_Error error = FT_Init_FreeType( &library );
    ASSERT(error == 0);
    error = FT_New_Memory_Face(library, static_cast<u8*>(fontBuffer), size, 0, &face);
    ASSERT(error == 0);

    int ret = -1;
    if (FT_IS_SCALABLE(face))
    {
        ret = getScalableParameters();
    }
    if (ret < 0)
    {
        ascender = roundUp64(face->ascender) / 64;
        descender = roundUp64(face->descender) / 64;
        lineHeight = roundUp64(face->height) / 64;
    }
    if (lineHeight < ascender - descender)
    {
        lineHeight = ascender - descender;
    }
    ASSERT (0 < ascender && descender <= 0 && 0 < lineHeight);

    lineSize = (bpp/8) * screenWidth * (lineHeight + baselineSkip);
    currentPosition.x = leftMargin;
    currentPosition.y = topMargin + lineHeight + descender;
    lineBuf = new LineBuffer(screenWidth); // [check] how to get the maximum number of characters in a line?

    // scroll parameters
    screenSize = bpp/8 * screenHeight * screenWidth;
    scrollOffset = bpp/8 * topMargin * screenWidth;

    u32 bottomMargin = (screenHeight - 1 - topMargin) % (lineHeight + baselineSkip);
    u32 bottomMarginSize = bpp/8 * bottomMargin * screenWidth;
    scrollSize = screenSize - scrollOffset - lineSize - bottomMarginSize;

    nextScreenTop = screenCache + scrollOffset + lineSize;
    newLine = screenCache + scrollOffset + scrollSize;
}

void Console::
addNewLine()
{
    setLineHead();
    lineBuf->reset();
    if (currentPosition.y + baselineSkip + lineHeight - descender < screenHeight)
    {
        currentPosition.y += lineHeight + baselineSkip;
        return;
    }

    // scroll screen.
    memmove(screenCache + scrollOffset, nextScreenTop, scrollSize);
    paintScreen(screenCache, scrollOffset, bg);
    paintScreen(newLine, lineSize, bg);
    updated = true;
    topLeftUpdated = 0;
    bottomRightUpdated = framebufferSize;
}

void Console::
paintScreen(u8* start, int size, Color& bg)
{
    switch (bpp)
    {
      case 24:
        paint24(start, start + size, bg);
        break;
      case 32:
        paint32(start, start + size, bg);
        break;
    }
    updated = true;
    topLeftUpdated = 0;
    bottomRightUpdated = framebufferSize;
}

FT_Int Console::
writeCharacter(FT_ULong charCode)
{
    FT_Error error = FT_Load_Char(face, charCode, FT_LOAD_RENDER);
    if (error)
    {
        esReport("FT_Load_Char error\n");
        return 0;
    }

    /*       slot->bitmap_left
     *       <--->
     * - - - - - - - - - top of the line.
     *
     *        ***       A
     *       *   *      | slot->bitmap_top
     *       *   *      V
     *       O**** <------ O is the current position.
     *           *
     *           *
     * - - - - - - - - - bottom of the line.
     *
     *
     */
    FT_GlyphSlot slot = face->glyph;
    FT_Int x = currentPosition.x + slot->bitmap_left;
    FT_Int y = currentPosition.y - slot->bitmap_top;

    u8* bottomRight = drawBitmap(&slot->bitmap, x, y);
    checkScreenUpdate(x, y, bottomRight);

    return slot->advance.x / 64;
}

void Console::
keyInput(char letter)
{
    Synchronized<IMonitor*> method(monitor);

    if (0 < ring.getUnused())
    {
        ring.write(&letter, 1);
    }
    monitor->notifyAll();
}

void Console::
clearLine()
{
    FT_Int y = currentPosition.y - ascender;
    if (y < 0)
    {
        lineSize += y;
        y = 0;
    }
    u8* top = screenCache + y * bpp/8 * screenWidth;
    paintScreen(top, lineSize, bg);
}

void Console::
refleshLine()
{
    Synchronized<IMonitor*> method(monitor);

    clearLine();
    int pos = lineBuf->getPosition();
    setLineHead();

    FT_Int restore = leftMargin;
    int x = 0;
    while (!lineBuf->isLast())
    {
        erase();
        FT_ULong charCode =lineBuf->getCharCode();
        FT_Int shift = writeCharacter(charCode);
        lineBuf->add(charCode, shift);
        if (x++ == pos)
        {
            restore = currentPosition.x;
        }
        nextPosition(shift);
    }
    lineBuf->setPosition(pos);
    currentPosition.x = restore;
}

/*
 *   Cursor
 *   +-----+  A <---- yMin
 *   |/////|  |
 *   |/////|  | ascender
 *   |/////|  |
 *   |/////|  V
 * - - - - - - - - - - - - - - - - - - - baseline
 *   |/////|  A
 *   |/////|  | |descender|
 *   +-----+  V
 *   <----->
 *    width
 */
void Console::
drawCursor(Color& color, FT_Int width)
{
    Synchronized<IMonitor*> method(monitor);

    const u8 factor = bpp/8;
    FT_Int x = currentPosition.x;
    FT_Int yMin = currentPosition.y - ascender;

    const u8* topLeft = screenCache + factor * x;
    u8* ptr = 0;
    FT_Int i;
    FT_Int y;
    for (y = yMin; y <= currentPosition.y - descender; ++y)
    {
        if (y < 0)
        {
            continue;
        }
        ptr = const_cast<u8*>(topLeft + factor * screenWidth * y);
        for (i = 0; i < width; ++i)
        {
            if (screenWidth <= i + x || screenHeight <= y)
            {
                continue;
            }

            switch (bpp)
            {
              case 24:
                *ptr++ = color.blue;
                *ptr++ = color.green;
                *ptr++ = color.red;
                break;
              case 32:
                *ptr++ = color.blue;
                *ptr++ = color.green;
                *ptr++ = color.red;
                ++ptr;
                break;
            }
        }
    }

    checkScreenUpdate(x, yMin, ptr);
}

void Console::
hideCursor()
{
    drawCursor(bg, cursorWidth);
    if (!lineBuf->isLast())
    {
        writeCharacter(lineBuf->getCharCode()); // redraw the character.
    }
}

void Console::
displayCursor()
{
    Synchronized<IMonitor*> method(monitor);

    if (++cursorBlinking < blinkingInterval)
    {
        return;
    }
    cursorBlinking = 0;
    if (cursor)
    {
        drawCursor(fg, cursorWidth);
    }
    else
    {
        hideCursor();
    }
    cursor ^= 1;
}

void Console::
erase()
{
    if (lineBuf->isLast())
    {
        drawCursor(bg, cursorWidth);
    }
    else
    {
        drawCursor(bg, lineBuf->getWidth());
    }
}

int main(int argc, char* argv[])
{
    // System()->trace(true);

    Handle<IContext> nameSpace = System()->getRoot();
    Handle<ICurrentThread> currentThread = System()->currentThread();

    // create console.
    int bufSize = 4096;
    u8* keyBuffer = new u8[bufSize];
    Handle<IFile> font = nameSpace->lookup("file/sazanami-mincho.ttf");
    Console console(font, 12, keyBuffer, bufSize); // font size is set to 12 pt.

    // check if the event queue is ready.
    Handle<IEventQueue> eventQueue = 0;
    while (!eventQueue)
    {
        eventQueue = nameSpace->lookup("device/event");
        currentThread->sleep(10000000 / 60);
        continue;
    }

    // register this console.
    Handle<IContext> device = nameSpace->lookup("device");
    ASSERT(device);
    IBinding* ret = device->bind("console", static_cast<IStream*>(&console));
    ASSERT(ret);

    esReport("start console.\n");
    int stroke;
    char letter;
    for (;;)
    {
        while (console.isAvailable())
        {
            console.displayCursor();
            if (eventQueue->getKeystroke(&stroke))
            {
                letter = stroke & 0xff;
                console.keyInput(letter); // save a character into the buffer.
            }
            console.flush();
            currentThread->sleep(10000000 / 60);
        }
        console.suspend();
    }

    // [check] how to stop this console?

    delete [] keyBuffer;

    // System()->trace(false);
}
