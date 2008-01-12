/*
 * Copyright (c) 2006
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

#include <string.h>
#include <es.h>
#include <es/clsid.h>
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
    Synchronized<IMonitor*> method(monitor);
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
    Synchronized<IMonitor*> method(monitor);
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
    Synchronized<IMonitor*> method(monitor);

    return startStopUnit(0, 1, 0, 0);
}

int AtaPacketDevice::
load()
{
    Synchronized<IMonitor*> method(monitor);

    return startStopUnit(0, 1, 1, 0);
}

int AtaPacketDevice::
lock()
{
    Synchronized<IMonitor*> method(monitor);

    return preventAllowMediumRemoval(true);
}

int AtaPacketDevice::
unlock()
{
    Synchronized<IMonitor*> method(monitor);

    return preventAllowMediumRemoval(false);
}

bool AtaPacketDevice::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IStream)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IID_IDiskManagement)
    {
        *objectPtr = static_cast<IDiskManagement*>(this);
    }
    else if (riid == IID_IRemovableMedia && removal)
    {
        *objectPtr = static_cast<IRemovableMedia*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
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
    Synchronized<IMonitor*> method(monitor);

    if (!removal)
    {
        return true;
    }

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

#if 0

xxx()
{
    u8 list[120];
    int cdrom = modeSense(0x00, PAGE_CDROM, list, sizeof(list));
    if (cdrom < 0)
    {
        esReport("Device %d supports neither CD-ROM nor DVD-ROM.\n", (Dev1 == this->device));
        return false;
    }

    u16 capabilities;
    int ret = getCDVDCapabilities(&capabilities);
    if (ret < 0)
    {
        esReport("C/DVD capabiliities unavaialble\n");
        return false;
    }

    if (testUnitReady())
    {
        u32 blockLength;
        u32 lba;
        int ret = readCapacity(&lba, &blockLength);
        if (ret < 0)
        {
            esReport("read capacity command: failed\n");
            // set temporary parameters
            blockLength = 2048;
            lba = 640 * 1024 * 1024 / blockLength;
        }
        sectorSize = blockLength;
        sectors = lba;
        mediaType = getMediaType();
    }
    else
    {
        // no media
        mediaType = PF_NO_MEDIA;
        // set temporary parameters
        sectorSize = 2048;
        sectors = 640 * 1024 * 1024 / 2048;
    }

    dumpCapabilities(cdrom, capabilities); // debug
}

void AtaPacketDevice::
dumpCapabilities(int cdrom, u16 capa)
{
    esReport("    capabilities: ");

    if (cdrom == 0)
    {
        esReport("CD-ROM");
    }

    if (capa & CD_RW_WRITE || capa & CD_RW_READ)
    {
        esReport("  CD-RW(%s%s)", capa & CD_RW_WRITE ? "W" : "",
                                capa & CD_RW_READ  ? "R" : "");
    }

    if (capa & CD_R_WRITE || capa & CD_R_READ)
    {
        esReport("  CD-R(%s%s)", capa & CD_R_WRITE ? "W" : "",
                               capa & CD_R_READ  ? "R" : "");
    }

    if (capa & DVD_ROM_READ)
    {
        esReport("  DVD-ROM");
    }

    if (capa & DVD_RAM_WRITE || capa & DVD_RAM_READ)
    {
        esReport("  DVD-RAM(%s%s)", capa & DVD_RAM_WRITE ? "W" : "",
                                  capa & DVD_RAM_READ  ? "R" : "");
    }

    if (capa & DVD_R_WRITE || capa & DVD_R_READ)
    {
        esReport("  DVD-R(%s%s)", capa & DVD_R_WRITE ? "W" : "",
                                capa & DVD_R_READ  ? "R" : "");
    }
    esReport("\n");

    esReport("    media: %s (%x)\n",
        mediaType == PF_NON_REMOVABLE ? "Non-removable disk" :
        mediaType == PF_REMOVABLE ? "Removable disk" :
        mediaType == PF_MO ? "MO Erasable" :
        mediaType == PF_AS_MO ? "AS-MO" :
        mediaType == PF_CD_ROM ? "CD-ROM" :
        mediaType == PF_CD_R ? "CD-R" :
        mediaType == PF_CD_RW ? "CD-RW" :
        mediaType == PF_DVD_ROM ? "DVD-ROM" :
        mediaType == PF_DVD_R ? "DVD-R" :
        mediaType == PF_DVD_RAM ? "DVD-RAM" :
        mediaType == PF_DVD_RW_RO ? "DVD-RW Restricted Overwrite" :
        mediaType == PF_DVD_RW_SR ? "DVD-RW Sequential recording" :
        mediaType == PF_DVD_R_DUAL_SR ? "DVD-R Dual Layer Sequential recording" :
        mediaType == PF_DVD_R_DUAL_LJ ? "DVD-R Dual Layer Jump recording" :
        mediaType == PF_DVD_PLUS_RW ? "DVD+RW" :
        mediaType == PF_DVD_PLUS_R  ? "DVD+R" :
        mediaType == PF_UNKNOWN ? "unknown media" : "No media",
        mediaType);
}

int AtaPacketDevice::
getConfig(void* response, int len)
{
    status = 0;
    error = 0;

    data = static_cast<u8*>(response);
    dlen = len;

    u8 packet[12];
    memset(packet, 0, sizeof(packet));
    packet[0] = PCgetConfig;
    // Allocation Length
    packet[7] = 0xff & len;
    packet[8] = 0xff & (len >> 8);

    int ret = ctlr->issue(this, packet, sizeof(packet));

    return ret;
}

int AtaPacketDevice::
getCDVDCapabilities(u16* capabilities)
{
    u8 list[120];
    int ret = modeSense(0x00, PAGE_CDROM_CAPABILITIES, list, sizeof(list));

    if (ret == 0)
    {
        u8* ptr = &list[8];
        if (*ptr == PAGE_CDROM_CAPABILITIES)
        {
            *capabilities = (u16) (ptr[3] << 8 | ptr[2]);
        }
        else
        {
            // unknown data format
            ret = -1;
        }
    }

    return ret;
}

int AtaPacketDevice::
getMediaType()
{
    if (!testUnitReady())
    {
        return -1;
    }

    u8 response[96];
    int ret = getConfig(response, sizeof(response));

    if (ret == 0)
    {
        mediaType = (response[6] << 8| response[7]);
        return mediaType; // defined in Profile List.
    }

    return ret;
}

#endif
