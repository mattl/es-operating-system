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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/usage.h>
#include "core.h"
#include "io.h"
#include "8042.h"

// #define VERBOSE

using namespace UsageID;

u8 Keyboard::keycode[128] =
{
    // 0x00
    0,
    KEYBOARD_ESCAPE,
    KEYBOARD_1,
    KEYBOARD_2,
    KEYBOARD_3,
    KEYBOARD_4,
    KEYBOARD_5,
    KEYBOARD_6,
    // 0x08
    KEYBOARD_7,
    KEYBOARD_8,
    KEYBOARD_9,
    KEYBOARD_0,
    KEYBOARD_MINUS,
    KEYBOARD_EQUAL,
    KEYBOARD_BACKSPACE,
    KEYBOARD_TAB,
    // 0x10
    KEYBOARD_Q,
    KEYBOARD_W,
    KEYBOARD_E,
    KEYBOARD_R,
    KEYBOARD_T,
    KEYBOARD_Y,
    KEYBOARD_U,
    KEYBOARD_I,
    // 0x18
    KEYBOARD_O,
    KEYBOARD_P,
    KEYBOARD_LEFT_BRACKET,
    KEYBOARD_RIGHT_BRACKET,
    KEYBOARD_ENTER,
    KEYBOARD_LEFTCONTROL,
    KEYBOARD_A,
    KEYBOARD_S,
    // 0x20
    KEYBOARD_D,
    KEYBOARD_F,
    KEYBOARD_G,
    KEYBOARD_H,
    KEYBOARD_J,
    KEYBOARD_K,
    KEYBOARD_L,
    KEYBOARD_SEMICOLON,
    // 0x28
    KEYBOARD_QUOTE,
    KEYBOARD_GRAVE_ACCENT,
    KEYBOARD_LEFTSHIFT,
    KEYBOARD_BACKSLASH,
    KEYBOARD_Z,
    KEYBOARD_X,
    KEYBOARD_C,
    KEYBOARD_V,
    // 0x30
    KEYBOARD_B,
    KEYBOARD_N,
    KEYBOARD_M,
    KEYBOARD_COMMA,
    KEYBOARD_PERIOD,
    KEYBOARD_SLASH,
    KEYBOARD_RIGHTSHIFT,
    KEYPAD_MULTIPLY,
    // 0x38
    KEYBOARD_LEFTALT,
    KEYBOARD_SPACEBAR,
    KEYBOARD_CAPS_LOCK,
    KEYBOARD_F1,
    KEYBOARD_F2,
    KEYBOARD_F3,
    KEYBOARD_F4,
    KEYBOARD_F5,
    // 0x40
    KEYBOARD_F6,
    KEYBOARD_F7,
    KEYBOARD_F8,
    KEYBOARD_F9,
    KEYBOARD_F10,
    KEYPAD_NUM_LOCK,
    KEYBOARD_SCROLL_LOCK,
    KEYPAD_7,
    // 0x48
    KEYPAD_8,
    KEYPAD_9,
    KEYPAD_SUBTRACT,
    KEYPAD_4,
    KEYPAD_5,
    KEYPAD_6,
    KEYPAD_ADD,
    KEYPAD_1,
    // 0x50
    KEYPAD_2,
    KEYPAD_3,
    KEYPAD_0,
    KEYPAD_DOT,
    0,
    0,
    0,
    KEYBOARD_F11,
    // 0x58
    KEYBOARD_F12,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x60
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x68
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x70
    KEYBOARD_INTERNATIONAL2,
    0,
    0,
    KEYBOARD_INTERNATIONAL3,
    0,
    0,
    0,
    0,
    // 0x78
    0,
    KEYBOARD_INTERNATIONAL4,
    0,
    KEYBOARD_INTERNATIONAL5,
    0,
    KEYBOARD_INTERNATIONAL1,
    0,
    0
};

