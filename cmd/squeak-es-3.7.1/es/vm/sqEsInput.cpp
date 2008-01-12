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

ICurrentProcess* System();

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
}

class EventQueue
{
    static const int KEYBUF_SIZE = 64;
    static const int MAX_EVENT_BUFFER = 1024;

    IMonitor* monitor;

    int keyBuf[KEYBUF_SIZE];    /* circular buffer */
    Ring keyRing;

    sqInputEvent eventBuffer[MAX_EVENT_BUFFER];
    Ring eventRing;

    u8 key[8 + 1];
    int modifiers;
    bool caps;
    bool numlock;

    int width;
    int height;
    u8 button;
    int x;
    int y;

    void keyDown(u8 key)
    {
        using namespace UsageID;

        switch (key)
        {
        case KEYBOARD_CAPS_LOCK:
            caps ^= true;
            break;
        case KEYPAD_NUM_LOCK:
            numlock ^= true;
            break;
        }

        key = translate(key);

        if (key)
        {
            if (inputSemaphoreIndex)
            {
                sqKeyboardEvent event;

                event.type = EventTypeKeyboard;
                event.timeStamp = 0;
                event.charCode = key;   // XXX
                event.pressCode = EventKeyDown;
                event.modifiers = modifiers;
                event.reserved1 = event.reserved2 = event.reserved3 = 0;
                eventRing.write(&event, sizeof event);

                event.pressCode = EventKeyChar;
                eventRing.write(&event, sizeof event);

                // signalSemaphoreWithIndex(inputSemaphoreIndex);
            }
            int stroke((modifiers << 8) | key);
            keyRing.write(&stroke, sizeof stroke);
        }
    }

    void keyUp(u8 key)
    {
        key = translate(key);
        if (key)
        {
            if (inputSemaphoreIndex)
            {
                sqKeyboardEvent event;

                event.type = EventTypeKeyboard;
                event.timeStamp = 0;
                event.charCode = key;   // XXX
                event.pressCode = EventKeyUp;
                event.modifiers = modifiers;
                event.reserved1 = event.reserved2 = event.reserved3 = 0;
                eventRing.write(&event, sizeof event);

                // signalSemaphoreWithIndex(inputSemaphoreIndex);
            }
        }
    }

    void mouse(u8 button, int x, int y)
    {
        using namespace UsageID;

        if (inputSemaphoreIndex)
        {
            sqMouseEvent event;
            event.type = EventTypeMouse;
            event.timeStamp = 0;
            event.x = x;
            event.y = y;
            event.buttons = ((button & 1) ? RedButtonBit : 0) |
                            ((button & 2) ? BlueButtonBit : 0) |
                            ((button & 4) ? YellowButtonBit : 0);
            event.modifiers = modifiers;
            event.reserved1 = event.reserved2 = 0;
            eventRing.write(&event, sizeof event);

            // signalSemaphoreWithIndex(inputSemaphoreIndex);
        }
    }

    u8 translateControl(u8 key)
    {
        using namespace UsageID;

        switch (key)
        {
        case KEYBOARD_HOME:
            return 1;
        case KEYBOARD_ENTER:
            return 3;
        case KEYBOARD_END:
            return 4;
        case KEYBOARD_INSERT:
            return 5;
        case KEYBOARD_BACKSPACE:
            return 8;
        case KEYBOARD_TAB:
            return 9;
        case KEYBOARD_PAGEUP:
            return 11;
        case KEYBOARD_PAGEDOWN:
            return 12;
        case KEYBOARD_RETURN:
            return 13;
        case KEYBOARD_LEFTALT:
            return 17;
        case KEYBOARD_RIGHTALT:
            return 20;
        case KEYBOARD_ESCAPE:
            return 27;
        case KEYBOARD_LEFTARROW:
            return 28;
        case KEYBOARD_RIGHTARROW:
            return 29;
        case KEYBOARD_UPARROW:
            return 30;
        case KEYBOARD_DOWNARROW:
            return 31;
        case KEYBOARD_DELETE:
            return 127;
        }
        return 0;
    }

