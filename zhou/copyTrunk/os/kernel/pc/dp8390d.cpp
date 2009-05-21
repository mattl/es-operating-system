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
/*
 * These coded instructions, statements, and computer programs contain
 * software derived from the following specification:
 *
 * National Semiconductor, "DP8390D/NS32490D NIC Network Interface
 * Controller", July 1995.
 * http://www.national.com/opf/DP/DP8390D.html
 *
 * REALTEK SEMICONDUCTOR CORP., "RTL8029AS Realtek PCI Full-Duplex Ethernet
 * Controller with built-in SRAM ADVANCED INFORMATION", Janualy 1997.
 */

#include <string.h>
#include <errno.h>
#include <es/types.h>
#include <es/formatter.h>
#include <es/exception.h>
#include "i386/dp8390d.h"
#include "i386/io.h"
#include "i386/core.h"

// #define VERBOSE

bool Dp8390d::
reset()
{
    // Reset
    u8 rst = inpb(base + RESET);
    esSleep(20);
    outpb(base + RESET, rst);
    esSleep(20);

    // RTL8029AS has 16KB SRAM built in.
    ramStart = 16 * 1024;
    ramEnd = 32 * 1024;

    return true;
}

bool Dp8390d::
readMacAddress()
{
    u8 prom[32];

    remoteRead(0, prom, sizeof(prom));

#ifdef VERBOSE
    esDump(prom, 32);
#endif

    if (prom[0x1c] != 'W')  // Check data bus type.
    {
        // non 16 bit data bus
        return false;
    }

    // Copy mac address.
    for (int i = 0; i < 6; ++i)
    {
        mac[i] = prom[2 * i];
    }

#ifdef VERBOSE
    esReport("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif

    return true;
}

bool Dp8390d::
initialize()
{
    txPageStart = ramStart / PAGE_SIZE;
    pageStart = txPageStart + NUM_TX_PAGE;
    pageStop = ramEnd / PAGE_SIZE;
    nextPacket = pageStart + 1;

    // Program Command Register for page 0
    outpb(base + CR, CR_STP | CR_RD2 | CR_PAGE0);

    // Initialize Data Configuration Register (DCR)
    outpb(base + DCR, DCR_FT1 | DCR_LS);

    // Clear Remote Byte Count Registers (RBCR0, RBCR1)
    outpb(base + RBCR0, 0);
    outpb(base + RBCR1, 0);

    // Initialize Receive Configuration Register (RCR)
    rcr = RCR_AB | RCR_AM;
    outpb(base + RCR, rcr);

    // Place the NIC in loopback mode.
    outpb(base + TCR, TCR_LB0);

    // Initialize Receive Buffer Ring: Boundary Pointer (BNDRY), Page Start (PSTART), and Page Stop (PSTOP)
    outpb(base + PSTART, pageStart);
    outpb(base + PSTOP, pageStop);
    outpb(base + BNRY, pageStart);

    // Clear Interrupt Status Register (ISR) by writing 0FFh to it.
    outpb(base + ISR, 0xff);

    // Initialize Interrupt Mask Register
    outpb(base + IMR, IMR_CNTE | IMR_OVWE | IMR_TXEE | IMR_RXEE | IMR_PTXE | IMR_PRXE);

    if (!readMacAddress())
    {
        return false;
    }

    {
        Lock::Synchronized method(spinLock);

        // Program Command Register for page 1 (Command Register = 61H)
        u8 cr = setPage(CR_PAGE1);

        // Initialize Physical Address Registers (PAR0-PAR5)
        outpb(base + PAR0, mac[0]);
        outpb(base + PAR1, mac[1]);
        outpb(base + PAR2, mac[2]);
        outpb(base + PAR3, mac[3]);
        outpb(base + PAR4, mac[4]);
        outpb(base + PAR5, mac[5]);

        // Initialize Multicast Address Registers (MAR0-MAR7)
        for (int i = 0; i < NUM_HASH_REGISTER; ++i)
        {
            outpb(base + MAR0 + i, hashTable[i]);
        }

        // Initialize CURRent pointer
        outpb(base + CURR, nextPacket);

        restorePage(cr);
    }

    outpb(base + TPSR, txPageStart);

    return true;
}

int Dp8390d::
getPacketSize(RingHeader* header)
{
    int lenHigh;    // The upper byte count

    if (nextPacket < header->nextPage)
    {
        lenHigh = header->nextPage - nextPacket - 1;
    }
    else
    {
        lenHigh = pageStop - nextPacket + header->nextPage - pageStart - 1;
    }
    if (header->lenLow > PAGE_SIZE - sizeof(RingHeader))
    {
        ++lenHigh;
    }
    return (lenHigh << 8) | header->lenLow; // Including FCS bytes.
}

int Dp8390d::
updateRing(u8 nextPage)
{
    nextPacket = nextPage;
    if (--nextPage < pageStart)
    {
        nextPage = pageStop - 1;
    }
    outpb(base + BNRY, nextPage);
}

bool Dp8390d::
isRingEmpty()
{
    Lock::Synchronized method(spinLock);

    u8 cr = setPage(CR_PAGE1);
    u8 curr = inpb(base + CURR);
    restorePage(cr);
    return (curr == nextPacket) ? true : false;
}

unsigned int Dp8390d::
generateCrc(const u8 mca[6])
{
    unsigned idx;
    unsigned bit;
    unsigned data;
    unsigned crc = 0xffffffff;

    for (idx = 0; idx < 6; idx++)
    {
        for (data = *mca++, bit = 0; bit < 8; bit++, data >>=1)
        {
            crc = (crc >> 1) ^ (((crc ^ data) & 1) ? POLY : 0);
        }
    }
    return crc;
}

// Note spinLock must be locked before calling recover().
void Dp8390d::
recover()
{
    // Clear Remote Byte Count Registers.
    outpb(base + RBCR0, 0);
    outpb(base + RBCR1, 0);

    if (resend && (!sending || ((inpb(base + ISR) & (ISR_PTX | ISR_TXE)))))
    {
        resend = false;
    }

    outpb(base + TCR, TCR_LB0); // Put NIC in loopback
    outpb(base + CR, CR_RD2 | CR_STA | CR_PAGE0); // Issue start command.

    // Remove packet(s)
    nextPacket = pageStart + 1;
    outpb(base + BNRY, pageStart);
    u8 cr = setPage(CR_PAGE1);
    outpb(base + CURR, nextPacket);
    restorePage(cr);

    outpb(base + TCR, 0);       // Take NIC out of loopback

    if (resend)
    {
        outpb(base + CR, CR_RD2 | CR_TXP | CR_STA | CR_PAGE0);
    }

    overflow = false;
    monitor->notifyAll();
}

int Dp8390d::
remoteRead(unsigned short src, void* buf, unsigned short len)
{
    ASSERT(0 < len);
    u8 cr = inpb(base + CR) & ~CR_TXP;  // Save

    // select page 0
    outpb(base + CR, CR_RD2 | CR_STA | CR_PAGE0);

    // set Remote DMA Byte Count.
    outpb(base + RBCR0, 0xff & len);
    outpb(base + RBCR1, len >> 8);

    // set Remote Start Address.
    outpb(base + RSAR0, 0xff & src);
    outpb(base + RSAR1, src >> 8);

    outpb(base + CR, CR_RD0 | CR_STA | CR_PAGE0);

    inpsb(base + DATA, buf, len);

    outpb(base + ISR, ISR_RDC);

    outpb(base + CR, cr);               // Restore
    return len;
}

int Dp8390d::
remoteWrite(unsigned short dst, const void* buf, unsigned short len)
{
    int plen = (len < 60) ? 60 : len;   // Excluding FCS

    // select page 0
    outpb(base + CR, CR_RD2 | CR_STA | CR_PAGE0);

    // clear interrupt status.
    outpb(base + ISR, ISR_RDC);

    // set Remote DMA Byte Count.
    outpb(base + RBCR0, plen);
    outpb(base + RBCR1, plen >> 8);

    // set remote DMA start address.
    outpb(base + RSAR0, 0xff & dst);
    outpb(base + RSAR1, dst >> 8);

    // start DMA.
    outpb(base + CR, CR_RD1 | CR_STA | CR_PAGE0);

    outpsb(base + DATA, buf, len);
    while (len++ < plen)
    {
        outpb(base + DATA, 0);
    }

    // Busy wait for the DMA completion. Note it appears qemu does not support RDC interrupts.
    while ((inpb(base + ISR) & ISR_RDC) != ISR_RDC)
    {
#if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__ ("pause\n");
#endif
    }
    outpb(base + ISR, ISR_RDC); // Clear

    // Send this packet
    outpb(base + TBCR0, plen & 0xff);
    outpb(base + TBCR1, (plen >> 8) & 0xff);
    outpb(base + CR, CR_RD2 | CR_TXP | CR_STA | CR_PAGE0);

    return plen;
}

u8 Dp8390d::
setPage(int page)
{
    // page: CR_PAGE[0-2]
    u8 cr = inpb(base + CR) & ~CR_TXP;
    outpb(base + CR, (cr & ~(CR_PS0 | CR_PS1)) | page);
    return cr;
}

void Dp8390d::
restorePage(u8 cr)
{
    outpb(base + CR, cr);
}

Dp8390d::
Dp8390d(u8 bus, unsigned base, int irq) :
    bus(bus),
    base(base),
    irq(irq),
    enabled(false),
    sending(0),
    overflow(false)
{
    monitor = es::Monitor::createInstance();

    alarm = es::Alarm::createInstance();
    alarm->setEnabled(false);
    alarm->setInterval(160000);
    alarm->setPeriodic(false);
    alarm->setCallback(this);

    memset(mac, 0, 6);
    memset(hashTable, 0, sizeof(hashTable));
    memset(hashRef, 0, sizeof(hashRef));

    if (!reset())
    {
        throw SystemException<ENODEV>();
    }

    if (!initialize())
    {
        throw SystemException<ENODEV>();
    }

    Core::registerInterruptHandler(bus, irq, this);

    memset(&statistics, 0, sizeof(statistics));
}

Dp8390d::
~Dp8390d()
{
    Core::unregisterInterruptHandler(bus, irq, this);
    if (monitor)
    {
        monitor->release();
    }
    if (alarm)
    {
        alarm->setEnabled(false);
        alarm->release();
    }
}

//
// INetworkInterface
//

int Dp8390d::
start()
{
    Synchronized<es::Monitor*> method(monitor);

    if (enabled)
    {
        return 0;
    }
    enabled = true;

    // Put NIC in START mode (Command Register 22H).
    outpb(base + CR, CR_RD2 | CR_STA | CR_PAGE0);

    // Initialize TCR to be ready for transmission and reception.
    outpb(base + TCR, 0);

    return 0;
}

int Dp8390d::
stop()
{
    Synchronized<es::Monitor*> method(monitor);

    if (!enabled)
    {
        return 0;
    }
    enabled = false;

    while (overflow || sending)
    {
        monitor->wait();
    }

    outpb(base + CR, CR_RD2 | CR_STP | CR_PAGE0);   // Issue the stop command
    esSleep(160000);                                // Wait for at least 1.6 ms
    outpb(base + RBCR0, 0);                         // Clear RBC0 and RBC1
    outpb(base + RBCR1, 0);
    outpb(base + TCR, TCR_LB0);                     // Put NIC in loopback

    // Remove packet(s)
    nextPacket = pageStart + 1;
    outpb(base + BNRY, pageStart);
    {
        Lock::Synchronized method(spinLock);
        u8 cr = setPage(CR_PAGE1);
        outpb(base + CURR, nextPacket);
        restorePage(cr);
    }

    monitor->notifyAll();

    return 0;
}

bool Dp8390d::
getPromiscuousMode()
{
    Synchronized<es::Monitor*> method(monitor);

    return rcr & RCR_PRO;
}

void Dp8390d::
setPromiscuousMode(bool on)
{
    Synchronized<es::Monitor*> method(monitor);

    if (on == ((rcr & RCR_PRO) ? true : false))
    {
        return;
    }

    if (on)
    {
        Lock::Synchronized method(spinLock);

        rcr |= RCR_PRO;
        outpb(base + RCR, rcr);

        // Program Command Register for page 1
        u8 cr = setPage(CR_PAGE1);
        for (int i = 0; i < NUM_HASH_REGISTER; ++i)
        {
            outpb(base + MAR0 + i, 0xff);
        }
        restorePage(cr);
    }
    else
    {
        Lock::Synchronized method(spinLock);

        rcr &= ~RCR_PRO;
        outpb(base + RCR, rcr);

        // Program Command Register for page 1
        u8 cr = setPage(CR_PAGE1);
        for (int i = 0; i < NUM_HASH_REGISTER; ++i)
        {
            outpb(base + MAR0 + i, hashTable[i]);
        }
        restorePage(cr);
    }
}

int Dp8390d::
addMulticastAddress(const u8 macaddr[6])
{
    Synchronized<es::Monitor*> method(monitor);

    if (!(*macaddr & 0x01))
    {
        return -1;
    }

    // Get the 6 most significant bits (little endian).
    int msb = 0x3f & generateCrc(macaddr);
    msb = ((msb & 1) << 5) | ((msb & 2) << 3) |
          ((msb & 4) << 1) | ((msb & 8) >> 1) |
          ((msb & 16) >> 3) | ((msb & 32) >> 5);

    // increment reference count.
    if (++hashRef[msb] == 1)
    {
        Lock::Synchronized method(spinLock);

        // Program Command Register for page 1
        u8 cr = setPage(CR_PAGE1);

        u8 mar = inpb(base + MAR0 + msb/8);
        u8 bit = 1<<(msb % 8);

        if (!(mar & bit))
        {
            mar |= bit;
            outpb(base + MAR0 + msb/8, mar);
            hashTable[msb/8] = mar;
        }

        restorePage(cr);
    }

    return 0;
}

int Dp8390d::
removeMulticastAddress(const u8 macaddr[6])
{
    Synchronized<es::Monitor*> method(monitor);

    if (!(*macaddr & 0x01))
    {
        return -1;
    }

    // Get the 6 most significant bits.
    int msb = 0x3f & generateCrc(macaddr);
    msb = ((msb & 1) << 5) | ((msb & 2) << 3) |
          ((msb & 4) << 1) | ((msb & 8) >> 1) |
          ((msb & 16) >> 3) | ((msb & 32) >> 5);

    if (--hashRef[msb] <= 0)
    {
        Lock::Synchronized method(spinLock);

        hashRef[msb] = 0;

        // Program Command Register for page 1
        u8 cr = setPage(CR_PAGE1);
        u8 mar = inpb(base + MAR0 + msb/8);
        u8 bit = 1<<(msb % 8);

        if (mar & bit)
        {
            mar &= ~bit;
            outpb(base + MAR0 + msb/8, mar);
            hashTable[msb/8] = mar;
        }

        restorePage(cr);
    }

    return 0;
}

void Dp8390d::
getMacAddress(u8 mac[6])
{
    Synchronized<es::Monitor*> method(monitor);

    memmove(mac, this->mac, sizeof(this->mac));
}

bool Dp8390d::
getLinkState()
{
    Synchronized<es::Monitor*> method(monitor);

    u8 config0;
    {
        Lock::Synchronized method(spinLock);

        // Assume 10BaseT with link test enabled on RTL8029AS
        u8 cr = setPage(CR_PAGE3);
        config0 = inpb(base + CONFIG0);
        restorePage(cr);
    }
    return (config0 & CONFIG0_BNC) ? false : true;
}

void Dp8390d::
getStatistics(Statistics* statistics)
{
    Synchronized<es::Monitor*> method(monitor);

    *statistics = this->statistics;
};

//
// IStream
//

int Dp8390d::
read(void* dst, int count)
{
    Synchronized<es::Monitor*> method(monitor);

    while (enabled && (overflow || sending || isRingEmpty()))
    {
        monitor->wait();
    }
    if (!enabled)
    {
        return -1;
    }

    // Read header
    RingHeader header;
    unsigned short addr = nextPacket * PAGE_SIZE;
    remoteRead(addr, &header, sizeof(header));
    int len = getPacketSize(&header);
    if (len < MIN_SIZE || MAX_SIZE < len)
    {
        updateRing(header.nextPage);
        return 0;
    }
    len -= 4;   // Subtract FCS bytes

    // Read packet
    int result = 0;
    if ((header.status & (RSR_FO | RSR_FAE | RSR_CRC | RSR_PRX)) == RSR_PRX)
    {
        statistics.inOctets += len;
        if (count < len)
        {
            esReport("Dp8390d::read: The specified buffer size is too small.\n");
            len = count;
        }
        result += len;
        addr += sizeof(header);
        u8* buf = static_cast<u8*>(dst);
        int snip = pageStop * PAGE_SIZE - addr;
        if (snip < len)
        {
            remoteRead(addr, buf, snip);
            len -= snip;
            buf += snip;
            addr = pageStart * PAGE_SIZE;
        }
        if (0 < len)
        {
            remoteRead(addr, buf, len);
        }
        updateRing(header.nextPage);
    }
    return result;
}

int Dp8390d::
write(const void* src, int count)
{
    Synchronized<es::Monitor*> method(monitor);

    if (!src || count <= 0 || MAX_SIZE - 4 < count)
    {
        return 0;
    }

    while (enabled && (overflow || sending))
    {
        monitor->wait();
    }
    if (!enabled)
    {
        return -1;
    }

    sending = count;
    sendingUcast = (static_cast<const u8*>(src)[0] & 0x01) ? false : true;

    remoteWrite(txPageStart * PAGE_SIZE, src, count);

    while (enabled && sending)
    {
        monitor->wait();
    }

    return count;
}

//
// ICallback
//
int  Dp8390d::
invoke(int irq)
{
    Lock::Synchronized method(spinLock);

    if (irq == 0)
    {
        // Recover ring overflow
        recover();
        return 0;
    }

    outpb(base + IMR, 0x00); // disable interrupts.

    statistics.inErrors += inpb(base + CNTR0); // Frame Alignment Error
    statistics.inErrors += inpb(base + CNTR1); // CRC Error
    statistics.inErrors += inpb(base + CNTR2); // Frames Lost

    while (u8 isr = (inpb(base + ISR) & (ISR_CNT | ISR_OVW | ISR_TXE | ISR_RXE | ISR_PTX | ISR_PRX)))
    {
        if (isr & (ISR_TXE | ISR_PTX))
        {
            // TRANSMIT ERROR or PACKET TRANSMITTED
            u8 tsr = inpb(base + TSR);
            if (tsr & TSR_PTX)
            {
                statistics.outOctets += sending;
                if (sendingUcast)
                {
                    ++statistics.outUcastPkts;
                }
                else
                {
                    ++statistics.outNUcastPkts;
                }
            }
            else
            {
                ++statistics.outErrors;
                if (tsr & TSR_ABT)
                {
                    ++statistics.outCollisions;
                }
            }

            outpb(base + ISR, ISR_TXE | ISR_PTX); // clear.
            sending = 0;
            monitor->notifyAll();
        }

        if (isr & (ISR_RXE | ISR_PRX))
        {
            // RECEIVE ERR or PACKET RECEIVED
            u8 rsr = inpb(base + RSR);
            if (rsr & RSR_PRX)
            {
                if (rsr & RSR_PHY)
                {
                    ++statistics.inNUcastPkts;
                }
                else
                {
                    ++statistics.inUcastPkts;
                }
            }
            else
            {
                ++statistics.inErrors;
            }
            outpb(base + ISR, ISR_RXE | ISR_PRX); // clear.
            monitor->notifyAll();
        }

        if (isr & ISR_CNT)
        {
            // MSB of one or more of the Network Tally Counters has been set.
            outpb(base + ISR, ISR_CNT);
        }

        if (isr & ISR_OVW)
        {
            // OVERWRITE WARNING: receive buffer ring storage resources have
            // been exhausted. (Local DMA has reached Boundary Pointer).
            ++statistics.inDiscards;
            if (!overflow)
            {
                overflow = true;
                resend = (sending && (inpb(base + CR) & CR_TXP)) ? true : false;
                outpb(base + CR, CR_RD2 | CR_STP | CR_PAGE0);   // Issue the stop command
                alarm->setEnabled(true);
            }
            outpb(base + ISR, ISR_OVW);
        }
    }

    outpb(base + IMR, IMR_CNTE | IMR_OVWE | IMR_TXEE | IMR_RXEE | IMR_PTXE | IMR_PRXE);

    return 0;
}

//
// IInterface
//

Object* Dp8390d::
queryInterface(const char* riid)
{
    Object* object;

    if (strcmp(riid, es::Stream::iid()) == 0)
    {
        object = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, es::NetworkInterface::iid()) == 0)
    {
        object = static_cast<es::NetworkInterface*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        object = static_cast<es::NetworkInterface*>(this);
    }
    else
    {
        return NULL;
    }
    object->addRef();
    return object;
}

unsigned int Dp8390d::
addRef()
{
    return ref.addRef();
}

unsigned int Dp8390d::
release()
{
    unsigned long count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
