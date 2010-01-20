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

#ifndef NINTENDO_ES_KERNEL_I386_DP8390D_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_DP8390D_H_INCLUDED

#include <es/device/INetworkInterface.h>
#include <es/base/IAlarm.h>
#include <es/base/IStream.h>
#include <es/base/ICallback.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/interlocked.h>
#include "i386/core.h"

class Dp8390d : public es::NetworkInterface, public es::Stream, public es::Callback
{
    static const int MIN_SIZE = 64;
    static const int MAX_SIZE = 1518;

    static const int PAGE_SIZE = 256;
    static const int NUM_TX_PAGE = 6;
    static const int NUM_HASH_REGISTER = 8;

    static const unsigned int POLY = 0xedb88320;    // CRC-32 poly (little endian)

    // Register address (Page 0)
    static const int CR     = 0x00;
    static const int CLDA0  = 0x01;
    static const int PSTART = 0x01;
    static const int CLDA1  = 0x02;
    static const int PSTOP  = 0x02;
    static const int BNRY   = 0x03;
    static const int TSR    = 0x04;
    static const int TPSR   = 0x04;
    static const int NCR    = 0x05;
    static const int TBCR0  = 0x05;
    static const int FIFO   = 0x06;
    static const int TBCR1  = 0x06;
    static const int ISR    = 0x07;
    static const int CRDA0  = 0x08;
    static const int RSAR0  = 0x08;
    static const int CRDA1  = 0x09;
    static const int RSAR1  = 0x09;
    static const int RBCR0  = 0x0a;
    static const int RBCR1  = 0x0b;
    static const int RSR    = 0x0c;
    static const int RCR    = 0x0c;
    static const int CNTR0  = 0x0d;
    static const int TCR    = 0x0d;
    static const int CNTR1  = 0x0e;
    static const int DCR    = 0x0e;
    static const int CNTR2  = 0x0f;
    static const int IMR    = 0x0f;

    static const int DATA   = 0x10;
    static const int RESET  = 0x1f;

    // Register address (Page 1)
    static const int PAR0   = 0x01;
    static const int PAR1   = 0x02;
    static const int PAR2   = 0x03;
    static const int PAR3   = 0x04;
    static const int PAR4   = 0x05;
    static const int PAR5   = 0x06;
    static const int CURR   = 0x07;
    static const int MAR0   = 0x08;
    static const int MAR1   = 0x09;
    static const int MAR2   = 0x0a;
    static const int MAR3   = 0x0b;
    static const int MAR4   = 0x0c;
    static const int MAR5   = 0x0d;
    static const int MAR6   = 0x0e;
    static const int MAR7   = 0x0f;

    // Register address (Page 3 for RTL8029AS)
    static const int CONFIG0 = 0x03;
    static const int CONFIG2 = 0x05;
    static const int CONFIG3 = 0x06;

    // Command Register(CR)
    static const int CR_STP = 1<<0;
    static const int CR_STA = 1<<1;
    static const int CR_TXP = 1<<2;
    static const int CR_RD0 = 1<<3;
    static const int CR_RD1 = 1<<4;
    static const int CR_RD2 = 1<<5;
    static const int CR_PS0 = 1<<6;
    static const int CR_PS1 = 1<<7;

    // alias
    static const int CR_PAGE0 = 0x00;
    static const int CR_PAGE1 = CR_PS0;
    static const int CR_PAGE2 = CR_PS1;
    static const int CR_PAGE3 = CR_PS0 | CR_PS1;    // RTL8029AS Configuration

    // Interrupt status register(ISR)
    static const int ISR_PRX = 1<<0;
    static const int ISR_PTX = 1<<1;
    static const int ISR_RXE = 1<<2;
    static const int ISR_TXE = 1<<3;
    static const int ISR_OVW = 1<<4;
    static const int ISR_CNT = 1<<5;
    static const int ISR_RDC = 1<<6;
    static const int ISR_RST = 1<<7;

    // Interrupt mask register(IMR)
    static const int IMR_PRXE = 1<<0;
    static const int IMR_PTXE = 1<<1;
    static const int IMR_RXEE = 1<<2;
    static const int IMR_TXEE = 1<<3;
    static const int IMR_OVWE = 1<<4;
    static const int IMR_CNTE = 1<<5;
    static const int IMR_RDCE = 1<<6;

    // Data configuration register(DCR)
    static const int DCR_WTS = 1<<0;
    static const int DCR_BOS = 1<<1;
    static const int DCR_LAS = 1<<2;
    static const int DCR_LS  = 1<<3;
    static const int DCR_AR  = 1<<4;
    static const int DCR_FT0 = 1<<5;
    static const int DCR_FT1 = 1<<6;

    // Transmit configuration register(TCR)
    static const int TCR_CRC  = 1<<0;
    static const int TCR_LB0  = 1<<1;
    static const int TCR_LB1  = 1<<2;
    static const int TCR_ATD  = 1<<3;
    static const int TCR_OFST = 1<<4;

    // Transmit status register(TSR)
    static const int TSR_PTX = 1<<0;
    static const int TSR_COL = 1<<2;
    static const int TSR_ABT = 1<<3;
    static const int TSR_CRS = 1<<4;
    static const int TSR_FU  = 1<<5;
    static const int TSR_CDH = 1<<6;
    static const int TSR_OWC = 1<<7;