    u8 translateKeypad(u8 key)
    {
        using namespace UsageID;

        if (numlock && !(modifiers & ShiftKeyBit))
        {
            return 0;
        }
        switch (key)
        {
        case KEYPAD_DOT:
            key = KEYBOARD_DELETE;
            break;
        case KEYPAD_1:
            key = KEYBOARD_END;
            break;
        case KEYPAD_2:
            key = KEYBOARD_DOWNARROW;
            break;
        case KEYPAD_3:
            key = KEYBOARD_PAGEDOWN;
            break;
        case KEYPAD_4:
            key = KEYBOARD_LEFTARROW;
            break;
        case KEYPAD_6:
            key = KEYBOARD_RIGHTARROW;
            break;
        case KEYPAD_7:
            key = KEYBOARD_HOME;
            break;
        case KEYPAD_8:
            key = KEYBOARD_UPARROW;
            break;
        case KEYPAD_9:
            key = KEYBOARD_PAGEUP;
            break;
        case KEYPAD_0:
            key = KEYBOARD_INSERT;
            break;
        default:
            return 0;
        }
        return translateControl(key);
    }

    u8 translateNormal(u8 key)
    {
        using namespace UsageID;

        if (KEYBOARD_A <= key && key <= KEYBOARD_Z)
        {
            key -= KEYBOARD_A;
            if (caps)
            {
                return 'A' + key;
            }
            else
            {
                return 'a' + key;
            }
        }

        switch (key)
        {
        case KEYBOARD_1:
            return '1';
        case KEYBOARD_2:
            return '2';
        case KEYBOARD_3:
            return '3';
        case KEYBOARD_4:
            return '4';
        case KEYBOARD_5:
            return '5';
        case KEYBOARD_6:
            return '6';
        case KEYBOARD_7:
            return '7';
        case KEYBOARD_8:
            return '8';
        case KEYBOARD_9:
            return '9';
        case KEYBOARD_0:
            return '0';

        case KEYBOARD_SPACEBAR:
            return ' ';
        case KEYBOARD_MINUS:
            return '-';
        case KEYBOARD_EQUAL:
            return '=';
        case KEYBOARD_LEFT_BRACKET:
            return '[';
        case KEYBOARD_RIGHT_BRACKET:
            return ']';
        case KEYBOARD_BACKSLASH:
            return '\\';
        case KEYBOARD_SEMICOLON:
            return ';';
        case KEYBOARD_QUOTE:
            return '\'';
        case KEYBOARD_GRAVE_ACCENT:
            return '`';
        case KEYBOARD_COMMA:
            return ',';
        case KEYBOARD_PERIOD:
            return '.';
        case KEYBOARD_SLASH:
            return '/';

        case KEYPAD_1:
            return '1';
        case KEYPAD_2:
            return '2';
        case KEYPAD_3:
            return '3';
        case KEYPAD_4:
            return '4';
        case KEYPAD_5:
            return '5';
        case KEYPAD_6:
            return '6';
        case KEYPAD_7:
            return '7';
        case KEYPAD_8:
            return '8';
        case KEYPAD_9:
            return '9';
        case KEYPAD_0:
            return '0';

        case KEYPAD_MULTIPLY:
            return '*';
        case KEYPAD_DIVIDE:
            return '/';
        case KEYPAD_ADD:
            return '+';
        case KEYPAD_SUBTRACT:
            return '-';
        case KEYPAD_DOT:
            return '.';
        }
        return 0;
    }

    u8 translateShift(u8 key)
    {
        using namespace UsageID;

        if (KEYBOARD_A <= key && key <= KEYBOARD_Z)
        {
            key -= KEYBOARD_A;
            if (caps)
            {
                return 'a' + key;
            }
            else
            {
                return 'A' + key;
            }
        }

        switch (key)
        {
        case KEYBOARD_1:
            return '!';
        case KEYBOARD_2:
            return '@';
        case KEYBOARD_3:
            return '#';
        case KEYBOARD_4:
            return '$';
        case KEYBOARD_5:
            return '%';
        case KEYBOARD_6:
            return '^';
        case KEYBOARD_7:
            return '&';
        case KEYBOARD_8:
            return '*';
        case KEYBOARD_9:
            return '(';
        case KEYBOARD_0:
            return ')';

        case KEYBOARD_SPACEBAR:
            return ' ';
        case KEYBOARD_MINUS:
            return '_';
        case KEYBOARD_EQUAL:
            return '+';
        case KEYBOARD_LEFT_BRACKET:
            return '{';
        case KEYBOARD_RIGHT_BRACKET:
            return '}';
        case KEYBOARD_BACKSLASH:
            return '|';
        case KEYBOARD_SEMICOLON:
            return ':';
        case KEYBOARD_QUOTE:
            return '"';
        case KEYBOARD_GRAVE_ACCENT:
            return '~';
        case KEYBOARD_COMMA:
            return '<';
        case KEYBOARD_PERIOD:
            return '>';
        case KEYBOARD_SLASH:
            return '?';

        case KEYPAD_MULTIPLY:
            return '*';
        case KEYPAD_DIVIDE:
            return '/';
        case KEYPAD_ADD:
            return '+';
        case KEYPAD_SUBTRACT:
            return '-';
        }
        return 0;
    }

