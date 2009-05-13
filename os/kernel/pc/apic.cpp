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

#include "apic.h"
#include "core.h"
#include "io.h"
#include "rtc.h"

extern Interlocked esShutdownCount;

u8 Apic::idBSP = 0;             // boot-strap processor's local APIC id
volatile u32* Apic::localApic;  // memory-mapped address of local APIC
volatile bool Apic::online;
unsigned Apic::busClock;

void Apic::
setIoApicID(volatile u32* addr, u8 id)
{
    addr[IOREGSEL] = IOAPICID;
    u32 ioapicid = addr[IOWIN];
    ioapicid &= 0xf0ffffff;
    ioapicid |= (id & 0x0f) << 24;
    addr[IOWIN] = ioapicid;
}

void Apic::
enableLocalApic()
{
    u8 id = getLocalApicID();
    localApic[SVR] |= 0x100;
    localApic[DFR] = 0xffffffff;   // Using the flat model
    localApic[LDR] &= ~(0xff << 24);
    localApic[LDR] |= 1 << (id + 24);

    localApic[LVT_LINT0] = localApic[LVT_LINT0] | 0x10000;  // masked
    localApic[LVT_LINT1] = localApic[LVT_LINT1] | 0x10000;  // masked
    localApic[LVT_TMR] = localApic[LVT_TMR] | 0x10000;      // masked
    localApic[LVT_PCR] = localApic[LVT_PCR] | 0x10000;      // masked
}

u8 Apic::
getLocalApicVersion()
{
    return localApic[APICVER] & 0xff;
}

void Apic::
setImcr(u8 value)
{
    Mps::FloatingPointerStructure* fps = mps->getFloatingPointerStructure();
    if (fps->featureBytes[1] & 0x80)    // test IMCRP bit
    {
        outpb(0x22, 0x70);
        outpb(0x23, value);
    }
}

Apic::
Apic(Mps* mps) :
    mps(mps),
    hz(0)
{
    if (!mps)
    {
        return;
    }

    Mps::ConfigurationTableHeader* cth = mps->getConfigurationTableHeader();
    if (!cth)
    {
        localApic = reinterpret_cast<u32*>(0xfee00000);
    }
    else
    {
        localApic = reinterpret_cast<u32*>(cth->localApicAddress);
    }

    setImcr(0x01);  // Switch to symmetric I/O mode

    enableLocalApic();

    if (cth)
    {
        idBSP = getLocalApicID();
        setIoApicID(reinterpret_cast<u32*>(0xfec00000), 2);
    }
    else
    {
        Setup setup(this);
        cth->accept(setup);
    }
    esReport("BSP: %u, Local APIC %x\n", idBSP, localApic);
}

Apic::
~Apic()
{
}

int Apic::
startup(unsigned int bus, unsigned irq)
{
    return setAffinity(bus, irq, 0xff);
}

int Apic::
shutdown(unsigned int bus, unsigned irq)
{
    return disable(bus, irq);
}

int Apic::
enable(unsigned int bus, unsigned int irq)
{
    Mps::InterruptAssignment assignment;
    volatile u32* addr = mps->getInterruptAssignment(bus, irq, assignment);
    u8 intin;
    if (!addr || 23 < assignment.apicINTINn)
    {
        return -1;
    }
    else
    {
        u32 mode = (1 << 11) | (1 << 8) | (32 + assignment.apicINTINn);     // logical, lowest priority
        if (assignment.getTriggerMode() == 3)
        {
            mode |= (1 << 15);      // Level sensitive
        }
        if (assignment.getPolarity() == 3)
        {
            mode |= (1 << 13);      // Low active
        }
        addr[IOREGSEL] = IOREDTBL + 2 * assignment.apicINTINn;
        addr[IOWIN] = mode;
    }
    return 32 + assignment.apicINTINn;
}

int Apic::
disable(unsigned int bus, unsigned int irq)
{
    Mps::InterruptAssignment assignment;
    volatile u32* addr = mps->getInterruptAssignment(bus, irq, assignment);
    u8 intin;
    if (!addr || 23 < assignment.apicINTINn)
    {
        return -1;
    }
    else
    {
        intin = assignment.apicINTINn;
    }
    addr[IOREGSEL] = IOREDTBL + 2 * intin;
    addr[IOWIN] = 1 << 16;  // mask
    return 32 + assignment.apicINTINn;
}

