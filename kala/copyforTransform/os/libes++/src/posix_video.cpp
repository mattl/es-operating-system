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

#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>

#include <es.h>
#include <es/naming/IContext.h>

#include "core.h"
#include "posix_system.h"
#include "posix_video.h"

namespace es
{

namespace posix
{

// #define VERBOSE

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

u16 VideoBuffer::xHotSpot(0);
u16 VideoBuffer::yHotSpot(0);

VideoBuffer::
VideoBuffer(es::Context* device) :
    Stream(-1)
{
    monitor = esCreateMonitor();

    xResolution = 1024;
    yResolution = 768;
    bitsPerPixel = 32;
    redFieldPosition = 16;
    greenFieldPosition = 8;
    blueFieldPosition = 0;
    base = 0;
    size = xResolution * yResolution * bitsPerPixel / 8;

    xPosition = xResolution / 2;
    yPosition = yResolution / 2;

    device->bind("framebuffer", static_cast<es::Stream*>(this));
    device->bind("cursor", static_cast<es::Cursor*>(this));
}

VideoBuffer::
~VideoBuffer()
{
    if (monitor)
    {
        monitor->release();
    }
}

int VideoBuffer::
show()
{
    Synchronized<es::Monitor*> method(monitor);

    int show = count.increment();
    if (show == 1)
    {
        saveBackground();
        drawCursor();
    }
    return show;
}

int VideoBuffer::
hide()
{
    Synchronized<es::Monitor*> method(monitor);

    int show = count.decrement();
    if (show == 0)
    {
        restoreBackground();
    }
    return show;
}

void VideoBuffer::
move(int dx, int dy)
{
    setPosition(xPosition + dx, yPosition + dy);
}

void VideoBuffer::
getPosition(int* x, int* y)
{
    Synchronized<es::Monitor*> method(monitor);

    *x = xPosition;
    *y = yPosition;
}

void VideoBuffer::
setPosition(int x, int y)
{
    Synchronized<es::Monitor*> method(monitor);

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

void VideoBuffer::
setPattern(const unsigned int data[32], const unsigned int mask[32], u16 xHotSpot, u16 yHotSpot)
{
    Synchronized<es::Monitor*> method(monitor);

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
        saveBackground();
        drawCursor();
    }
}

void VideoBuffer::
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
        memmove(background[i], base + (xResolution * y + x) * (bitsPerPixel / 8), len);
    }
}

void VideoBuffer::
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
        memmove(base + (xResolution * y + x) * (bitsPerPixel / 8), background[i], len);
    }
}

void VideoBuffer::
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
        offset = -x;
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
        u8* ptr = base + (xResolution * y + x) * (bitsPerPixel / 8);
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

long long VideoBuffer::
getPosition()
{
    return 0;
}

void VideoBuffer::
setPosition(long long position)
{
}

long long VideoBuffer::
getSize()
{
    return size;
}

void VideoBuffer::
setSize(long long Size)
{
}

int VideoBuffer::
read(void* dst, int count)
{
    return -1;
}

int VideoBuffer::
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
        memmove(dst, base + offset, count);
        drawCursor();
    }
    else
    {
        memmove(dst, base + offset, count);
    }
    return count;
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
        memmove(base + offset, src, count);
        saveBackground();
        drawCursor();
    }
    else
    {
        memmove(base + offset, src, count);
    }
    return count;
}

void VideoBuffer::
flush()
{
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

Object* VideoBuffer::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Cursor::iid()) == 0)
    {
        objectPtr = static_cast<es::Cursor*>(this);
    }
    else if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Cursor*>(this);
    }
    else if (strcmp(riid, es::Pageable::iid()) == 0)
    {
        objectPtr = static_cast<es::Pageable*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();

    if (getfd() == -1)
    {
        init();
    }

    return objectPtr;
}

unsigned int VideoBuffer::
addRef()
{
    return Stream::addRef();
}

unsigned int VideoBuffer::
release()
{
    return Stream::release();
}

namespace
{
    VideoBuffer* vb;

    void idle(void)
    {
        glutPostRedisplay();
    }

    void resize(int w, int h)
    {
        glViewport(0, 0, w, h);
        glLoadIdentity();
    }
}

void VideoBuffer::draw()
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xResolution, yResolution, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, base);

    glEnable(GL_TEXTURE_2D);

    glNormal3d(0.0, 0.0, 1.0);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 1.0);
    glVertex3d(-1.0, -1.0,  0.0);
    glTexCoord2d(1.0, 1.0);
    glVertex3d( 1.0, -1.0,  0.0);
    glTexCoord2d(1.0, 0.0);
    glVertex3d( 1.0,  1.0,  0.0);
    glTexCoord2d(0.0, 0.0);
    glVertex3d(-1.0,  1.0,  0.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void VideoBuffer::
display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    vb->draw();
    glutSwapBuffers();
}

void* VideoBuffer::
start(void* param)
{
    char* argv[] = { "" };
    int argc = 0;

    vb = static_cast<VideoBuffer*>(param);

    glutInitWindowSize(vb->xResolution, vb->yResolution);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutCreateWindow("ES Operating System");
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutIdleFunc(idle);
    // glutMouseFunc(mouse);
    // glutMotionFunc(motion);
    // glutKeyboardFunc(keyboard);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vb->xResolution, vb->yResolution, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, vb->base);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glClearColor(1.0, 1.0, 1.0, 0.0);

    glutMainLoop();
}

void VideoBuffer::
init()
{
    int fd = shm_open("/es-frame-buffer", O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    base = static_cast<u8*>(mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (base == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    shm_unlink("/es-frame-buffer");
    memset(base, 0, size);

    setfd(fd);

    es::Thread* thread = esCreateThread(start, this);
    thread->start();
}

}   // namespace posix

}   // namespace es
