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

#include <es.h>
#include "i386/pci.h"
#include "i386/dp8390d.h"
#include "i386/es1370.h"

#define VERBOSE

Pci::
Pci(Mps* mps, IContext* device) :
    mps(mps),
    maxDevice(0),
    device(device)
{
    ASSERT(sizeof(ConfigurationSpaceHeader) == 64);

    if (device)
    {
        device->addRef();
    }

    u32 org = inpl(CONFIG_ADDRESS);
    if ((org & RESERVED) == 0)
    {
        outpl(CONFIG_ADDRESS, CFGE);
        u32 res = inpl(CONFIG_ADDRESS);
        outpl(CONFIG_ADDRESS, org);
        if (res & CFGE)
        {
            maxDevice = 32;
            scan();
        };
    }
}

Pci::
~Pci()
{
    if (device)
    {
        device->release();
    }
}

void Pci::
attach(int bus, int dev, ConfigurationSpaceHeader* csp)
{
    int nicCount = 0;
    int soundCount = 0;
    u8 irq;

    if (!mps->getFloatingPointerStructure())
    {
        irq = csp->interruptLine;
    }
    else
    {
        // Convert to MPC irq spec.
        ASSERT(dev < 32);
        irq = dev << 2;
        if (1 <= csp->interruptLine && csp->interruptLine <= 4)
        {
            irq |= (csp->interruptPin - 1);
        }
    }

    switch (csp->classCode >> 16)
    {
    case 0x0200:    // Ethernet network adapter
        if (csp->vendorID == 0x10EC && csp->deviceID == 0x8029)
        {
            if (++nicCount == 1)
            {
                // Realtek Semiconductor, RTL8029(AS)
                esReport("Realtek Semiconductor, RTL8029(AS) Ethernet Adapter (%x, %d)\n",
                         csp->baseAddressRegisters[0] & ~1, csp->interruptLine);
                Dp8390d* ne2000 = new Dp8390d(bus,
                                              csp->baseAddressRegisters[0] & ~1,
                                              irq);
                device->bind("ethernet", static_cast<IStream*>(ne2000));
                ne2000->release();
            }
        }
        break;

    case 0x0401: // Audio device
        if (csp->vendorID == 0x1274 /* Ensoniq */ && csp->deviceID == 0x5000)
        {
            if (++soundCount == 1)
            {
                u32 command;
                u32 tagValue = tag(bus, dev, 0);
                command = read(tagValue, 0x04);
                write(tagValue, 0x04, command | 0x0004 | 0x0002 | 0x0001); // enable bus master.

                // ENSONIQ, AudioPCI ES1370
                esReport("ENSONIQ, AudioPCI ES1370 Sound Ahdapter (%x, %d)\n",
                         csp->baseAddressRegisters[0] & ~1, csp->interruptLine);
                Es1370* es1370 = new Es1370(bus,
                                            csp->baseAddressRegisters[0] & ~1,
                                            irq);
                ASSERT(static_cast<IStream*>(&es1370->inputLine));
                ASSERT(static_cast<IStream*>(&es1370->outputLine));
                device->bind("soundInput", static_cast<IStream*>(&es1370->inputLine));
                device->bind("soundOutput", static_cast<IStream*>(&es1370->outputLine));
                es1370->release();
            }
        }
        break;
    }
}

void Pci::
scan()
{
    for (int bus = 0; bus <= 3; ++bus)
    {
        for (int dev = 0; dev < maxDevice; ++dev)
        {
            ConfigurationSpaceHeader csh(tag(bus, dev, 0));
            if (csh.deviceID == 0xffff)
            {
                continue;
            }
            attach(bus, dev, &csh);

#ifdef VERBOSE
            esReport("PCI %d:%d\n", bus, dev);
            csh.report();
#endif

            if (!(csh.headerType & 0x80))   // not multi-function device?
            {
                continue;
            }
            for (u8 func = 1; func < 8; ++func)
            {
                ConfigurationSpaceHeader csh(tag(bus, dev, func));
                if (csh.deviceID == 0xffff)
                {
                    continue;
                }
                attach(bus, dev, &csh);

#ifdef VERBOSE
                esReport("PCI: %d:%d:%d\n", bus, dev, func);
                csh.report();
#endif
            }
        }
    }
}
