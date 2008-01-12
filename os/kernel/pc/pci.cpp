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

#define VERBOSE

Pci::
Pci(IContext* device) :
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
attach(ConfigurationSpaceHeader* csp)
{
    int nicCount = 0;

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
                Dp8390d* ne2000 = new Dp8390d(csp->baseAddressRegisters[0] & ~1,
                                              csp->interruptLine);
                device->bind("ethernet", static_cast<IStream*>(ne2000));
                ne2000->release();
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
        for (int device = 0; device < maxDevice; ++device)
        {
            ConfigurationSpaceHeader csh(tag(bus, device, 0));
            if (csh.deviceID == 0xffff)
            {
                continue;
            }
            attach(&csh);

#ifdef VERBOSE
            esReport("PCI %d:%d\n", bus, device);
            csh.report();
#endif

            if (!(csh.headerType & 0x80))   // not multi-function device?
            {
                continue;
            }
            for (u8 func = 1; func < 8; ++func)
            {
                ConfigurationSpaceHeader csh(tag(bus, device, func));
                if (csh.deviceID == 0xffff)
                {
                    continue;
                }
                attach(&csh);

#ifdef VERBOSE
                esReport("PCI: %d:%d:%d\n", bus, device, func);
                csh.report();
#endif
            }
        }
    }
}