    u8 translate(u8 key)
    {
        using namespace UsageID;

        u8 control;

        control = translateKeypad(key);
        if (control)
        {
            return control;
        }

        control = translateControl(key);
        if (control)
        {
            return control;
        }

        if (!(modifiers & ShiftKeyBit))
        {
            key = translateNormal(key);
        }
        else
        {
            key = translateShift(key);
        }
        return key;
    }

public:
    EventQueue() :
        keyRing(keyBuf, sizeof keyBuf),
        eventRing(eventBuffer, sizeof eventBuffer),
        modifiers(0),
        caps(false),
        numlock(true),
        width(1024),
        height(768),
        button(0),
        x(width / 2),
        y(height / 2)
    {
        monitor = System()->createMonitor();
        key[0] = 0;
        memset(key + 1, 255, 8);
    }

    ~EventQueue()
    {
        if (monitor)
        {
            monitor->release();
        }
    }

    bool getEvent(sqInputEvent* event)
    {
        Synchronized<IMonitor*> method(monitor);

        int count = eventRing.read(event, sizeof(sqInputEvent));
        return (count == sizeof(sqInputEvent)) ? true : false;
    }

    bool getKeystroke(int* stroke)
    {
        Synchronized<IMonitor*> method(monitor);

        int count = keyRing.read(stroke, sizeof(int));
        return (count == sizeof(int)) ? true : false;
    }

    bool peekKeystroke(int* stroke)
    {
        Synchronized<IMonitor*> method(monitor);

        int count = keyRing.peek(stroke, sizeof(int));
        return (count == sizeof(int)) ? true : false;
    }

    u8 getButtonState()
    {
        Synchronized<IMonitor*> method(monitor);

        return (modifiers << 3) |
               ((button & 1) ? RedButtonBit : 0) |
               ((button & 2) ? BlueButtonBit : 0) |
               ((button & 4) ? YellowButtonBit : 0);
    }

    void getMousePoint(int& x, int &y)
    {
        Synchronized<IMonitor*> method(monitor);

        x = this->x;
        y = this->y;
    }

    void keyEvent(u8* data, long size)
    {
        Synchronized<IMonitor*> method(monitor);

        using namespace UsageID;

        u8 next[8 + 1];
        if (8 < size)
        {
            size = 8;
        }
        memmove(next, data, size);
        memset(next + size, 255, 9 - size);

        u8* from = key;
        u8* to = next;
        unsigned bits;

        bits = (*from ^ *to) & *from;
        while (bits)
        {
            int key = ffs(bits) - 1;
            ASSERT(0 <= key);
            bits &= ~(1u << key);
            keyDown(key + KEYBOARD_LEFTCONTROL);
        }

        bits = (*from ^ *to) & *to;
        while (bits)
        {
            int key = ffs(bits) - 1;
            ASSERT(0 <= key);
            bits &= ~(1u << key);
            keyUp(key + KEYBOARD_LEFTCONTROL);
        }

        u8 mod(*to | (*to >> 4));
        modifiers = ((mod & (1<<(0x0f & KEYBOARD_LEFTCONTROL))) ? CtrlKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_LEFTSHIFT))) ? ShiftKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_LEFTALT))) ? CommandKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_RIGHTCONTROL))) ? CtrlKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_RIGHTSHIFT))) ? ShiftKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_RIGHTALT))) ? OptionKeyBit : 0);

        *from++ = *to++;
        do
        {
            if (*from < *to)
            {
                keyUp(*from);
                ++from;
            }
            else if (*to < *from)
            {
                keyDown(*to);
                ++to;
            }
            else
            {
                // XXX auto
                ++from;
                ++to;
            }
        } while (*to != *from || *to != 255);

        memmove(key, next, 9);
    }

    void mouseEvent(u8* data, long size)
    {
        Synchronized<IMonitor*> method(monitor);

        using namespace UsageID;

        if (4 <= size)
        {
            s8 z(data[3]);
            if (z < 0)
            {
                keyDown(KEYBOARD_PAGEUP);
                keyUp(KEYBOARD_PAGEUP);
            }
            else if (0 < z)
            {
                keyDown(KEYBOARD_PAGEDOWN);
                keyUp(KEYBOARD_PAGEDOWN);
            }
        }

        if (size < 3 || !(data[0] ^ button) && !data[1] && !data[2])
        {
            return;
        }
        button = data[0];
        x += (s8) data[1];
        y -= (s8) data[2];
        if (x < 0)
        {
            x = 0;
        }
        if (width <= x)
        {
            x = width - 1;
        }
        if (y < 0)
        {
            y = 0;
        }
        if (height <= y)
        {
            y = height - 1;
        }

        mouse(button, x, y);
    }
};

