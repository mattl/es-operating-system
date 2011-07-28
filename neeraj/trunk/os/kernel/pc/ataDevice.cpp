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

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "io.h"
#include "ataController.h"

// #define VERBOSE

using namespace ATAttachment;
using namespace Register;

AtaDevice::
AtaDevice(AtaController* ctlr, u8 device, u8* signature) :
    ctlr(ctlr),
    device(device),
    size(0),
    sectorSize(512),
    multiple(0),
    packetSize(0),
    dma(0),
    removal(false)
{
    using namespace Command;
    using namespace DeviceIdentification;
    using namespace Status;

    monitor = es::Monitor::createInstance();

    if (!identify(signature))
    {
        return;
    }

    // Select DMA mode
    if (ctlr->dma && (id[CAPABILITIES] & 0x100))
    {
        dma = id[MULTIWORD_DMA_TRANSFER] & 0x0707;
        dma = (dma >> 8) & dma;
        if (dma == 0 && (id[FIELD_VALIDITY] & 0x04))
        {
            dma = id[ULTRA_DMA_MODES] & 0x7f7f;
            dma = (dma >> 8) & dma;
            dma |= 0x80;
        }
    }

#ifdef VERBOSE
    esReport("CHS=%d/%d/%d\n", id[1], id[3], id[6]);
    esDump(id, 512);
#endif

    if (AtaController::isAtapiDevice(signature))
    {
        return;
    }

    long long sectors;
    if (id[FEATURES_COMMAND_SETS_SUPPORTED + 1] & 0x0400)
    {
        sectors = id[MAXIMUM_USER_LBA_FOR_48_BIT_ADDRESS_FEATURE_SET] |
                  (id[MAXIMUM_USER_LBA_FOR_48_BIT_ADDRESS_FEATURE_SET + 1] << 16) |
                  ((long long) id[MAXIMUM_USER_LBA_FOR_48_BIT_ADDRESS_FEATURE_SET + 2] << 32);
    }
    else
    {
        sectors = id[TOTAL_NUMBER_OF_USER_ADDRESSABLE_SECTORS] |
                  (id[TOTAL_NUMBER_OF_USER_ADDRESSABLE_SECTORS + 1] << 16);
    }
    size = sectors * sectorSize;

    // Select read/write multiple
    u16 maxrwm = (id[READ_WRITE_MULTIPLE_SUPPORT] & 0xff);
    if (0 < maxrwm)
    {
        if (id[MULTIPLE_SECTOR_SETTING] & 0x100)
        {
            multiple = (id[MULTIPLE_SECTOR_SETTING] & 0xff);
        }
        if (multiple == 0)
        {
            multiple = maxrwm;
        }
        if (16 < multiple)
        {
            multiple = 16;
        }
        ctlr->select(device);
        outpb(ctlr->cmdPort + SECTOR_COUNT, multiple);
        outpb(ctlr->cmdPort + COMMAND, Command::SET_MULTIPLE_MODE);
        ctlr->sync(DF | ERR);
        u8 state = inpb(ctlr->cmdPort + STATUS);
        if (state & (DF | ERR))
        {
            multiple = 0;
        }
    }

    if (id[FEATURES_COMMAND_SETS_SUPPORTED + 1] & 0x0400)
    {
        if (dma)
        {
            readCmd = READ_DMA_EXT;
            writeCmd = WRITE_DMA_EXT;
        }
        else if (multiple)
        {
            readCmd = READ_MULTIPLE_EXT;
            writeCmd = WRITE_MULTIPLE_EXT;
        }
        else
        {
            readCmd = READ_SECTOR_EXT;
            writeCmd = WRITE_SECTOR_EXT;
        }
    }
    else
    {
        if (dma)
        {
            readCmd = READ_DMA;
            writeCmd = WRITE_DMA;
        }
        else if (multiple)
        {
            readCmd = READ_MULTIPLE;
            writeCmd = WRITE_MULTIPLE;
        }
        else
        {
            readCmd = READ_SECTOR;
            writeCmd = WRITE_SECTOR;
        }
    }
}

AtaDevice::
~AtaDevice()
{
    monitor->release();
}

bool AtaDevice::
identify(u8* signature)
{
    using namespace Status;

    ctlr->select(device);
    if (AtaController::isAtapiDevice(signature))
    {
        outpb(ctlr->cmdPort + COMMAND, Command::IDENTIFY_PACKET_DEVICE);
    }
    else
    {
        outpb(ctlr->cmdPort + COMMAND, Command::IDENTIFY_DEVICE);
    }
    esSleep(4);
    ctlr->sync(DRQ | ERR);
    u8 state = inpb(ctlr->cmdPort + STATUS);
    if (state & ERR)
    {
        return false;
    }
    memset(id, 0, sectorSize);
    inpsw(ctlr->cmdPort + DATA, id, 256);
    inpb(ctlr->cmdPort + STATUS);
    return true;
}

long long AtaDevice::
getPosition()
{
    return 0;
}

void AtaDevice::
setPosition(long long pos)
{
}

long long AtaDevice::
getSize()
{
    return this->size;
}

void AtaDevice::
setSize(long long size)
{
}

int AtaDevice::
read(void* dst, int count)
{
    return 0;
}

int AtaDevice::
read(void* dst, int count, long long offset)
{
#ifdef VERBOSE
    esReport("AtaDevice::%s(%p, %d, %lld)\n", __func__, dst, count, offset);
#endif
    count = ctlr->issue(this, readCmd, dst, count / sectorSize, offset / sectorSize);
#ifdef VERBOSE
    esReport("AtaDevice::%s : %d\n", __func__, count);
#endif
    return (count <= 0) ? count : (count * sectorSize);
}

int AtaDevice::
write(const void* src, int count)
{
    return 0;
}

int AtaDevice::
write(const void* src, int count, long long offset)
{
#ifdef VERBOSE
    esReport("AtaDevice::%s(%p, %d, %lld)\n", __func__, src, count, offset);
#endif
    count = ctlr->issue(this, writeCmd, const_cast<void*>(src), count / sectorSize, offset / sectorSize);
#ifdef VERBOSE
    esReport("AtaDevice::%s : %d\n", __func__, count);
#endif
    return (count <= 0) ? count : (count * sectorSize);
}

void AtaDevice::
flush()
{
}

unsigned int AtaDevice::
getHeads()
{
    return id[3];   // 0..255
}

unsigned int AtaDevice::
getCylinders()
{
    return id[1];   // 0..1023
}

unsigned int AtaDevice::
getSectorsPerTrack()
{
    return id[6];   // 1..63
}

unsigned int AtaDevice::
getBytesPerSector()
{
    return sectorSize;
}

long long AtaDevice::
getDiskSize()
{
    return size;
}

Object* AtaDevice::
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

unsigned int AtaDevice::
addRef()
{
    return ref.addRef();
}

unsigned int AtaDevice::
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

bool AtaDevice::
detect()
{
    return true;
}
