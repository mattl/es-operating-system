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

#ifndef NINTENDO_ES_KERNEL_I386_APIC_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_APIC_H_INCLUDED

#include <es.h>
#include <es/ref.h>
#include <es/base/ICallback.h>
#include <es/device/IPic.h>
#include "mps.h"

class Apic : public es::Pic, public es::Callback
{
    // Memory mapped registers for accessing IOAPIC registers
    static const int IOREGSEL  = 0x00 / sizeof(u32);
    static const int IOWIN     = 0x10 / sizeof(u32);

    // IOAPIC registers
    static const int IOAPICID  = 0x00;
    static const int IOAPICVER = 0x01;
    static const int IOAPICARB = 0x02;
    static const int IOREDTBL  = 0x10;

    // Local APIC registers
    static const int APICID    = 0x0020 / sizeof(u32);      // Local APIC ID Register
    static const int APICVER   = 0x0030 / sizeof(u32);      // Local APIC Version Register
    static const int TPR       = 0x0080 / sizeof(u32);      // Task Priority Register
    static const int APR       = 0x0090 / sizeof(u32);      // Arbitration Priority Register
    static const int PPR       = 0x00a0 / sizeof(u32);      // Processor Priority Register
    static const int EOI       = 0x00b0 / sizeof(u32);      // EOI Register
    static const int LDR       = 0x00d0 / sizeof(u32);      // Logical Destination Register
    static const int DFR       = 0x00e0 / sizeof(u32);      // Destination Format Register
    static const int SVR       = 0x00f0 / sizeof(u32);      // Spurious Interrupt Vector Register
    static const int ISR       = 0x0100 / sizeof(u32);      // In-Service Register
    static const int TMR       = 0x0180 / sizeof(u32);      // Trigger Mode Register
    static const int IRR       = 0x0200 / sizeof(u32);      // Interrupt Request Register
    static const int ESR       = 0x0280 / sizeof(u32);      // Error Status Register
    static const int ICRL      = 0x0300 / sizeof(u32);      // Interrupt Command Register [0-31]
    static const int ICRH      = 0x0310 / sizeof(u32);      // Interrupt Command Register [32-63]
    static const int LVT_TR    = 0x0320 / sizeof(u32);      // LVT Timer Register
    static const int LVT_TMR   = 0x0330 / sizeof(u32);      // LVT Thermal Sensor Register
    static const int LVT_PCR   = 0x0340 / sizeof(u32);      // LVT Performance Monitoring Counters Register
    static const int LVT_LINT0 = 0x0350 / sizeof(u32);      // LVT LINT0 Register
    static const int LVT_LINT1 = 0x0360 / sizeof(u32);      // LVT LINT1 Register
    static const int LVT_ER    = 0x0370 / sizeof(u32);      // LVT Error Register
    static const int ICR       = 0x0380 / sizeof(u32);      // Initial Count Register
    static const int CCR       = 0x0390 / sizeof(u32);      // Current Count Register
    static const int DCR       = 0x03e0 / sizeof(u32);      // Divide Configuration Register

    static u8               idBSP;              // boot-strap processor's local APIC id
    static volatile u32*    localApic;          // memory-mapped address of local APIC
    static volatile bool    online;
    static unsigned         busClock;

    Ref         ref;
    Mps*        mps;
    unsigned    hz;         // for counter 0

    static void setIoApicID(volatile u32* addr, u8 id);
    /** Get local APIC version.
     * @return 1Xh: Local APIC. 0Xh: 82489DX external APIC.
     */
    static u8 getLocalApicVersion();
    void setImcr(u8 value);

    void sendInit(u8 id, u32 addr);
    void sendStartup(u8 id, u32 addr);

    /** Start-up the application processor
     * @param id        Local APIC ID of the application processor
     * @param hltAP     the address of hlt routine
     * @param startAP   the address of AP startup routine
     */
    void startupAp(u8 id, u32 hltAP, u32 startAP);
    void shutdownAp(u8 id, u32 hltAP);

    static u32 exchange(volatile u32* addr, u32 value)
    {
        __asm__ __volatile__ (
            "xchgl  %0, (%1)\n"
            : "=a" (value) : "r" (addr), "0" (value) );
        return value;
    }

    class Setup : public Mps::Visitor
    {
        Apic* apic;
    public:
        Setup(Apic* apic) :
            apic(apic)
        {
        }
        bool at(const Mps::Processor* processor)
        {
            if (processor->isUsable() && processor->isBootstrapProcessor())
            {
                apic->idBSP = processor->id;
            }
            return true;
        }
        bool at(const Mps::IOApic* ioApic)
        {
            if (ioApic->isUsable())
            {
                apic->setIoApicID(reinterpret_cast<u32*>(ioApic->address), ioApic->id);
            }
            return true;
        }
        bool at(const Mps::InterruptAssignment* interrupt)
        {
            return true;
        }
    };

    class Startup : public Mps::Visitor
    {
        Apic* apic;
        u32 hltAP;
        u32 startAP;
    public:
        Startup(Apic* apic, u32 hltAP, u32 startAP) :
            apic(apic),
            hltAP(hltAP),
            startAP(startAP)
        {
        }
        bool at(const Mps::Processor* processor)
        {
            if (processor->isUsable() && !processor->isBootstrapProcessor())
            {
                apic->startupAp(processor->id, hltAP, startAP);
            }
            return true;
        }
    };

public:
    Apic(Mps* mps);
    ~Apic();

    static int readRtcCounter(int addr);
    static void busFreq();

    void setTimer(int vec, long hz);
    void enableWatchdog();

    void startupAllAP(u32 hltAP, u32 startAP)
    {
        Mps::ConfigurationTableHeader* cth = mps->getConfigurationTableHeader();
        if (!cth)
        {
            startupAp(idBSP ? 0 : 1, hltAP, startAP);
        }
        else
        {
            Startup v(this, hltAP, startAP);
            cth->accept(v);
        }
    }

    static void broadcastIPI(u8 vec);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    // IPic
    int startup(unsigned int bus, unsigned int irq);
    int shutdown(unsigned int bus, unsigned int irq);
    int enable(unsigned int bus, unsigned int irq);
    int disable(unsigned int bus, unsigned int irq);
    bool ack(int vec);
    bool end(int vec);
    int setAffinity(unsigned int bus, unsigned int irq, unsigned int mask);
    unsigned int splIdle();
    unsigned int splLo();
    unsigned int splHi();
    void splX(unsigned int x);

    // ICallback
    int invoke(int);

    static u8 getLocalApicID()
    {
        return localApic ? (localApic[APICID] >> 24) : 0;
    }
    static void enableLocalApic();
    static void started()
    {
        online = true;
    }
};

#endif // NINTENDO_ES_KERNEL_I386_APIC_H_INCLUDED
