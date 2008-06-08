/*
 * Copyright 2008 Chis Dan Ionut
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
 *
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
#include <es/naming/IContext.h>
#include "posix/video.h"
#include <string.h>
#include <stdlib.h>


// #define VERBOSE

#define BYTES_PER_PIXEL (bitsPerPixel / 8)
static Display* display;
static int screen;
static Window window;
static GC gc;
static Visual* visual;
static XImage* xi;
static char* buffer;

u32 VideoBuffer::data[32] =
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

u32 VideoBuffer::mask[32] =
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

VideoBuffer::
VideoBuffer(IContext* device)
{
    device->bind("framebuffer", static_cast<IStream*>(this));
    device->bind("cursor", static_cast<ICursor*>(this));
    
    xResolution = 1024;
    yResolution = 768;
    bitsPerPixel = 32;
    
    size = xResolution * yResolution * (bitsPerPixel / 8);
    xPosition = xResolution / 2;
    yPosition = yResolution / 2;
    showCursor = 0;
    isInitialized = false; 
}

VideoBuffer::
~VideoBuffer()
{ 
}

long long VideoBuffer::
getPosition()
{
     return -1;
}

void VideoBuffer::
setPosition(long long position)
{
}

long long VideoBuffer::
getSize()
{
     return this->size;
}

void VideoBuffer::
setSize(long long Size)
{
}

int VideoBuffer::
read(void* buf, int bufLength)
{
    return 0;
}

int VideoBuffer::
read(void* buf, int bufLength, long long offset)
{
    return -1;
}

int VideoBuffer::
write(const void* src, int count)
{
    return -1;
}

int VideoBuffer::
write(const void* src, int count, long long offset)
{
    if (offset < 0)
    {
        count = -offset;
        offset = 0;
    }

    if (size <= offset + count)
    {
        count = size - offset - 1;
    }
  
    if (count <= 0)
    {
        return 0;
    }

    ASSERT(0 <= offset && offset < size);

    char* source = (char*) src;
    int line, col;
    int cnt = count;

    line = offset / (xResolution * (bitsPerPixel / 8));
    col = offset % (xResolution * (bitsPerPixel / 8));
 
#ifdef VERBOSE
    esReport("line=%d col=%d count=%d\n",line, col, count);
#endif //VERBOSE

    while (cnt > 0)
      {
        if (col == xResolution * 4)
        {
            col = 0;
            line += 1;
        }

        buffer[line * xi->bytes_per_line + col] = *source;
        
        col++;
        source++;
        cnt--;
    }
    XPutImage(display, window, gc, xi, 0, 0, 0, 0, xResolution, yResolution);
    
    // If cursor is active, draw it over
    if (showCursor)
    {
      drawCursor();
    }
    return count;
}

void VideoBuffer::
flush()
{
    XFlush(display);
}

unsigned long long VideoBuffer::
get(long long offset)
{
    return 0;
}

void VideoBuffer::
put(long long offset, unsigned long long pte)
{
}

int VideoBuffer::
show()
{
    if (!showCursor)
    {
        showCursor = 1;
        saveBackground();
        drawCursor();
    }
    return showCursor;;
}

int VideoBuffer::
hide()
{
    if (showCursor)
    {
        showCursor = 0;
        //restore the image under the cursor and draw
        restoreBackground();
        XPutImage(display, window, gc, xi, 0, 0, 0, 0, xResolution, yResolution);
    }
    return showCursor;
}

void VideoBuffer::
move(int dx, int dy)
{
    setPosition(xPosition + dx, yPosition + dy);
}

void VideoBuffer::
getPosition(int* x, int* y)
{
    *x = xPosition;
    *y = yPosition;
}

void VideoBuffer::
setPosition(int x, int y)
{
    if (x == xPosition && y == yPosition)
    {  
        return;
    }

    if (showCursor)
    {
        restoreBackground();
    }
    if (x < 0)
    {
        x = 0;
    }
    else if (xResolution <= x)
    {
        x = xResolution -1;
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

    if (showCursor)
    {
        saveBackground();
        drawCursor();
    }
}

void VideoBuffer::
setPattern(const u32 data[32], const u32 mask[32], u16 xHotSpot, u16 yHotSpot)
{
}

void* VideoBuffer::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == ICursor::iid())
    {
        objectPtr = static_cast<ICursor*>(this);
    }
    else if (riid == IStream::iid())
    {
        objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<ICursor*>(this);
    }
    else if (riid == IPageable::iid())
    {
        objectPtr = static_cast<IPageable*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();

    if (!isInitialized)
    {
        initX11(); 
        isInitialized = true; 
    }
    return objectPtr;
}

unsigned int VideoBuffer::
addRef()
{
    return 0;
}

unsigned int VideoBuffer::
release()
{
    return 0;
}

void VideoBuffer::
initX11()
{
    // Open a display
    display = XOpenDisplay(0);
    ASSERT(display != NULL);
    
    screen = DefaultScreen(display);
    visual = DefaultVisual(display, screen);
    int blackColor = BlackPixel(display, DefaultScreen(display));
    
    // Create a Window
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 
                                 xResolution, yResolution, 0, blackColor, blackColor);
    XSelectInput(display, window, StructureNotifyMask);
    XMapWindow(display, window);
    gc = XCreateGC(display, window, 0, 0);
    
    // Create and image as the window's content
    // FIXME : for some reason setting the depth to 32 in the call to XCreateImage
    //         breaks the application; depth is explicitly set in the returned struct 
    xi = XCreateImage(display, visual, 24, ZPixmap, 0, 0,
                      xResolution, yResolution, 32, 0);
  
    buffer = (char*) malloc(xi->bytes_per_line * yResolution);
    // Explicitly set image depth (bits per pixel)
    xi->bits_per_pixel = bitsPerPixel; 
    XInitImage(xi);
    xi->data = buffer;
    
    for (;;) 
    {  
        XEvent e;
        XNextEvent(display, &e);
        if (e.type == MapNotify)
            break;
    }
}  

void VideoBuffer::
drawCursor()
{
    int x, y;
    int len;
    x = xPosition;
    y = yPosition;
    len = 32;
  
    if (xResolution <= x + len)
    {
        len = xResolution - x - 1;
    }
  
    for (int i = 0; i < 32 && y < yResolution; i++, y++)
    {
        for (int j = 0; j < len; j++)
        {
            u32 m = mask[i];
            u32 d = data[i];
            u32 bit = 0x80000000 >> j;
            char r, g, b;
        
            if ((m & bit) && (d & bit))
            {
                r = g = b = 0; // black
            }
            else if (!(m & bit) && (d & bit))
            {
                r = g = b = 0; //black
            }
            else if ((m & bit) && !(d & bit))
            {
                r = g = b = 255; //white
            }
            else
            {
                continue;
            }
        buffer[y * xi->bytes_per_line + (x + j) * BYTES_PER_PIXEL] = b;
        buffer[y * xi->bytes_per_line + (x + j) * BYTES_PER_PIXEL + 1] = g;
        buffer[y * xi->bytes_per_line + (x + j) * BYTES_PER_PIXEL + 2] = r;
        }
    }
    XPutImage(display, window, gc, xi, 0, 0, 0, 0, xResolution, yResolution);
}

void VideoBuffer::
saveBackground()
{
    int x, y;
    int len = 32;
  
    x = xPosition;
    y = yPosition;
    if (xResolution <= x + len)
        len = xResolution - x - 1;

    len *= BYTES_PER_PIXEL;
    for(int i = 0; i < 32 && y < yResolution; i++, y++)
    {
        memmove(background[i], buffer + y * xi->bytes_per_line + x * BYTES_PER_PIXEL, len);
    }
}

void VideoBuffer::
restoreBackground()
{
    int x, y;
    int len = 32;

    x = xPosition;
    y = yPosition;
    if (xResolution <= x + len)
    {
      len = xResolution - x - 1;
    }
  
    len *= BYTES_PER_PIXEL;
    for (int i = 0; i < 32 && y < yResolution; i++, y++)
    {
        memmove(buffer + y * xi->bytes_per_line + x * BYTES_PER_PIXEL, background[i], len);
    }
}