    // Receive configuration register(RCR)
    static const int RCR_SEP = 1<<0;
    static const int RCR_AR  = 1<<1;
    static const int RCR_AB  = 1<<2;
    static const int RCR_AM  = 1<<3;
    static const int RCR_PRO = 1<<4;
    static const int RCR_MON = 1<<5;

    // Receive status register(RSR)
    static const int RSR_PRX = 1<<0;
    static const int RSR_CRC = 1<<1;
    static const int RSR_FAE = 1<<2;
    static const int RSR_FO  = 1<<3;
    static const int RSR_MPA = 1<<4;
    static const int RSR_PHY = 1<<5;
    static const int RSR_DIS = 1<<6;
    static const int RSR_DFR = 1<<7;

    static const int CONFIG0_BNC = 1<<2;

    struct RingHeader
    {
        u8 status;
        u8 nextPage;
        u8 lenLow;
        u8 lenHigh;
    };

    struct Statistics
    {
        unsigned long long  inOctets;        // The total number of octets received.
        unsigned int        inUcastPkts;     // The number of unicast packets delivered.
        unsigned int        inNUcastPkts;    // The number of non-unicast delivered.
        unsigned int        inDiscards;      // The number of inbound packets discarded.
        unsigned int        inErrors;        // The number of inbound packets that contained errors.
        unsigned int        inUnknownProtos; // The number of inbound packets discarded because of an unknown or unsupported protocol.
        unsigned long long  outOctets;       // The total number of octets transmitted.
        unsigned int        outUcastPkts;    // The total number of packets transmitted to a unicast address.
        unsigned int        outNUcastPkts;   // The total number of packets transmitted to a non-unicast address.
        unsigned int        outDiscards;     // The number of outbound packets discarded.
        unsigned int        outErrors;       // The number of outbound packets that could not be transmitted because of errors.

        unsigned int        outCollisions;   // Collisions on CSMA
    };

    Lock         spinLock;       // for invoke()
    es::Monitor* monitor;
    Ref          ref;

    u8           bus;
    unsigned     base;           // I/O base address
    int          irq;

    u8           mac[6];

    u16          ramStart;       // The start address of the buffer memory
    u16          ramEnd;         // The end address of the buffer memory

    bool         enabled;

    int          sending;        // The number of octets being sent. Zero if not sending.
    bool         sendingUcast;   // True if sending a ucast packet.

    es::Alarm*   alarm;
    bool         overflow;
    bool         resend;

    Statistics   statistics;

    // Ring
    u8           pageStart;
    u8           pageStop;
    u8           nextPacket;

    // Send
    u8           txPageStart;

    // Multicast
    u8           hashTable[NUM_HASH_REGISTER];
    int          hashRef[8 * NUM_HASH_REGISTER]; // Reference count of each entry in the hash table.
    u8           rcr;                            // Saved RCR register value. (because no register for page 2 works properly).

    // Initialization
    bool reset();
    bool readMacAddress();
    bool initialize();

    // Read
    int getPacketSize(RingHeader* header);
    int updateRing(u8 nextPage);
    bool isRingEmpty();
    unsigned int generateCrc(const u8 macaddr[6]);

    // Overflow
    void recover();

    // Misc.
    int remoteRead(unsigned short src, void* buf, unsigned short len);
    int remoteWrite(unsigned short dst, const void* buf, unsigned short len);
    u8 setPage(int page);
    void restorePage(u8 cr);

public:
    Dp8390d(u8 bus, unsigned base, int irq);
    ~Dp8390d();

    // INetworkInterface
    int getType()
    {
        return es::NetworkInterface::Ethernet;
    }
    int start();
    int stop();

    bool getPromiscuousMode();
    void setPromiscuousMode(bool on);
    int addMulticastAddress(const u8* mac);
    int removeMulticastAddress(const u8* mac);

    int getMacAddress(u8* mac);
    bool getLinkState();

    unsigned long long getInOctets()
    {
        return statistics.inOctets;
    }
    unsigned int getInUcastPkts()
    {
        return statistics.inUcastPkts;
    }
    unsigned int getInNUcastPkts()
    {
        return statistics.inNUcastPkts;
    }
    unsigned int getInDiscards()
    {
        return statistics.inDiscards;
    }
    unsigned int getInErrors()
    {
        return statistics.inErrors;
    }
    unsigned int getInUnknownProtos()
    {
        return statistics.inUnknownProtos;
    }
    unsigned long long getOutOctets()
    {
        return statistics.outOctets;
    }
    unsigned int getOutUcastPkts()
    {
        return statistics.outUcastPkts;
    }
    unsigned int getOutNUcastPkts()
    {
        return statistics.outNUcastPkts;
    }
    unsigned int getOutDiscards()
    {
        return statistics.outDiscards;
    }
    unsigned int getOutErrors()
    {
        return statistics.outErrors;
    }
    unsigned int getOutCollisions()
    {
        return statistics.outCollisions;
    }

    int getMTU()
    {
        return 1500;
    }

    // IStream
    long long getPosition()
    {
        return 0;
    }

    void setPosition(long long pos)
    {
    }

    long long getSize()
    {
        return 0;
    }

    void setSize(long long size)
    {
    }

    int read(void* dst, int count);

    int read(void* dst, int count, long long offset)
    {
        return -1;
    }

    int write(const void* src, int count);

    int write(const void* src, int count, long long offset)
    {
        return -1;
    }

    void flush()
    {
    }

    // ICallback
    int invoke(int irq);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // NINTENDO_ES_KERNEL_I386_DP8390D_H_INCLUDED