bool Apic::
ack(int vec)
{
    ASSERT(32 <= vec && vec < 32 + 24);

    // Note IOAPIC ignores edge-sensitive interrupts (ISA) signaled on a
    // masked interrupt pin (i.e. does not deliver or hold pending). So we
    // do not mask irq here to keep the source code simple.

    // Note it appears qemu does not emulate ISR and we don't check ISR here.
    localApic[EOI] = 0;

    return true;
}

bool Apic::
end(int vec)
{
    return true;
}

int Apic::
setAffinity(unsigned int bus, unsigned int irq, unsigned int mask)
{
    Mps::InterruptAssignment assignment;
    volatile u32* addr = mps->getInterruptAssignment(bus, irq, assignment);
    u8 intin;
    if (!addr || 23 < assignment.apicINTINn)
    {
        return -1;
    }
    else
    {
        addr[IOREGSEL] = IOREDTBL + 2 * assignment.apicINTINn + 1;
        addr[IOWIN] = (mask << 24);
    }
    return 32 + assignment.apicINTINn;
}

// cf. HLT opcode is 0xf4
void Apic::
sendInit(u8 id, u32 addr)
{
    u32 r;
    long t;

    // Set shutdown code and warm-reset vector
    Rtc::cmosWrite(0x0f, 0x0a);
    *(u32*) ((0x40 << 4) | 0x67) = ((addr & 0xffff0) << 12) | (addr & 0x0f);

    // set the target AP id in ICRH
    r = localApic[ICRH];
    r &= 0xF0FFFFFF;
    r |= id << (56 - 32);
    localApic[ICRH] = r;

Retry:
    // Send a INIT message to the AP
    r = localApic[ICRL];
    r &= 0xFFF0F800;
    r |= 0x00000500;
    localApic[ICRL] = r;

    // Check delivery status for 20 microseconds
    esSleep(200);
    if (localApic[ICRL] & (1u << 12))
    {
        // Failed
        goto Retry;
    }
}

// addr must be page-aligned
void Apic::
sendStartup(u8 id, u32 addr)
{
    u32 r;
    long t;

    ASSERT((addr & ~0x000ff000) == 0);

    // Set the target AP id in ICRH
    r = localApic[ICRH];
    r &= 0xF0FFFFFF;
    r |= id << (56 - 32);
    localApic[ICRH] = r;

Retry:
    // Send a start-up IPI message to the AP
    r = localApic[ICRL];
    r &= 0xFFF0F800;
    r |= 0x00000600 | (addr >> 12);
    localApic[ICRL] = r;

    // Check delivery status for 20 microseconds
    esSleep(200);
    if (localApic[ICRL] & (1u << 12))
    {
        // Failed
        goto Retry;
    }
}

/** Broadcasts an IPI message to all excluding self
 */
void Apic::
broadcastIPI(u8 vec)
{
    if (!localApic)
    {
        return;
    }

    while (localApic[ICRL] & 0x1000)
    {
        // Wait during send pending
    }
    u32 r = localApic[ICRL];
    r &= 0xFFF0F800;
    r |= 0x000c4000 | vec;
    localApic[ICRL] = r;
}

void Apic::
startupAp(u8 id, u32 hltAP, u32 startAP)
{
    online = false;

    if (getLocalApicVersion() < 0x10)
    {
        // 82489DX external APIC
        sendInit(id, startAP);
    }
    else
    {
        // local APIC
        sendInit(id, hltAP);
        esSleep(100000);

        sendStartup(id, startAP);
        esSleep(2000);

        sendStartup(id, startAP);
        esSleep(2000);
    }

    // Wait till AP gets ready
    while (!online)
    {
        __asm__ __volatile__ ("pause\n");
    }
}

void Apic::
shutdownAp(u8 id, u32 hltAP)
{
#if 0
    setTimer(32, 0);   // stop timer
#endif
    sendInit(id, hltAP);
    esSleep(400000);
}

unsigned Apic::
splIdle()
{
    unsigned x = exchange(localApic + TPR, 0 << 4);
    __asm__ __volatile__ ("sti\n");
    return x;
}

unsigned Apic::
splLo()
{
    unsigned x = exchange(localApic + TPR, 1 << 4);
    __asm__ __volatile__ ("sti\n");
    return x;
}

unsigned Apic::
splHi()
{
    unsigned x = exchange(localApic + TPR, 15 << 4);
    __asm__ __volatile__ ("cli\n");
    return x;
}

void Apic::
splX(unsigned x)
{
    switch (x)
    {
    case 0 << 4:
        splIdle();
        break;
    case 1 << 4:
        splLo();
        break;
    case 15 << 4:
        splHi();
        break;
    default:
        esPanic(__FILE__, __LINE__, "inv. spl %d", x);
        break;
    }
}

