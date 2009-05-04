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

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from Squeak.
 *
 *   Squeak is distributed for use and modification subject to a liberal
 *   open source license.
 *
 *   http://www.squeak.org/SqueakLicense/
 *
 *   Unless stated to the contrary, works submitted for incorporation
 *   into or for distribution with Squeak shall be presumed subject to
 *   the same license.
 *
 *   Portions of Squeak are:
 *
 *   Copyright (c) 1996 Apple Computer, Inc.
 *   Copyright (c) 1997-2001 Walt Disney Company, and/or
 *   Copyrighted works of other contributors.
 *   All rights reserved.
 */

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/ring.h>
#include <es/synchronized.h>
#include <es/usage.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/device/ICursor.h>
#include <es/naming/IContext.h>
#include "../IEventQueue.h"



es::CurrentProcess* System();

#define WIDTH   1024
#define HEIGHT  768

/****************************************************************************/
/*              Keyboard and Mouse                                          */
/****************************************************************************/

extern "C"
{
    #include "sq.h"

    /* Set the 16x16 cursor bitmap. If cursorMaskIndex is nil, then make the mask the same as
       the cursor bitmap. If not, then mask and cursor bits combined determine how cursor is
       displayed:
            mask    cursor  effect
             0        0     transparent (underlying pixel shows through)
             1        1     opaque black
             1        0     opaque white
             0        1     invert the underlying pixel
    */
    struct Cursor
    {
        unsigned data[32];
        unsigned mask[32];
        long     hotSpot;
    };

    int inputSemaphoreIndex = 0;/* if non-zero the event semaphore index */

    /*** Variables -- Imported from Virtual Machine ***/
    extern int interruptPending;
    extern int interruptCheckCounter;
    extern int interruptKeycode;
    extern int fullScreenFlag;
    extern int deferDisplayUpdates; /* Is the Interpreter doing defered updates for us?! */

    int synchronizedSignalSemaphoreWithIndex(int semaIndex);
}

/*** Variables -- Event Recording ***/

Handle<es::EventQueue> gEventQueue __attribute__ ((init_priority (1001)));

Cursor mouseCursor;

Handle<es::Pageable> framebuffer;
Handle<es::Cursor> cursor;
int bpp;
void* framebufferPtr;

/*** Synchronization functions ***/

int synchronizedSignalSemaphoreWithIndex(int semaIndex)
{
    ASSERT(gEventQueue);
    /* do our job */
    int result = signalSemaphoreWithIndex(semaIndex);
    /* wake up interpret() if sleeping */
    gEventQueue->notify();
    return result;
}

/* User input recording I:
   In general, either set of input function can be supported,
   depending on the platform. This (first) set is state based
   and should be supported even on platforms that make use
   of the newer event driven API to support older images
   without event support.
*/

int ioGetKeystroke()
{
    ioProcessEvents();  // process all pending events

    ASSERT(gEventQueue);
    if (int stroke = gEventQueue->getKeystroke())
    {
        return stroke;
    }
    return -1;  // keystroke buffer is empty
}

int ioPeekKeystroke()
{
    ioProcessEvents();  // process all pending events

    ASSERT(gEventQueue);
    if (int stroke = gEventQueue->peekKeystroke())
    {
        return stroke;
    }
    return -1;  // keystroke buffer is empty
}

// Return the state of the mouse and modifier buttons
int ioGetButtonState()
{
    ioProcessEvents();  // process all pending events

    ASSERT(gEventQueue);
    return gEventQueue->getButtonState();
}

// Return the mouse point two 16-bit positive integers packed into a 32-bit integer
// x is high 16 bits; y is low 16 bits
int ioMousePoint()
{
    ioProcessEvents();  // process all pending events

    ASSERT(gEventQueue);
    return gEventQueue->getMousePoint();
}

/****************************************************************************/
/*              Event based primitive set                                   */
/****************************************************************************/

/* Note: In an event driven architecture, ioProcessEvents is obsolete.
   It can be implemented as a no-op since the image will check for
   events in regular intervals. */
int ioProcessEvents()
{
    /* process Macintosh events, checking for the interrupt key. Return
       true if the interrupt key was pressed. This might simply do nothing
       on some other platform.*/
    // aioPoll(0);

    /* return true by RecordKeystroke if interrupt key is pressed */
    return 0;
}

/* set an asynchronous input semaphore index for events */
int ioSetInputSemaphore(int semaIndex)
{
    inputSemaphoreIndex = semaIndex;
    return 1;
}

/* retrieve the next input event from the OS */
int ioGetNextEvent(sqInputEvent* event)
{
    ioProcessEvents();  // process all pending events
    ASSERT(gEventQueue);

    es::EventQueue::InputEvent inputEvent;
    gEventQueue->getEvent(&inputEvent);
    if (inputEvent.type)
    {
        event->type = inputEvent.type;
        event->timeStamp = inputEvent.timeStamp;
        event->unused1 = inputEvent.unused1;
        event->unused2 = inputEvent.unused2;
        event->unused3 = inputEvent.unused3;
        event->unused4 = inputEvent.unused4;
        event->unused5 = inputEvent.unused5;
        event->unused6 = inputEvent.unused6;
        return 1;
    }
    return -1;
}

