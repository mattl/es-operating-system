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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/ring.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/usage.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/base/IInterfaceStore.h>
#include <es/device/ICursor.h>
#include <es/naming/IContext.h>
#include <es/interlocked.h>

#include "eventManager.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

namespace
{
    Interlocked registered = 0;
};

class EventManager : public IEventQueue
{
    static const int WIDTH  = 1024; // [check] how to determine this parameter?
    static const int HEIGHT = 768;  // [check]

    static const int KEYBUF_SIZE = 64;
    static const int MAX_EVENT_BUFFER = 1024;
    static const int AUTO_REPEAT_RATE = 4;
    static const int AUTO_REPEAT_DELAY = 20;

    Ref ref;
    IMonitor* monitor;

    int keyBuf[KEYBUF_SIZE];    /* circular buffer */
    Ring keyRing;

    InputEvent eventBuffer[MAX_EVENT_BUFFER];
    Ring eventRing;

    u8 key[8 + 1];
    int modifiers;
    bool caps;
    bool numlock;
    int repeat;

    int width;
    int height;
    u8 button;
    int x;
    int y;

    Handle<ICursor> cursor;
    static int id;

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
            {
                Synchronized<IMonitor*> method(monitor);
                KeyboardEvent event;

                event.type = EventTypeKeyboard;
                event.timeStamp = 0;
                event.charCode = key;   // XXX
                event.pressCode = EventKeyDown;
                event.modifiers = modifiers;
                event.reserved1 = event.reserved2 = event.reserved3 = 0;
                eventRing.write(&event, sizeof event);

                event.pressCode = EventKeyChar;
                eventRing.write(&event, sizeof event);

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
            Synchronized<IMonitor*> method(monitor);

            KeyboardEvent event;

            event.type = EventTypeKeyboard;
            event.timeStamp = 0;
            event.charCode = key;   // XXX
            event.pressCode = EventKeyUp;
            event.modifiers = modifiers;
            event.reserved1 = event.reserved2 = event.reserved3 = 0;
            eventRing.write(&event, sizeof event);

        }
    }

    void mouse(u8 button, int x, int y)
    {
        Synchronized<IMonitor*> method(monitor);

        using namespace UsageID;

        MouseEvent event;
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
    }

    u8 translateControl(u8 key)
    {
        using namespace UsageID;

        switch (key)
        {
        case KEYBOARD_HOME:
            return 1;
        case KEYPAD_ENTER:
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
        case KEYBOARD_ENTER:
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
    EventManager() :
        keyRing(keyBuf, sizeof keyBuf),
        eventRing(eventBuffer, sizeof eventBuffer),
        modifiers(0),
        caps(false),
        numlock(true),
        repeat(0),
        width(WIDTH),
        height(HEIGHT),
        button(0),
        x(width / 2),
        y(height / 2)
    {
        monitor = System()->createMonitor();
        key[0] = 0;
        memset(key + 1, 255, 8);

        Handle<IContext> root = System()->getRoot();
        cursor = root->lookup("device/cursor");
    }

    ~EventManager()
    {
        if (monitor)
        {
            monitor->release();
        }
    }

    //
    // IEventQueue
    //
    bool getEvent(InputEvent* event)
    {
        if (!registered)
        {
            return false;
        }

        Synchronized<IMonitor*> method(monitor);

        int count = eventRing.read(event, sizeof(InputEvent));
        return (count == sizeof(InputEvent)) ? true : false;
    }

    bool getKeystroke(int* stroke)
    {
        if (!registered)
        {
            return false;
        }

        Synchronized<IMonitor*> method(monitor);

        int count = keyRing.read(stroke, sizeof(int));
        return (count == sizeof(int)) ? true : false;
    }

    bool peekKeystroke(int* stroke)
    {
        if (!registered)
        {
            return false;
        }

        Synchronized<IMonitor*> method(monitor);

        int count = keyRing.peek(stroke, sizeof(int));
        return (count == sizeof(int)) ? true : false;
    }

    unsigned int getButtonState()
    {
        if (!registered)
        {
            return 0;
        }

        Synchronized<IMonitor*> method(monitor);

        return (modifiers << 3) |
               ((button & 1) ? RedButtonBit : 0) |
               ((button & 2) ? BlueButtonBit : 0) |
               ((button & 4) ? YellowButtonBit : 0);
    }

    void getMousePoint(int& x, int &y)
    {
        if (registered)
        {
            Synchronized<IMonitor*> method(monitor);

            x = this->x;
            y = this->y;
        }
    }

    bool keyEvent(const void* data, int size)
    {
        if (!registered)
        {
            return false;
        }

        Synchronized<IMonitor*> method(monitor);
        bool notify(false);

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
            notify = true;
        }

        bits = (*from ^ *to) & *to;
        while (bits)
        {
            int key = ffs(bits) - 1;
            ASSERT(0 <= key);
            bits &= ~(1u << key);
            keyUp(key + KEYBOARD_LEFTCONTROL);
            notify = true;
        }

        u8 mod(*to | (*to >> 4));
        modifiers = ((mod & (1<<(0x0f & KEYBOARD_LEFTCONTROL))) ? CtrlKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_LEFTSHIFT))) ? ShiftKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_LEFTALT))) ? CommandKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_RIGHTCONTROL))) ? CtrlKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_RIGHTSHIFT))) ? ShiftKeyBit : 0) |
                    ((mod & (1<<(0x0f & KEYBOARD_RIGHTALT))) ? OptionKeyBit : 0);

        bool repeated(false);
        *from++ = *to++;
        do
        {
            if (*from < *to)
            {
                keyUp(*from);
                notify = true;
                ++from;
                repeat = 0;
            }
            else if (*to < *from)
            {
                keyDown(*to);
                notify = true;
                ++to;
                repeat = 0;
            }
            else
            {
                repeated = true;
                if (++repeat == AUTO_REPEAT_DELAY)
                {
                    keyDown(*to);   // Auto repeat
                    repeat = AUTO_REPEAT_DELAY - AUTO_REPEAT_RATE;
                }
                ++from;
                ++to;
            }
        } while (*to != *from || *to != 255);
        if (!repeated)
        {
            repeat = 0;
        }

        memmove(key, next, 9);

        return notify;
    }

    bool mouseEvent(const void* eventData, int size)
    {
        if (!registered)
        {
            return false;
        }

        Synchronized<IMonitor*> method(monitor);
        u8* data = (u8*) eventData;
        bool notify(false);

        using namespace UsageID;

        if (4 <= size)
        {
            s8 z(data[3]);
            if (z < 0)
            {
                keyDown(KEYBOARD_PAGEUP);
                keyUp(KEYBOARD_PAGEUP);
                notify = true;
            }
            else if (0 < z)
            {
                keyDown(KEYBOARD_PAGEDOWN);
                keyUp(KEYBOARD_PAGEDOWN);
                notify = true;
            }
        }

        if (3 <= size && ((data[0] ^ button) || data[1] || data[2]))
        {
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
            cursor->setPosition(x, y);
            notify = true;
        }

        return notify;
    }

    bool wait(long long timeout)
    {
        Synchronized<IMonitor*> method(monitor);

        if (registered && eventRing.getUsed() == 0)
        {
            return monitor->wait(timeout);
        }
        return false;
    }

    void notify()
    {
        if (registered)
        {
            monitor->notifyAll();
        }
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IEventQueue*>(this);
        }
        else if (riid == IID_IEventQueue)
        {
            *objectPtr = static_cast<IEventQueue*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
    }

    unsigned int addRef()
    {
        unsigned int count = ref.addRef();
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
            monitor->notifyAll();
            delete this;
            return 0;
        }
        return count;
    }
};

