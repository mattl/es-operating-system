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

#ifndef NINTENDO_ES_KERNEL_I386_DP8390D_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_DP8390D_H_INCLUDED

#include <es/device/INetworkInterface.h>
#include <es/base/IStream.h>
#include <es/base/ICallback.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/interlocked.h>
#include "i386/core.h"

class Dp8390d : public INetworkInterface, public IStream, public ICallback
{
    Lock            spinLock;
    IMonitor*       monitor;
    Ref             ref;

    unsigned        base;    // I/O base address
    int             irq;     // IRQ

    u8              mac[6];

    unsigned int    nicMemSize;   // total NIC memory size
    unsigned int    reservedPage; // reserved memory size
    bool            sendDone;

    Interlocked     overflow;
    DateTime        lastOverflow;
    bool            resend;

    Statistics statistics;

    static const int PAGE_SIZE = 256;
    static const int NUM_TX_PAGE = 6;
    static const int NUM_HASH_REGISTER = 8;

    // Receive
    struct RingHeader
    {
        u8 status;
        u8 nextPage;
        u8 lenLow;
        u8 lenHigh;
    };

    struct Ring
    {
        u8 pageStart;
        u8 pageStop;
        u8 nextPacket;
    };
    Ring ring;

    // Send
    u8 txPageStart;

    // Multicast
    u8 hashTable[NUM_HASH_REGISTER];
    int hashRef[8 * NUM_HASH_REGISTER];   // reference count of an entry in the hash table.
    u8 rcr; // keep RCR register value. (because no register for page 2 works properly).

    // initialization
    int readProm();
    int initializeMacAddress();
    int initializeMulticastAddress();
    int initializeRing();
    int reset();

    // write
    int writeLocked(const void* src, int count);

    // read
    int getPacketSize(RingHeader* header);
    int checkRingStatus(RingHeader* header, int len);
    int updateRing(RingHeader* header);
    bool isRingEmpty();
    void updateReceiveStatistics(RingHeader* header, int len);
    int readLocked(void* dst, int count);
    unsigned int generateCrc(const u8* mca);

    // error
    int recoverFromOverflow();
    void issueStopCommand();

    // misc
    int readNicMemory(unsigned short src, u8* buf, unsigned short len);
    int writeToNicMemory(unsigned short dst, u8* buf, unsigned short len);
    u8 setPage(int page);
    void restorePage(u8 cr);
    u8 getIsr();
    u8 getCurr();

public:
    Dp8390d(unsigned base, int irq);
    ~Dp8390d();

    // INetworkInterface
    int getType()
    {
        return INetworkInterface::Ethernet;
    }
    int start();
    int stop();
    int probe();

    bool getPromiscuousMode();
    void setPromiscuousMode(bool on);
    int addMulticastAddress(const u8 mac[6]);
    int removeMulticastAddress(const u8 mac[6]);

    void getMacAddress(u8 mac[6]);
    bool getLinkState();

    void getStatistics(Statistics* statistics);

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
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef();
    unsigned int release();

private:

    static const int MIN_SIZE = 60;
    static const int MAX_SIZE = 1514;

    static const unsigned int POLY  = 0xedb88320; // CRC-32 poly (little endian)

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
};

#endif // NINTENDO_ES_KERNEL_I386_DP8390D_H_INCLUDED