/*** I/O Primitives ***/
int ioSetCursor(int cursorBitsIndex, int offsetX, int offsetY)
{
    /* old version; just call the new version. */
    ioSetCursorWithMask(cursorBitsIndex, 0, offsetX, offsetY);
}

int ioSetCursorWithMask(int cursorBitsIndex, int cursorMaskIndex, int offsetX, int offsetY)
{
    /* Optional primitive; this could be defined to do nothing. */
    /* Set the 16x16 cursor bitmap. If cursorMaskIndex is nil, then make the mask the same as
       the cursor bitmap. If not, then mask and cursor bits combined determine how cursor is
       displayed:
            mask    cursor  effect
             0        0     transparent (underlying pixel shows through)
             1        1     opaque black
             1        0     opaque white
             0        1     invert the underlying pixel
    */
    if (cursorMaskIndex == 0)
    {
        for (int i = 0; i < 16; i++)
        {
            mouseCursor.data[i] = (checkedLongAt(cursorBitsIndex + (4 * i)) >> 16) & 0xFFFF;
            mouseCursor.mask[i] = (checkedLongAt(cursorBitsIndex + (4 * i)) >> 16) & 0xFFFF;
        }
    }
    else
    {
        for (int i = 0; i < 16; i++)
        {
            mouseCursor.data[i] = (checkedLongAt(cursorBitsIndex + (4 * i)) >> 16) & 0xFFFF;
            mouseCursor.mask[i] = (checkedLongAt(cursorMaskIndex + (4 * i)) >> 16) & 0xFFFF;
        }
    }

    /* Squeak hotspot offsets are negative; Mac's are positive */
    mouseCursor.hotSpot = (-offsetX << 16) | -offsetY;
    if (cursor)
    {
        cursor->setPattern(mouseCursor.data, mouseCursor.mask, -offsetX + 16, -offsetY);
    }
}

/*** I/O Stubs ***/

int ioRelinquishProcessorForMicroseconds(int microSeconds)
{
    /* wake us up if something happens */
    ASSERT(gEventQueue);
    gEventQueue->wait(microSeconds * 10LL);
    interruptCheckCounter = 0;  // for smooth eToy animation, etc.
    return microSeconds;
}

int ioForceDisplayUpdate()
{
    /* does nothing on a Mac */
}

int ioScreenSize()
{
    /* return the screen size as two positive 16-bit integers packed into a 32-bit integer */
    int w = WIDTH, h = HEIGHT;

    return (w << 16) | (h & 0xFFFF);  /* w is high 16 bits; h is low 16 bits */
}

int ioScreenDepth()
{
    /* returns the depth of the OS display */
    return 32;
}

int ioShowDisplay(
    int dispBitsIndex, int width, int height, int depth,
    int affectedL, int affectedR, int affectedT, int affectedB)
{
    unsigned *dst, *src;
    int offset;

    if (!cursor || !framebuffer)
    {
        return 1;
    }

    // copy the given rectangular display region to the hardware display buffer.
    if (affectedR <= affectedL || affectedT >= affectedB)
    {
        return 1;
    }

    cursor->hide();
    switch (bpp)
    {
    case 24:
        for (int y = affectedT; y < affectedB; ++y)
        {
            u32* src = (u32*) dispBitsIndex;
            u8* dst = (u8*) framebufferPtr;
            int offset = width * y + affectedL;
            src += offset;
            dst += offset * 3;
            for (int x = affectedL; x < affectedR; ++x)
            {
                u32 pixel = *src++;
                *dst++ = pixel;             // blue
                *dst++ = (pixel >> 8);      // green
                *dst++ = (pixel >> 16);     // red
            }
        }
        break;
    case 32:
        for (int y = affectedT; y < affectedB; ++y)
        {
            u32* src = (u32*) dispBitsIndex;
            u32* dst = (u32*) framebufferPtr;
            int offset = width * y + affectedL;
            src += offset;
            dst += offset;
            for (int x = affectedL; x < affectedR; ++x)
            {
                *dst++ = *src++;
            }
        }
        break;
    }
    cursor->show();

    return 1;
}

int ioFormPrint(
    int bitsAddr, int width, int height, int depth,
    double hScale, double vScale, int landscapeFlag)
{
    success(false);
}

int ioHasDisplayDepth(int depth)
{
    if (depth == 32)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int ioSetDisplayMode(int width, int height, int depth, int fullscreenFlag)
{
    if (width == WIDTH && height == HEIGHT && depth == 32)
    {
        return 1;   // accept
    }
    else
    {
        return 0;   // reject
    }
}

int ioSetFullScreen(int fullScreen)
{
    return 1;
}

void initInputProcess()
{
    Handle<es::Context> root = System()->getRoot();

    gEventQueue = root->lookup("device/event");
    ASSERT(gEventQueue);
    gEventQueue->getMousePoint();

    framebuffer = root->lookup("device/framebuffer");
    long long size;
    size = framebuffer->getSize();
    framebufferPtr = System()->map(0, size,
                                   es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                                   es::CurrentProcess::MAP_SHARED,
                                   framebuffer, 0);
    bpp = 8 * (size / (WIDTH * HEIGHT));

    cursor = root->lookup("device/cursor");
    cursor->show();
}