u8 Keyboard::keycodeEsc[128] =
{
    // 0x00
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x08
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x10
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x18
    0,
    0,
    0,
    0,
    KEYPAD_ENTER,
    KEYBOARD_RIGHTCONTROL,
    0,
    0,
    // 0x20
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x28
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x30
    0,
    0,
    0,
    0,
    0,
    KEYPAD_DIVIDE,
    0,
    KEYBOARD_PRINTSCREEN,
    // 0x38
    KEYBOARD_RIGHTALT,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x40
    0,
    0,
    0,
    0,
    0,
    0,
    KEYBOARD_PAUSE,
    KEYBOARD_HOME,
    // 0x48
    KEYBOARD_UPARROW,
    KEYBOARD_PAGEUP,
    0,
    KEYBOARD_LEFTARROW,
    0,
    KEYBOARD_RIGHTARROW,
    0,
    KEYBOARD_END,
    // 0x50
    KEYBOARD_DOWNARROW,
    KEYBOARD_PAGEDOWN,
    KEYBOARD_INSERT,
    KEYBOARD_DELETE,
    0,
    0,
    0,
    0,
    // 0x58
    0,
    0,
    0,
    KEYBOARD_LEFT_GUI,
    KEYBOARD_RIGHT_GUI,
    0,
    0,
    0,
    // 0x60
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x68
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x70
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x78
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

u8 Keyboard::
sendControllerCommand(u8 cmd)
{
    u8 r;

    while ((r = inpb(STATUS_BYTE)) & (INPUT_BUFFER_FULL | OUTPUT_BUFFER_FULL))
    {
        if (r & OUTPUT_BUFFER_FULL)
        {
            inpb(DATA_BUFFER);
        }
#if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__ ("pause\n");
#endif
    }
    outpb(COMMAND_BYTE, cmd);

    switch (cmd)
    {
    case WRITE_KEYBOARD_CONTROLLER_COMMAND_BYTE:
    case WRITE_AUXILIARY_DEVICE:
        return 0;
        break;
    default:
        return receiveData();
        break;
    }
}

void Keyboard::
sendData(u8 data)
{
    u8 r;

    while ((r = inpb(STATUS_BYTE)) & (INPUT_BUFFER_FULL | OUTPUT_BUFFER_FULL))
    {
        if (r & OUTPUT_BUFFER_FULL)
        {
            inpb(DATA_BUFFER);
        }
#if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__ ("pause\n");
#endif
    }
    outpb(DATA_BUFFER, data);
}

u8 Keyboard::
receiveData(int retry)
{
    while (!(inpb(STATUS_BYTE) & OUTPUT_BUFFER_FULL))
    {
#if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__ ("pause\n");
#endif
        --retry;
        if (retry < 0)
        {
            throw SystemException<ENODEV>();
        }
    }
    return inpb(DATA_BUFFER);
}

void Keyboard::
reset(void)
{
    if (sendControllerCommand(SELF_TEST) != 0x55)
    {
        return;
    }

    if (sendControllerCommand(INTERFACE_TEST) != 0x00)
    {
        return;
    }

    cmd = sendControllerCommand(READ_KEYBOARD_CONTROLLER_COMMAND_BYTE);
    cmd &= ~(DISABLE_KEYBOARD | DISABLE_AUXILIARY_DEVICE | ENABLE_KEYBOARD_INTERRUPT);
    cmd |= SYSTEM_FLAG | ENABLE_AUXILIARY_INTERRUPT;
    sendControllerCommand(WRITE_KEYBOARD_CONTROLLER_COMMAND_BYTE);
    sendData(cmd);

    u8 rc;
    do
    {
        sendData(RESET);
        rc = receiveData();
    } while (rc != 0xfa);
    if (receiveData() != 0xaa)
    {
        return;
    }

    cmd &= ~(DISABLE_KEYBOARD | DISABLE_AUXILIARY_DEVICE);
    cmd |= IBM_KEYBOARD_TRANSLATE_MODE | ENABLE_KEYBOARD_INTERRUPT | ENABLE_AUXILIARY_INTERRUPT;
    sendControllerCommand(WRITE_KEYBOARD_CONTROLLER_COMMAND_BYTE);
    sendData(cmd);
}

s8 Keyboard::
clamp(int n)
{
    if (n < -128)
    {
        return -128;
    }
    if (127 < n)
    {
        return 127;
    }
    return n;
}

int Keyboard::
invoke(int param)
{
    u8 status;

    while ((status = inpb(STATUS_BYTE)) & OUTPUT_BUFFER_FULL)
    {
        if (status & AUXILIARY_OUTPUT_BUFFER_FULL)
        {
            u8 data = inpb(DATA_BUFFER);
            if (count == 0 && !(data & 0x08))
            {
                continue;
            }
            packet[count] = data;

            s8 dz = 0;
            ++count;
            switch (aux)
            {
            case 0: // PS/2 mouse
                if (count == 3)
                {
                    count = 0;
                    packet[3] = 0;
                }
                break;
            case 3: // Intellimouse
                if (count == 4)
                {
                    dz = data;
                    count = 0;
                    packet[3] = 0;
                }
                break;
            case 4: // Intellimouse (5 button)
                if (count == 4)
                {
                    dz = data & 0xff;
                    if (dz & 0x08)
                    {
                        dz |= ~0xf;
                    }
                    count = 0;
                }
                break;
            default:
                count = 0;
                break;
            }

            if (count == 0)
            {
                Lock::Synchronized method(spinLock);

                int d;

                d = packet[1];
                if (packet[0] & 0x10)
                {
                    d |= ~0xff;
                }
                axis[0] = clamp(axis[0] + d);

                d = packet[2];
                if (packet[0] & 0x20)
                {
                    d |= ~0xff;
                }
                axis[1] = clamp(axis[1] + d);

                axis[2] = clamp(axis[2] + dz);
                button = ((packet[3] >> 1) & 0x18) | (packet[0] & 0x07);
                // esReport("aux: %02x %02x %02x %02x: %02x\n", packet[0], packet[1], packet[2], packet[3], (u8) dz);
            }
        }
        else
        {
            u8 data = inpb(DATA_BUFFER);
            u16 key = 0;
            switch (data)
            {
            case 0xe0:
                esc = 0xe0;
                break;
            case 0xe1:
                esc = 0xe1;
                break;
            default:
                switch (esc)
                {
                case 0xe0:
                    key = keycodeEsc[data & 0x7f];
                    esc = 0;
                    break;
                case 0xe1:
                    ++esc;
                    break;
                case 0xe2:
                    key = KEYBOARD_PAUSE;
                    esc = 0;
                    break;
                default:
                    key = keycode[data & 0x7f];
                    esc = 0;
                    break;
                }
                break;
            }
            if (key)
            {
                Lock::Synchronized method(spinLock);

                if (data & 0x80)
                {
                    // break
                    map[key / (8 * sizeof(unsigned))] &= ~(1u << (key % (8 * sizeof(unsigned))));
                }
                else
                {
                    // make
                    map[key / (8 * sizeof(unsigned))] |= (1u << (key % (8 * sizeof(unsigned))));
                }
            }
#ifdef VERBOSE
            if (key)
            {
                esReport("kbd: %02x (%02x) %s\n", key, data, (data & 0x80) ? "break" : "make");
            }
#endif  // VERBOSE
        }
    }
    return 0;
}

void Keyboard::
setLED(u8 led)
{
    sendData(SET_MODE_INDICATORS);
    sendData(led & 0x07);
}

Keyboard::
Keyboard(es::Context* device) :
    cmd(0),
    esc(0),
    aux(0xff),
    count(0),
    button(0),
    keyboardStream(this, &Keyboard::readKeyboard),
    mouseStream(this, &Keyboard::readMouse)
{
    memset(map, 0, sizeof map);
    memset(axis, 0, sizeof axis);

    cmd = sendControllerCommand(READ_KEYBOARD_CONTROLLER_COMMAND_BYTE);
    cmd &= ~(DISABLE_KEYBOARD | DISABLE_AUXILIARY_DEVICE);
    cmd |= IBM_KEYBOARD_TRANSLATE_MODE | ENABLE_KEYBOARD_INTERRUPT | ENABLE_AUXILIARY_INTERRUPT;
    sendControllerCommand(WRITE_KEYBOARD_CONTROLLER_COMMAND_BYTE);
    sendData(cmd);

    detectAuxDevice();

    Core::registerInterruptHandler(IRQ_KEYBOARD, this);
    Core::registerInterruptHandler(IRQ_AUXILIARY_DEVICE, this);

    device->bind("keyboard", static_cast<es::Stream*>(&keyboardStream));
    device->bind("mouse", static_cast<es::Stream*>(&mouseStream));
}

Keyboard::
~Keyboard()
{
}

Object* Keyboard::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Keyboard::
addRef()
{
    return ref.addRef();
}

unsigned int Keyboard::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

u8 Keyboard::
writeAuxDevice(u8 byte)
{
    sendControllerCommand(WRITE_AUXILIARY_DEVICE);
    sendData(byte);
    return receiveData();
}

void Keyboard::
detectAuxDevice(void)
{
    try
    {
        writeAuxDevice(SET_DEFAULTS);
        writeAuxDevice(SET_STREAM_MODE);

        // Try to enter the scrolling wheel mode.
        writeAuxDevice(SET_SAMPLE_RATE);
        writeAuxDevice(200);
        writeAuxDevice(SET_SAMPLE_RATE);
        writeAuxDevice(100);
        writeAuxDevice(SET_SAMPLE_RATE);
        writeAuxDevice(80);
        writeAuxDevice(GET_DEVICE_ID);
        aux = receiveData();

        if (aux == 0x03)
        {
            // Try to enter the scrolling wheel plus 5 button mode.
            writeAuxDevice(SET_SAMPLE_RATE);
            writeAuxDevice(200);
            writeAuxDevice(SET_SAMPLE_RATE);
            writeAuxDevice(200);
            writeAuxDevice(SET_SAMPLE_RATE);
            writeAuxDevice(80);
        }

        writeAuxDevice(GET_DEVICE_ID);
        aux = receiveData();
        esReport("PS2 mouse: type = %02x\n", aux);
        switch (aux)
        {
        case 0x00:  // PS/2 mouse
        case 0x03:  // Intellimouse
        case 0x04:  // Intellimouse (5 button)
            writeAuxDevice(ENABLE_DATA_REPORTING);
            break;
        default:
            writeAuxDevice(DISABLE_DATA_REPORTING);
            break;
        }
    }
    catch (...)
    {
        esReport("PS2 mouse: not detected.\n");
    }
}

int Keyboard::
readKeyboard(void* dst, int count)
{
    Lock::Synchronized method(spinLock);

    if (count <= 0)
    {
        return 0;
    }
    if (1 + 256 < count)
    {
        count = 1 + 256;
    }

    u8* packet = static_cast<u8*>(dst);
    *packet++ = static_cast<u8>(map[KEYBOARD_LEFTCONTROL / (8 * sizeof(unsigned))] >>
                                (KEYBOARD_LEFTCONTROL % (8 * sizeof(unsigned))));
    int n = 1;
    for (int i = 0; i < 256 / (8 * sizeof(unsigned)); ++i)
    {
        unsigned bits = map[i];
        while (bits)
        {
            int j = ffs(bits) - 1;
            ASSERT(0 <= j);
            bits &= ~(1u << j);
            int key = 8 * sizeof(unsigned) * i + j;
            if (KEYBOARD_LEFTCONTROL <= key)
            {
                break;
            }
            *packet++ = static_cast<u8>(key);
            if (count <= ++n)
            {
                return count;
            }
        }
    }
    return n;
}

int Keyboard::
readMouse(void* dst, int count)
{
    Lock::Synchronized method(spinLock);

    if (count <= 0)
    {
        return 0;
    }
    if (1 + sizeof(axis) < count)
    {
        count = 1 + sizeof(axis);
    }
    u8* packet = static_cast<u8*>(dst);
    *packet = button;
    memmove(packet + 1, axis, count - 1);
    memset(axis, 0, sizeof(axis));
    return count;
}

Keyboard::
Stream::Stream(Keyboard* keyboard, Reader reader) :
    keyboard(keyboard),
    reader(reader)
{
}

long long Keyboard::
Stream::getPosition()
{
    return 0;
}

void Keyboard::
Stream::setPosition(long long pos)
{
}

long long Keyboard::
Stream::getSize()
{
    return 0;
}

void Keyboard::
Stream::setSize(long long size)
{
}

int Keyboard::
Stream::read(void* dst, int count)
{
    return (keyboard->*reader)(dst, count);
}

int Keyboard::
Stream::read(void* dst, int count, long long offset)
{
    return (keyboard->*reader)(dst, count);
}

int Keyboard::
Stream::write(const void* src, int count)
{
}

int Keyboard::
Stream::write(const void* src, int count, long long offset)
{
}

void Keyboard::
Stream::flush()
{
}

Object* Keyboard::
Stream::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
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

unsigned int Keyboard::
Stream::addRef()
{
    return keyboard->addRef();
}

unsigned int Keyboard::
Stream::release()
{
    return keyboard->release();
}
