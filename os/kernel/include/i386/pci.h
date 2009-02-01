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

#ifndef NINTENDO_ES_KERNEL_I386_PCI_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_PCI_H_INCLUDED

#include <es/types.h>
#include <es/naming/IContext.h>
#include "io.h"
#include "mps.h"



class Pci
{
    static const int CONFIG_ADDRESS = 0xcf8;
    static const int CONFIG_DATA = 0xcfc;

    static const u32 RESERVED = 0x7f000000;
    static const u32 CFGE = 0x80000000;         // Configuration Enable

    Mps*        mps;
    int         maxDevice;
    es::Context*   device;

public:
    struct ConfigurationSpaceHeader
    {
        u16 vendorID;
        u16 deviceID;
        u16 command;
        u16 status;
        u32 classCode;      // with Revision ID (7:0)
        u8  cacheLineSize;
        u8  latencyTimer;
        u8  headerType;
        u8  bist;
        u32 baseAddressRegisters[6];
        u32 cardbusCISPointer;
        u16 subsystemVendorID;
        u16 subsystemID;
        u32 expansionROMBaseAddress;
        u8  capabilitiesPointer;
        u8  reserved1;
        u8  reserved2;
        u8  reserved3;
        u32 reserved4;
        u8  interruptLine;
        u8  interruptPin;
        u8  minGnt;
        u8  maxLat;

        ConfigurationSpaceHeader(u32 tag)
        {
            u32* ptr = reinterpret_cast<u32*>(this);
            int reg = 0;
            *ptr++ = read(tag, reg);
            if (deviceID == 0xffff)
            {
                return;
            }
            while ((reg += 4) < 64)
            {
                *ptr++ = read(tag, reg);
            }
        }

        void report()
        {
            esReport("  device ID: %04x\n", deviceID);
            esReport("  vendor ID: %04x\n", vendorID);
            esReport("  command: %04x\n", command);
            esReport("  status: %04x\n", status);
            esReport("  class code: %08x\n", classCode);
            esReport("  base address register: %08x %08x\n", baseAddressRegisters[0],
                                                             baseAddressRegisters[1]);
            esReport("  subsystem ID: %04x\n", subsystemID);
            esReport("  subsystem vendor ID: %04x\n", subsystemVendorID);
            esReport("  interrupt line: %d\n", interruptLine);
            esReport("  interrupt pin: %d\n", interruptPin);
        }
    };

    Pci(Mps* mps, es::Context* device);
    ~Pci();

    static u32 tag(u8 bus, u8 device, u8 func)
    {
        device &= 0x1f;
        func &= 0x7;
        return CFGE | (bus << 16) | (device << 11) | (func << 8);
    }

    static u32 read(u32 tag, u32 reg)
    {
        outpl(CONFIG_ADDRESS, tag | (reg & 0xfc));
        u32 data = inpl(CONFIG_DATA);
        outpl(CONFIG_ADDRESS, 0);
        return data;
    }

    static void write(u32 tag, u32 reg, u32 data)
    {
        outpl(CONFIG_ADDRESS, tag | (reg & 0xfc));
        outpl(CONFIG_DATA, data);
        outpl(CONFIG_ADDRESS, 0);
    }

    void scan();

    void attach(int bus, int device, ConfigurationSpaceHeader* csp);
};

#endif // NINTENDO_ES_KERNEL_I386_PCI_H_INCLUDED
