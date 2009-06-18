/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#include <string.h>
#include <es.h>
#include <es/endian.h>
#include "io.h"
#include "ataController.h"

// #define VERBOSE

using namespace BigEndian;
using namespace ATAttachment;
using namespace Register;

AtaPacketDevice::
AtaPacketDevice(AtaController* ctlr, u8 device, u8* signature) :
    AtaDevice(ctlr, device, signature)
{
    using namespace Command;
    using namespace PacketDeviceIdentification;
    using namespace Status;

    if ((id[GENERAL_CONFIGURATION] & 0xc000) != 0x8000)
    {
        return;
    }

    removal = id[GENERAL_CONFIGURATION] & 0x80;

    // Determine the packet size to be used
    switch (id[GENERAL_CONFIGURATION] & 0x0003)
    {
    case 0:
        packetSize = 12;
        break;
    case 1:
        packetSize = 16;
        break;
    }

    sectorSize = 2048;
}

AtaPacketDevice::
~AtaPacketDevice()
{
}

u8 AtaPacketDevice::
testUnitReady()
{
    u8 packet[12];

    memset(packet, 0, sizeof(packet));
    packet[0] = PacketCommand::TEST_UNIT_READY;
    ctlr->issue(this, packet, sizeof(packet), 0, 0, 0);
    u8 status = inpb(ctlr->cmdPort + STATUS);
    if (status & Status::CHK)
    {
        return inpb(ctlr->cmdPort + ERROR);
    }
    return 0;
}

int AtaPacketDevice::
requestSense(void* sense, int count)
{
    u8 packet[12];

    memset(packet, 0, sizeof(packet));
    packet[0] = PacketCommand::REQUEST_SENSE;
    packet[4] = count; // Allocation Length
    return ctlr->issue(this, packet, sizeof(packet), sense, count);
}

int AtaPacketDevice::
readCapacity()
{
    u8 buffer[8];
    u8 packet[12];
    memset(buffer, 0, sizeof(buffer));
    memset(packet, 0, sizeof(packet));
    packet[0] = PacketCommand::READ_CD_RECORDED_CAPACITY;
    if (ctlr->issue(this, packet, sizeof(packet), buffer, 8) == sizeof(buffer))
    {
        sectorSize = dword(buffer + 4);
        size = sectorSize * dword(buffer + 0);
        return 0;
    }
    return -1;
}

int AtaPacketDevice::
read(void* dst, int count, long long offset)
{
    Synchronized<es::Monitor*> method(monitor);
    using namespace Features;

    if (size < offset)
    {
        return 0;
    }
    if (size <= offset + count)
    {
        count = size - offset;
    }
    if (count <= 0)
    {
        return 0;
    }

    u8 packet[12];
    packet[0] = PacketCommand::READ_12;
    packet[1] = 0;
    xdword(packet + 2, offset / sectorSize);
    xdword(packet + 6, count / sectorSize);
    packet[10] = 0;
    packet[11] = 0;
    count = ctlr->issue(this, packet, sizeof(packet),
                        dst, count, dma ? DMA : 0);
    return count;
}

int AtaPacketDevice::
write(const void* src, int count, long long offset)
{
    Synchronized<es::Monitor*> method(monitor);
    using namespace Features;

    if (size < offset)
    {
        return 0;
    }
    if (size <= offset + count)
    {
        count = size - offset;
    }
    if (count <= 0)
    {
        return 0;
    }

    u8 packet[12];
    packet[0] = PacketCommand::WRITE_12;
    packet[1] = 0;
    xdword(packet + 2, offset / sectorSize);
    xdword(packet + 6, count / sectorSize);
    packet[10] = 0;
    packet[11] = 0;
    count = ctlr->issue(this, packet, sizeof(packet),
                        const_cast<void*>(src), count, dma ? DMA : 0);
    return count;
}

int AtaPacketDevice::
modeSense(u8 pageCtrl, u8 pageCode, void* modeParamList, int count)
{
    u8 packet[12];

    memset(packet, 0, sizeof(packet));
    packet[0] = PacketCommand::MODE_SENSE_10;
    packet[2] = ((pageCtrl << 6) | pageCode);
    // Allocation Length
    packet[7] = count;
    packet[8] = count >> 8;
    return ctlr->issue(this, packet, sizeof(packet),
                       modeParamList, count);
}

int AtaPacketDevice::
startStopUnit(bool immediate, bool loEj, bool start, u8 powerCondition)
{
    u8 packet[12];
    memset(packet, 0, sizeof(packet));
    packet[0] = PacketCommand::START_STOP_UNIT;
    packet[1] = immediate;
    packet[4] = (0xf0 & ((u8) powerCondition << 4)) | ((u8) loEj << 1) | (u8) start;
    return ctlr->issue(this, packet, sizeof(packet));
}

int AtaPacketDevice::
stopDisc()
{
    return startStopUnit(0, 0, 0, 0);
}

int AtaPacketDevice::
startDisc()
{
    // Start the Disc and read the TOC.
    return startStopUnit(0, 0, 1, 0);
}

int AtaPacketDevice::
preventAllowMediumRemoval(bool prevent, bool persistent)
{
    u8 packet[12];
    memset(packet, 0, sizeof(packet));
    packet[0] = PacketCommand::PREVENT_ALLOW_MEDIUM_REMOVAL;
    packet[4] = (((u8) persistent) << 1) | (u8) prevent;
    return ctlr->issue(this, packet, sizeof(packet));
}

int AtaPacketDevice::
eject()
{
    Synchronized<es::Monitor*> method(monitor);

    return startStopUnit(0, 1, 0, 0);
}

int AtaPacketDevice::
load()
{
    Synchronized<es::Monitor*> method(monitor);

    return startStopUnit(0, 1, 1, 0);
}

int AtaPacketDevice::
lock()
{
    Synchronized<es::Monitor*> method(monitor);

    return preventAllowMediumRemoval(true);
}

int AtaPacketDevice::
unlock()
{
    Synchronized<es::Monitor*> method(monitor);

    return preventAllowMediumRemoval(false);
}

Object* AtaPacketDevice::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, es::Disk::iid()) == 0)
    {
        objectPtr = static_cast<es::Disk*>(this);
    }
    else if (strcmp(riid, es::RemovableMedia::iid()) == 0 && removal)
    {
        objectPtr = static_cast<es::RemovableMedia*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int AtaPacketDevice::
addRef()
{
    return AtaDevice::addRef();
}

unsigned int AtaPacketDevice::
release()
{
    return AtaDevice::release();
}

bool AtaPacketDevice::
detect()
{
    Synchronized<es::Monitor*> method(monitor);

    if (!removal)
    {
        return true;
    }

#ifdef VERBOSE
    esReport("AtaPacketDevice::detect()\n");
#endif
    u8 status = testUnitReady();
#ifdef VERBOSE
    esReport("status: %02x\n", status);
#endif

    u8 sense[18];
    requestSense(sense, sizeof sense);
    u8 senceKey = sense[2] & 0x0f;
    u8 asc = sense[12];
    u8 ascq = sense[13];
#ifdef VERBOSE
    esReport("sence key: %d, asc %02x, ascq %02x\n",
             senceKey, asc, ascq);
#endif

    if (senceKey || asc || ascq)
    {
        size = 0;
        return false;
    }
    else
    {
        readCapacity();
#ifdef VERBOSE
        esReport("sector size: %d, size %lld\n",
                 sectorSize, size);
#endif
        return true;
    }
}
