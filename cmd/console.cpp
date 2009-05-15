/*
 * Copyright 2008, 2009 Google Inc.
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

#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/formatter.h>
#include <es/interlocked.h>
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
#include "canvas.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

namespace
{
    Interlocked registered = 0;

    struct CanvasInfo
    {
        int x; // top-left
        int y; // top-left
        cairo_format_t format;
        int width;
        int height;
    };
};

es::CurrentProcess* System();

class Console : public es::Stream, public es::Service
{
    static const int CR = '\r';
    static const int LF = '\n';
    static const int BS = 0x08; // backspace.

    es::Monitor* monitor;
    Ref ref;
    bool available; // indicate if this console is available.

    // keystroke buffer.
    Ring ring;

    // framebuffer
    Handle<es::Pageable> framebufferMap;
    void* framebuffer;
    // copy of the framebuffer in the main memory.
    u8* textCache;
    u8* screenCache;

    u32 framebufferSize;
    u32 lineSize;
    int bpp;            // bits per pixel
    bool updated;       // indicate if the screen is updated.
    u32 topLeftUpdated; // offset of the updated pixel in the screen cache.
    u32 bottomRightUpdated;

    // FreeType
    Handle<es::Pageable> fontMap;
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

    void addNewLine();
    u8* drawBitmapGray(FT_Bitmap* bitmap, FT_Int x, FT_Int y);
    u8* drawBitmapMono(FT_Bitmap* bitmap, FT_Int x, FT_Int y);
    u8* drawBitmap(FT_Bitmap*  bitmap, FT_Int x, FT_Int y);
    void drawCursor(Color& color, FT_Int width);
    char* getCharCode(char* data, FT_ULong& charCode);
    int getScalableParameters();
    void hideCursor();
    void initializeFramebuffer();
    void initializeFreeType(Handle<es::File> font);
    void invalidate(int topLeft, int bottomRight);

    void nextPosition(FT_Int shift)
    {
        if (currentPosition.x + shift < screenWidth)
        {
            currentPosition.x += shift;
            lineBuf->forward();
        }
    }

    void paintScreen(u8* start, int size, Color& bg);
    void prevPosition()
    {
        lineBuf->backward();
        currentPosition.x -= lineBuf->getWidth();
        if (currentPosition.x < leftMargin)
        {
            currentPosition.x = leftMargin;
        }
    }

    void refreshLine();
    void setLineHead()
    {
        currentPosition.x = leftMargin;
        lineBuf->setPosition(0);
    }

    FT_Int writeCharacter(FT_ULong charCode);

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

    inline void textOverlay24(u8* frame, u8* end, u8* text, u8* canvas)
    {
        u8 pixel[3];
        while (frame < end)
        {
            if (*text == fg.blue &&
                *(text+1) == fg.green &&
                *(text+2) == fg.red)
            {
                pixel[0] = *text++;
                pixel[1] = *text++;
                pixel[2] = *text++;
                canvas += sizeof(pixel);
            }
            else
            {
                pixel[0] = *canvas++;
                pixel[1] = *canvas++;
                pixel[2] = *canvas++;
                text += sizeof(pixel);
            }
            memmove(frame, pixel, sizeof(pixel));
            frame += sizeof(pixel);
        }
    }

    inline void textOverlay32(u8* frame, u8* frameEnd, u8* text, u8* canvas)
    {
        u32* dst = (u32*) frame;
        u32* srcText = (u32*) text;
        u32* srcCanvas = (u32*) canvas;
        u32* end = (u32*) frameEnd;
        while (dst < end)
        {
            text = (u8*) srcText;
            if (*text == fg.blue &&
                *(text+1) == fg.green &&
                *(text+2) == fg.red)
            {
                *dst++ = *srcText++;
                ++srcCanvas;
            }
            else
            {
                *dst++ = *srcCanvas++;
                ++srcText;
            }
        }
    }

    inline void refreshScreen()
    {
        updated = true;
        topLeftUpdated = 0;
        bottomRightUpdated = framebufferSize;
    }

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
    Console(Handle<es::File> font, int fontSize, u8* keyBuffer, int keyBufferSize) :
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

        // black
        fg.blue  = 0x0;
        fg.green = 0x0;
        fg.red   = 0x0;

        // white
        bg.blue  = 0xff;
        bg.green = 0xff;
        bg.red   = 0xff;

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
        delete [] textCache;
        delete [] screenCache;
        if (monitor)
        {
            monitor->release();
        }
    }

    void clear(u8* screen);
    void clearLine();
    void compose(u8* data, CanvasInfo* info);
    void displayCursor();
    void erase();
    int getBpp()
    {
        return bpp;
    }
    bool isAvailable()
    {
        Synchronized<es::Monitor*> method(monitor);
        return available;
    }
    void keyInput(char letter);
    bool suspend()
    {
        Synchronized<es::Monitor*> method(monitor);
        while (!available && registered)
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
        Synchronized<es::Monitor*> method(monitor);
        if (updated && available)
        {
            u8* frame = (u8*) framebuffer + topLeftUpdated;
            u8* frameEnd = (u8*) framebuffer + bottomRightUpdated;
            u8* text = textCache + topLeftUpdated;
            u8* canvas = screenCache + topLeftUpdated;

            switch (bpp)
            {
              case 24:
                textOverlay24(frame, frameEnd, text, canvas);
                break;

              case 32:
                textOverlay32(frame, frameEnd, text, canvas);
                break;
            }
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
        Synchronized<es::Monitor*> method(monitor);
        if (!available)
        {
            refreshScreen();
            available = true;
            monitor->notifyAll();
        }
        return true;
    }

    bool stop()
    {
        Synchronized<es::Monitor*> method(monitor);
        available = false;
        return true;
    }

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::Stream::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else if (strcmp(riid, es::Service::iid()) == 0)
        {
            objectPtr = static_cast<es::Service*>(this);
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

    unsigned int addRef()
    {
        int count = ref.addRef();
        if (count == 2)
        {
            registered = true;
        }
        return count;
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 1)
        {
            registered = false;
        }
        else if (count == 0)
        {
            available = false; // stop this service.
            monitor->notifyAll();
            delete this;
            return 0;
        }
        return count;
    }
};

int Console::
write(const void* src, int count)
{
    Synchronized<es::Monitor*> method(monitor);
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
                refreshLine();
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
    Synchronized<es::Monitor*> method(monitor);
    if (!available)
    {
        return -1;
    }

    int n;
    while (available && (n = ring.read(dst, count)) == 0)
    {
        monitor->wait();
    }
    return n;
}

u8* Console::
drawBitmapGray(FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
    const FT_Int y_max = y + bitmap->rows;
    const u8 factor = bpp/8;
    const u8* top = textCache + factor * x;
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
    const u8* topLeft = textCache + factor * x;
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
clear(u8* screen)
{
    switch (bpp)
    {
      case 24:
        paint24(screen, screen + framebufferSize, bg);
        break;
      case 32:
        paint32(screen, screen + framebufferSize, bg);
        break;
    }
}

void Console::
initializeFramebuffer()
{
    Handle<es::Context> nameSpace = System()->getRoot();

    framebufferMap = nameSpace->lookup("device/framebuffer");
    framebufferSize = framebufferMap->getSize();
    framebuffer = System()->map(0, framebufferSize,
                             es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                             es::CurrentProcess::MAP_SHARED,
                             framebufferMap, 0);
    textCache = new u8[framebufferSize];
    screenCache = new u8[framebufferSize];
    TEST(textCache && screenCache);

    bpp = 8 * (framebufferSize / (screenWidth * screenHeight));
    TEST(bpp == 24 || bpp == 32); // [check] 8 and 16bit mode are not supported.

    clear(textCache);
    clear(screenCache);

    refreshScreen();
}

void Console::
initializeFreeType(Handle<es::File> font)
{
    ASSERT(font);
    fontMap = font->getPageable();
    ASSERT(fontMap);
    long long size = font->getSize();
    fontBuffer = System()->map(0, size,
                             es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                             es::CurrentProcess::MAP_SHARED,
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

    nextScreenTop = textCache + scrollOffset + lineSize;
    newLine = textCache + scrollOffset + scrollSize;
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
    memmove(textCache + scrollOffset, nextScreenTop, scrollSize);
    paintScreen(textCache, scrollOffset, bg);
    paintScreen(newLine, lineSize, bg);
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
    refreshScreen();
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
    invalidate((bpp/8) * (x + screenWidth * y), bottomRight - textCache);

    return slot->advance.x / 64;
}

void Console::
keyInput(char letter)
{
    Synchronized<es::Monitor*> method(monitor);

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
    u8* top = textCache + y * bpp/8 * screenWidth;
    paintScreen(top, lineSize, bg);
}

void Console::
refreshLine()
{
    Synchronized<es::Monitor*> method(monitor);

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

void Console::
invalidate(int topLeft, int bottomRight)
{
    if (topLeft < topLeftUpdated)
    {
        topLeftUpdated = topLeft;
        updated = true;
    }
    if (bottomRightUpdated < bottomRight)
    {
        bottomRightUpdated = bottomRight;
        updated = true;
    }
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
    Synchronized<es::Monitor*> method(monitor);

    const u8 factor = bpp/8;
    FT_Int x = currentPosition.x;
    FT_Int yMin = currentPosition.y - ascender;

    const u8* topLeft = textCache + factor * x;
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

    invalidate((bpp/8) * (x + screenWidth * yMin), ptr - textCache);
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
    Synchronized<es::Monitor*> method(monitor);

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

void Console::
compose(u8* data, CanvasInfo* info)
{
    Synchronized<es::Monitor*> method(monitor);

    int factor = bpp / 8;
    int offset = (screenWidth * info->y + info->x) * factor;

    int len = std::min(screenWidth - info->x, info->width);
    if (len <= 0)
    {
        return;
    }

    u8* dst = static_cast<u8*>(screenCache) + offset;
    u8* end = static_cast<u8*>(screenCache) + framebufferSize;
    u8* src = data;
    u8* ptr;
    for (int y = 0; y < info->height && dst < end; ++y)
    {
        switch (info->format)
        {
        case CAIRO_FORMAT_RGB24:
            src = data;
            ptr = dst;
            for (int x = 0; x < len; ++x)
            {
                *ptr++ = *src++;
                *ptr++ = *src++;
                *ptr++ = *src++;
                ++src;
            }
            data += 4 * info->width;
            dst += factor * screenWidth;
            break;

        case CAIRO_FORMAT_ARGB32:
            memmove(dst, src, factor * len);
            src += 4 * info->width;
            dst += factor * screenWidth;
            break;

        case CAIRO_FORMAT_A8:
        case CAIRO_FORMAT_A1:
        case CAIRO_FORMAT_RGB16_565:
        default:
            return;
        }
    }

    invalidate(offset, dst - static_cast<u8*>(screenCache));
}

int main(int argc, char* argv[])
{
    // System()->trace(true);

    Handle<es::Context> nameSpace = System()->getRoot();
    Handle<es::CurrentThread> currentThread = System()->currentThread();

    // create console.
    int bufSize = 4096;
    u8* keyBuffer = new u8[bufSize];
    Handle<es::File> font = nameSpace->lookup("file/fonts/sazanami-mincho.ttf");
    Console* console = new Console(font, 12, keyBuffer, bufSize); // font size is set to 12 pt.

    // check if the event queue is ready.
    Handle<es::EventQueue> eventQueue = 0;
    while (!eventQueue)
    {
        eventQueue = nameSpace->lookup("device/event");
        currentThread->sleep(10000000 / 60);
        continue;
    }

    // register this console.
    Handle<es::Context> device = nameSpace->lookup("device");
    ASSERT(device);
    es::Binding* ret = device->bind("console", static_cast<es::Stream*>(console));
    ASSERT(ret);

    // register canvas
    cairo_surface_t* surface;
    CanvasInfo canvasInfo;
    canvasInfo.x = 0;
    canvasInfo.y = 0;
    canvasInfo.width = 1024;
    canvasInfo.height = 768;

    switch (console->getBpp())
    {
    case 24:
        canvasInfo.format = CAIRO_FORMAT_RGB24;
        break;
    case 32:
        canvasInfo.format = CAIRO_FORMAT_ARGB32;
        break;
    }

    surface = cairo_image_surface_create(canvasInfo.format, canvasInfo.width, canvasInfo.height);
    Canvas* canvas = new Canvas(surface, canvasInfo.width, canvasInfo.height);
    ASSERT(canvas);
    device->bind("canvas", static_cast<es::CanvasRenderingContext2D*>(canvas));
    ASSERT(nameSpace->lookup("device/canvas"));

    esReport("start console.\n");
    int stroke;
    char letter;
    u8* data;
    while (registered)
    {
        if (console->isAvailable())
        {
            console->displayCursor();
            if (stroke = eventQueue->getKeystroke())
            {
                letter = stroke & 0xff;
                console->keyInput(letter); // save a character into the buffer.
            }
            data = canvas->getData();
            if (data)
            {
                console->compose(data, &canvasInfo);
            }
            console->flush();
            currentThread->sleep(10000000 / 60);
        }
        else
        {
            console->suspend();
        }
    }

    canvas->release();
    console->release();
    delete [] keyBuffer;

    // System()->trace(false);
}
