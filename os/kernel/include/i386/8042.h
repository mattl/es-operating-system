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

#ifndef NINTENDO_ES_KERNEL_I386_8042_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_8042_H_INCLUDED

// 8042 keyboard controller

#include <es.h>
#include <es/ref.h>
#include <es/base/ICallback.h>
#include <es/base/IStream.h>
#include <es/naming/IContext.h>
#include "thread.h"

class Keyboard : public es::Callback
{
    typedef int (Keyboard::*Reader)(void* dst, int count);

    class Stream : public es::Stream
    {
        Keyboard*   keyboard;
        Reader      reader;

    public:
        Stream(Keyboard* keyboard, Reader reader);
        long long getPosition();
        void setPosition(long long pos);
        long long getSize();
        void setSize(long long size);
        int read(void* dst, int count);
        int read(void* dst, int count, long long offset);
        int write(const void* src, int count);
        int write(const void* src, int count, long long offset);
        void flush();

        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    // Port address
    static const u8 DATA_BUFFER = 0x60;
    static const u8 COMMAND_BYTE = 0x64;    // write
    static const u8 STATUS_BYTE = 0x64;     // read

    // Keyboard controller command byte
    static const u8 ENABLE_KEYBOARD_INTERRUPT = 0x01;
    static const u8 ENABLE_AUXILIARY_INTERRUPT = 0x02;
    static const u8 SYSTEM_FLAG = 0x04;
    static const u8 DISABLE_KEYBOARD = 0x08;
    static const u8 DISABLE_AUXILIARY_DEVICE = 0x10;
    static const u8 IBM_PERSONAL_COMPUTER_MODE = 0x10;
    static const u8 IBM_KEYBOARD_TRANSLATE_MODE = 0x20;

    // Keyboard status controller status byte
    static const u8 OUTPUT_BUFFER_FULL = 0x01;
    static const u8 INPUT_BUFFER_FULL = 0x02;
    // static const u8 SYSTEM_FLAG = 0x04;
    static const u8 COMMAND_DATA = 0x08;
    static const u8 INHIBIT_SWITCH = 0x10;
    static const u8 AUXILIARY_OUTPUT_BUFFER_FULL = 0x20;
    static const u8 TRANSMIT_TIME_OUT = 0x20;
    static const u8 GENERAL_TIME_OUT = 0x40;
    static const u8 PARITY_ERROR = 0x80;

    // Keyboard controller command
    static const u8 SELF_TEST = 0xaa;
    static const u8 INTERFACE_TEST = 0xab;
    static const u8 READ_KEYBOARD_CONTROLLER_COMMAND_BYTE = 0x20;
    static const u8 WRITE_KEYBOARD_CONTROLLER_COMMAND_BYTE = 0x60;
    static const u8 WRITE_AUXILIARY_DEVICE = 0xd4;

    // Keyboard controller command
    static const u8 RESET = 0xff;
    static const u8 SET_MODE_INDICATORS = 0xed;

    // Mouse command
    static const u8 SET_DEFAULTS = 0xf6;
    static const u8 DISABLE_DATA_REPORTING = 0xf5;
    static const u8 ENABLE_DATA_REPORTING = 0xf4;
    static const u8 SET_SAMPLE_RATE = 0xf3;
    static const u8 GET_DEVICE_ID = 0xf2;
    static const u8 SET_STREAM_MODE = 0xea;

    // IRQ
    static const u8 IRQ_KEYBOARD = 1;
    static const u8 IRQ_AUXILIARY_DEVICE = 12;

    Ref         ref;
    Lock        spinLock;
    u8          cmd;

    // Keyboard
    u8          esc;
    unsigned    map[256 / (8 * sizeof(unsigned))];
    static u8   keycode[128];
    static u8   keycodeEsc[128];

    // Aux device
    u8          aux;        // deVice type
    u8          packet[4];
    u8          count;
    u8          button;
    s8          axis[3];

    // Streams
    Stream      keyboardStream;
    Stream      mouseStream;

    u8 sendControllerCommand(u8 cmd);
    void sendData(u8 data);
    u8 receiveData(int retry = 10000);
    void reset(void);

    u8 writeAuxDevice(u8 byte);
    void detectAuxDevice(void);

    int readKeyboard(void* dst, int count);
    int readMouse(void* dst, int count);

    static s8 clamp(int n);

public:
    Keyboard(es::Context* device);
    ~Keyboard();

    void setLED(u8 led);

    // ICallback
    int invoke(int param);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // NINTENDO_ES_KERNEL_I386_8042_H_INCLUDED