/*** Variables -- Event Recording ***/

EventQueue eventQueue __attribute__ ((init_priority (1001)));;

Cursor mouseCursor;

Handle<IPageable> framebuffer;
Handle<ICursor> cursor;
int bpp;
void* framebufferPtr;

/* User input recording I:
   In general, either set of input function can be supported,
   depending on the platform. This (first) set is state based
   and should be supported even on platforms that make use
   of the newer event driven API to support older images
   without event support.
*/

int ioGetKeystroke(void)
{
    ioProcessEvents();  // process all pending events

    int stroke;
    if (eventQueue.getKeystroke(&stroke))
    {
        return stroke;
    }
    return -1;  // keystroke buffer is empty
}

int ioPeekKeystroke(void)
{
    ioProcessEvents();  // process all pending events

    int stroke;
    if (eventQueue.peekKeystroke(&stroke))
    {
        return stroke;
    }
    return -1;  // keystroke buffer is empty
}

// Return the state of the mouse and modifier buttons
int ioGetButtonState(void)
{
    ioProcessEvents();  // process all pending events

    return eventQueue.getButtonState();
}

// Return the mouse point two 16-bit positive integers packed into a 32-bit integer
int ioMousePoint(void)
{
    ioProcessEvents();  // process all pending events

    int x;
    int y;
    eventQueue.getMousePoint(x, y);
    return (x << 16) | y;   // x is high 16 bits; y is low 16 bits */
}

/****************************************************************************/
/*              Event based primitive set                                   */
/****************************************************************************/

/* Note: In an event driven architecture, ioProcessEvents is obsolete.
   It can be implemented as a no-op since the image will check for
   events in regular intervals. */
int ioProcessEvents(void)
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

    if (eventQueue.getEvent(event) == sizeof(sqInputEvent))
    {
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
    Handle<ICurrentThread> current(System()->currentThread());
    current->sleep(microSeconds * 10);

    // aioPoll(microSeconds);
    return 0;
}

int ioForceDisplayUpdate(void)
{
    /* does nothing on a Mac */
}

int ioScreenSize(void)
{
    /* return the screen size as two positive 16-bit integers packed into a 32-bit integer */
    int w = 1024, h = 768;

    return (w << 16) | (h & 0xFFFF);  /* w is high 16 bits; h is low 16 bits */
}

int ioScreenDepth(void)
{
    /* returns the depth of the OS display */
    return 32;
}

int ioShowDisplay(
    int dispBitsIndex, int width, int height, int depth,
    int affectedL, int affectedR, int affectedT, int affectedB)
{
    int x, y;
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
    if (width == 1024 && height == 768 && depth == 32)
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

void* inputProcess(void* param)
{
    using namespace UsageID;

    Handle<IContext> root = System()->getRoot();
    Handle<IStream> keyboard(root->lookup("device/keyboard"));
    Handle<IStream> mouse(root->lookup("device/mouse"));
    Handle<ICurrentThread> currentThread = System()->currentThread();

    int x;
    int y;
    eventQueue.getMousePoint(x, y);

    framebuffer = root->lookup("device/framebuffer");
    long long size;
    size = framebuffer->getSize();
    framebufferPtr = System()->map(0, size,
                                 ICurrentProcess::PROT_READ | ICurrentProcess::PROT_WRITE,
                                 ICurrentProcess::MAP_SHARED,
                                 framebuffer, 0);
    bpp = 8 * (size / (1024 * 768));

    cursor = root->lookup("device/cursor");
    cursor->setPosition(x, y);
    cursor->show();

    for (;;)
    {
        u8 buffer[8];
        long count;

        count = keyboard->read(buffer, 8);
        eventQueue.keyEvent(buffer, count);

        count = mouse->read(buffer, 4);
        eventQueue.mouseEvent(buffer, count);

        eventQueue.getMousePoint(x, y);
        cursor->setPosition(x, y);

        currentThread->sleep(10000000 / 60);
    }

    return 0;
}
