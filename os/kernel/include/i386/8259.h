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

#ifndef NINTENDO_ES_KERNEL_I386_8259_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_8259_H_INCLUDED

#include <es/device/IPic.h>
#include <es/ref.h>



class Pic : public es::Pic
{
    Ref ref;
    u8  imrMaster;
    u8  imrSlave;

public:
    // Constructor
    Pic();

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    // IPic
    int startup(unsigned bus, unsigned irq);
    int shutdown(unsigned bus, unsigned irq);
    int enable(unsigned bus, unsigned irq);
    int disable(unsigned bus, unsigned irq);
    bool ack(int vec);
    bool end(int vec);
    int setAffinity(unsigned bus, unsigned irq, unsigned int cpuMask);
    unsigned int splIdle();
    unsigned int splLo();
    unsigned int splHi();
    void splX(unsigned int x);

    static const int PORT_MASTER =  0x20;
    static const int PORT_MASTER_IMR =  0x21;
    static const int PORT_SLAVE = 0xa0;
    static const int PORT_SLAVE_IMR = 0xa1;

    static const u8 OCW2 = 0x00;
    static const u8 OCW2_SPECIFIC_EOI = 0x60;
    static const u8 OCW3 = 0x08;
    static const u8 OCW3_ISR = OCW3 | 3;
    static const u8 OCW3_IRR = OCW3 | 2;
    static const u8 ICW1 = 0x10;

private:
    int readISR(unsigned irq);
};

#endif // NINTENDO_ES_KERNEL_I386_8259_H_INCLUDED