//
// IInterface
//

Object* Apic::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Pic::iid()) == 0)
    {
        objectPtr = static_cast<es::Pic*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Pic*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Apic::
addRef()
{
    return ref.addRef();
}

unsigned int Apic::
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

int Apic::
readRtcCounter(int addr)
{
    do
    {
        outpb(Rtc::PORT_ADDR, Rtc::PORT_A);
    }
    while (inpb(Rtc::PORT_DATA) & 0x80);    // wait UIP
    outpb(Rtc::PORT_ADDR, addr);
    u8 bcd = inpb(Rtc::PORT_DATA);
    return (bcd & 0xf) + 10 * (bcd >> 4);
}

// Determine bus clock frequency
void Apic::
busFreq()
{
    int t0;
    int t1;
    int t2;

    localApic[DCR] &= ~0x0b;
    localApic[DCR] |= 0x0b;         // divide by 1

    localApic[LVT_TR] |= 0x10000;   // mask
    localApic[LVT_TR] &= ~0x20000;  // one-shot

    unsigned x = Core::splHi();
    t0 = readRtcCounter(Rtc::SECONDS);
    do {
        t1 = readRtcCounter(Rtc::SECONDS);
    } while (t0 == t1);
    localApic[ICR] = 0xffffffff;
    do {
        t2 = readRtcCounter(Rtc::SECONDS);
    } while (t2 == t1);
    unsigned count = localApic[CCR];
    Core::splX(x);

    busClock = 0xffffffffU - count;
    busClock = (busClock + 500000) / 1000000 * 1000000;
    esReport("Bus freq: %u %u\n", busClock, 0xffffffffU - count);

    localApic[ICR] = 1;
    while (0 < localApic[CCR])
    {
    }
}

// To stop timer, set hz = 0
void Apic::
setTimer(int vec, long hz)
{
    ASSERT(vec < 256);
    ASSERT(0 < busClock);
    localApic[LVT_TR] |= 0x10000;   // mask
    localApic[LVT_TR] &= ~0x200ff;  // one-shot
    if (0 < hz)
    {
        this->hz = hz;
        localApic[DCR] &= ~0x0b;
        localApic[DCR] |= 0x0b;                 // divide by 1
        localApic[ICR] = busClock / hz;
        localApic[LVT_TR] |= 0x20000 | vec;     // periodic
        localApic[LVT_TR] &= ~0x10000;          // unmask
    }
}

void Apic::
enableWatchdog()
{
    localApic[LVT_PCR] |= 0x10000;          // mask
    localApic[LVT_PCR] &= ~0x7ff;
    localApic[LVT_PCR] |= 0x400 | NO_NMI;   // NMI
    localApic[LVT_PCR] &= ~0x10000;         // unmask

    // wrmsr and rdpmc
    // ESCR: USR on, OS on, tag diable,
    // CCCR

    //
    // Architectural Performance Monitoring Version 1 Facilities
    //

    // IA32_PERFEVTSELx
    //

    // IA32_PERFEVTSELx MSRs
    //
    // Event Select 3CH, Umask 01H - Unhalted reference cycles
    // USR on, OS on, edge off, PC clear, INT set, EN set, INV clear
    // Counter mask 1

    // 186H 390 IA32_PERFEVTSEL0 Unique
    // 187H 391 IA32_PERFEVTSEL1 Unique
    // C1H 193 IA32_PMC0 Unique Performance counter register.
    // C2H 194 IA32_PMC1 Unique Performance counter register.

    // XXX Add overflow check. cf. Core 2 Duo/945G @ 266MHz
    wrmsr(IA32_PMC0, 0xffffffffu - 5 * busClock, 0xffffffffu);
    //  1 0101 0011b
    wrmsr(IA32_PERFEVTSEL0, 0x0153013c, 0x00000000);
}

//
// ICallback
//

#include "8254.h"

int Apic::
invoke(int)
{
    if (getLocalApicID() == 0)      // is bsp?
    {
        Pit::tick += 10000000 / hz;
        Alarm::invoke();
    }

    if (0 < esShutdownCount)
    {
        setTimer(0, 0);             // Stop local timer
        esShutdownCount.decrement();
        if (getLocalApicID() == 0)  // is bsp?
        {
            while (0 < esShutdownCount)
            {
                __asm__ __volatile__ ("pause\n");
            }
            Core::shutdown();
        }
        for (;;)
        {
            __asm__ __volatile__ ("hlt\n");
        }
    }

    return 0;
}