static void* inputProcess(Handle<IContext> nameSpace)
{
    using namespace UsageID;

    // Create Event manager objects.
    Handle<IEventQueue> eventQueue = new EventManager;
    ASSERT(eventQueue);

    // register the event queue.
    Handle<IContext> device = nameSpace->lookup("device");
    ASSERT(device);
    IBinding* ret = device->bind("event", static_cast<IEventQueue*>(eventQueue));
    ASSERT(ret);

    Handle<IStream> keyboard(nameSpace->lookup("device/keyboard"));
    ASSERT(keyboard);
    Handle<IStream> mouse(nameSpace->lookup("device/mouse"));
    ASSERT(mouse);
    Handle<ICurrentThread> currentThread = System()->currentThread();

    int x;
    int y;

    while (registered)
    {
        u8 buffer[8];
        long count;
        bool notify(false);

        count = keyboard->read(buffer, 8);
        notify |= eventQueue->keyEvent(buffer, count);

        count = mouse->read(buffer, 4);
        notify |= eventQueue->mouseEvent(buffer, count);

        if (notify)
        {
            eventQueue->notify();
        }
        ASSERT(currentThread);
        currentThread->sleep(10000000 / 60);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    esReport("This is the Event manager server process.\n");
    // System()->trace(true);

    Handle<IContext> nameSpace = System()->getRoot();
    Handle<IClassStore> classStore = nameSpace->lookup("class");
    TEST(classStore);

    // Register IEventQueue interface.
    Handle<IInterfaceStore> interfaceStore = nameSpace->lookup("interface");
    interfaceStore->add(IEventQueueInfo, IEventQueueInfoSize);

    // Register Event manager factory.
    Handle<IClassFactory> eventManagerFactory(new(ClassFactory<EventManager>));
    classStore->add(CLSID_EventManager, eventManagerFactory);

    inputProcess(nameSpace);

    // Unregister Event Manager factory.
    classStore->remove(CLSID_EventManager);

    // Unregister IEventQueue interface.
    interfaceStore->remove(IID_IEventQueue);

    esReport("Event manager is terminated.\n");
    // System()->trace(false);
}
