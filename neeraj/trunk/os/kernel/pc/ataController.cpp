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

#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "core.h"
#include "io.h"
#include "ataController.h"

// #define VERBOSE

using namespace ATAttachment;
using namespace Register;

bool AtaController::
isAtaDevice(const u8* signature)
{
    return (signature[2] == 0x00 && signature[3] == 0x00) ? true : false;
}

bool AtaController::
isAtapiDevice(const u8* signature)
{
    return (signature[2] == 0x14 && signature[3] == 0xeb) ? true : false;
}

bool AtaController::
softwareReset()
{
    using namespace DeviceControl;

    inpb(cmdPort + STATUS); // Modified not to call sync(0) for Virtual PC.
    outpb(ctlPort + DEVICE_CONTROL, SRST | NIEN);
    esSleep(50);
    outpb(ctlPort + DEVICE_CONTROL, NIEN);
    esSleep(20000);
    sync(0);
    return true;
}

bool AtaController::
detectDevice(int dev, u8* signature)
{
    // Get signature
    outpb(cmdPort + DEVICE, dev);
    esSleep(4);
    signature[0] = inpb(cmdPort + SECTOR_COUNT);
    signature[1] = inpb(cmdPort + LBA_LOW);
    signature[2] = inpb(cmdPort + LBA_MID);
    signature[3] = inpb(cmdPort + LBA_HIGH);
    signature[4] = inpb(cmdPort + DEVICE);

    u8 status = inpb(cmdPort + STATUS);
    u8 err = inpb(cmdPort + ERROR);

    if (status == 0xff)
    {
        return false;
    }
    if (!(status & Status::DRDY) && !isAtapiDevice(signature))
    {
        return false;
    }
    return true;
}

void AtaController::
select(u8 device)
{
    using namespace Status;

    long timeout = 0;
    while (inpb(ctlPort + ALTERNATE_STATUS) & (DRQ | BSY))
    {
#if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__ ("pause\n");
#endif
        if (1000000L < ++timeout)
        {
            esPanic(__FILE__, __LINE__, "AtaController::%s -- timeout (%02x)",
                    __func__, inpb(ctlPort + ALTERNATE_STATUS));
            break;
        }
    }
    outpb(cmdPort + DEVICE, device);
    esSleep(4);
    timeout = 0;
    while (inpb(ctlPort + ALTERNATE_STATUS) & (DRQ | BSY))
    {
#if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__ ("pause\n");
#endif
        if (1000000L < ++timeout)
        {
            esPanic(__FILE__, __LINE__, "AtaController::%s -- timeout (%02x)",
                    __func__, inpb(ctlPort + ALTERNATE_STATUS));
            break;
        }
    }
}

u8 AtaController::
sync(u8 status)
{
    using namespace Status;

    long timeout = 0;
    while (inpb(ctlPort + ALTERNATE_STATUS) & BSY)
    {
#if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__ ("pause\n");
        // Reading or writing a byte from/to port 0x80 take almost
        // exactly 1 microsecond independent of the processor type
        // and speed.
        __asm__ __volatile__ ("outb %%al, $0x80" ::: "%eax");
#endif
        if (1000000L < ++timeout)
        {
            esPanic(__FILE__, __LINE__, "AtaController::%s -- timeout (%02x)\n",
                    __func__, inpb(ctlPort + ALTERNATE_STATUS));
            break;
        }
    }
    inpb(ctlPort + ALTERNATE_STATUS);
    return inpb(cmdPort + STATUS);
}

int AtaController::
condDone(int)
{
    return done ? true : false;
}

void AtaController::
wait()
{
    DelegateTemplate<AtaController> cond(this, &AtaController::condDone);
    rendezvous.sleep(&cond);
}

void AtaController::
notify()
{
    rendezvous.wakeup();
}

int AtaController::
issue(AtaDevice* device, u8 cmd, void* buffer, int count, long long lba)
{
    Synchronized<es::Monitor*> method(monitor);

    using namespace Device;
    using namespace DeviceControl;
    using namespace DeviceIdentification;
    using namespace Command;
    using namespace Status;

#ifdef VERBOSE
    esReport("AtaController::%s(%02x, %p, %d, %lld)\n", __func__, cmd, buffer, count, lba);
#endif

    Lock::Synchronized io(lock);

    if (device->id[FEATURES_COMMAND_SETS_SUPPORTED + 1] & 0x0400)
    {
        if (lba >> 48)
        {
            return -1;
        }
        if (65536 < count)
        {
            count = 65536;
        }
    }
    else
    {
        if (lba >> 28)
        {
            return -1;
        }
        if (256 < count)
        {
            count = 256;
        }
    }

    outpb(ctlPort + DEVICE_CONTROL, 0);
    select(device->device);
    switch (cmd)
    {
    case READ_DMA:
    case READ_DMA_EXT:
    case WRITE_DMA:
    case WRITE_DMA_EXT:
        dma->setup(cmd, buffer, count * device->sectorSize);
        break;
    }
    if (device->id[FEATURES_COMMAND_SETS_SUPPORTED + 1] & 0x0400)
    {
        outpb(cmdPort + SECTOR_COUNT, count >> 8);
        outpb(cmdPort + SECTOR_COUNT, count);
        outpb(cmdPort + LBA_LOW, lba >> 24);
        outpb(cmdPort + LBA_LOW, lba);
        outpb(cmdPort + LBA_MID, lba >> 32);
        outpb(cmdPort + LBA_MID, lba >> 8);
        outpb(cmdPort + LBA_HIGH, lba >> 40);
        outpb(cmdPort + LBA_HIGH, lba >> 16);
        outpb(cmdPort + DEVICE, LBA | device->device);
    }
    else
    {
        outpb(cmdPort + SECTOR_COUNT, count);
        outpb(cmdPort + LBA_LOW, lba);
        outpb(cmdPort + LBA_MID, lba >> 8);
        outpb(cmdPort + LBA_HIGH, lba >> 16);
        outpb(cmdPort + DEVICE, LBA | device->device | (lba >> 24));
    }

#ifdef VERBOSE
    esReport("AtaController::%s: %02x %x %d %lld\n", __func__, cmd, cmdPort, count, lba);
#endif

    this->cmd = cmd;
    done = false;
    current = device;
    data = static_cast<u8*>(buffer);
    limit = data + device->sectorSize * count;
    outpb(cmdPort + COMMAND, cmd);
    esSleep(4);

    switch (cmd)
    {
    case WRITE_SECTOR:
    case WRITE_SECTOR_EXT:
        if (sync(DRQ | ERR) & ERR)
        {
            count  = -1;
            done = true;
            break;
        }
        outpsw(cmdPort + DATA, buffer, device->sectorSize / 2);
        break;
    case WRITE_MULTIPLE:
    case WRITE_MULTIPLE_EXT:
        if (sync(DRQ | ERR) & ERR)
        {
            count  = -1;
            done = true;
            break;
        }
        if (count < device->multiple)
        {
            outpsw(cmdPort + DATA, buffer, count * device->sectorSize / 2);
        }
        else
        {
            outpsw(cmdPort + DATA, buffer, device->multiple * device->sectorSize / 2);
        }
        break;
    case READ_DMA:
    case READ_DMA_EXT:
    case WRITE_DMA:
    case WRITE_DMA_EXT:
        dma->start(cmd);
        break;
    }

    lock.unlock();
    wait();
    lock.lock();

    outpb(ctlPort + DEVICE_CONTROL, NIEN);
    return count;
}

int AtaController::
issue(AtaDevice* device, u8* packet, int packetSize,
      void* buffer, int count, u8 features)
{
    Synchronized<es::Monitor*> method(monitor);

    using namespace Device;
    using namespace DeviceControl;
    using namespace Command;
    using namespace Status;

    select(device->device);

    outpb(cmdPort + FEATURES, features);
    outpb(cmdPort + SECTOR_COUNT, 0);
    outpb(cmdPort + LBA_LOW, 0);
    int len = (device->sectorSize < count) ? device->sectorSize : count;
    outpb(cmdPort + BYTE_COUNT_LOW, len);
    outpb(cmdPort + BYTE_COUNT_HIGH, len >> 8);
    outpb(cmdPort + DEVICE_SELECT, device->device);

    // Save packet
    memmove(this->packet, packet, packetSize);
    memset(this->packet + packetSize, 0, device->packetSize - packetSize);

#ifdef VERBOSE
    esReport("AtaController::%s: %02x(%02x) %x (%d)\n", __func__, PACKET, *packet, cmdPort, __LINE__);
#endif

    this->cmd = PACKET;
    this->features = features;
    done = false;
    current = device;
    data = static_cast<u8*>(buffer);
    limit = data + count;
    outpb(cmdPort + COMMAND, PACKET);
    esSleep(4);
    sync(CHK | DRQ);
    if (invoke(0) < 0)
    {
        return -1;
    }

    wait();

    outpb(ctlPort + DEVICE_CONTROL, NIEN);
    return count;
}

int AtaController::
invoke(int param)
{
    Lock::Synchronized io(lock);

    using namespace Command;
    using namespace Error;
    using namespace Features;
    using namespace Status;
    using namespace InterruptReason;

    u8 error;
    u8 status;

    if (inpb(ctlPort + ALTERNATE_STATUS) & BSY)
    {
        return -1;
    }

    status = inpb(cmdPort + STATUS);
    if (!current)
    {
        return -1;
    }

#ifdef VERBOSE
    if (param)
    {
        esReport("AtaController::%s(%d) : %02xh : %02xh\n", __func__, param, cmd, status);
    }
#endif

    if (status & ERR)
    {
        error = inpb(cmdPort + ERROR);
    }
    else
    {
        error = 0;
        switch (cmd)
        {
        case READ_SECTOR:
        case READ_SECTOR_EXT:
            if (status & DRQ)
            {
                inpsw(cmdPort + DATA, data, current->sectorSize / 2);
                data += current->sectorSize;
                if (limit <= data)
                {
                    done = true;
                }
#ifdef VERBOSE
                esReport("AtaController::%s : %p, %p, %d\n", __func__, data, limit, done);
#endif
            }
            else
            {
                error = ABRT;
            }
            break;
        case READ_MULTIPLE:
        case READ_MULTIPLE_EXT:
            if (status & DRQ)
            {
                int len = current->multiple * current->sectorSize;
                if (limit < data + len)
                {
                    len = limit - data;
                }
                inpsw(cmdPort + DATA, data, len / 2);
                data += len;
                if (limit <= data)
                {
                    done = true;
                }
#ifdef VERBOSE
                esReport("AtaController::%s : %d, %p, %p, %d\n", __func__, len, data, limit, done);
#endif
            }
            else
            {
                error = ABRT;
            }
            break;
        case WRITE_SECTOR:
        case WRITE_SECTOR_EXT:
            data += current->sectorSize;
            if (limit <= data)
            {
                done = true;
            }
            else if (status & DRQ)
            {
                outpsw(cmdPort + DATA, data, current->sectorSize / 2);
            }
            else
            {
                error = ABRT;
            }
            break;
        case WRITE_MULTIPLE:
        case WRITE_MULTIPLE_EXT:
            data += current->multiple * current->sectorSize;
            if (limit <= data)
            {
                data = limit;
                done = true;
            }
            else if (status & DRQ)
            {
                int len = current->multiple * current->sectorSize;
                if (limit <= data + len)
                {
                    len = limit - data;
                }
                outpsw(cmdPort + DATA, data, len / 2);
            }
            else
            {
                error = ABRT;
            }
            break;
        case PACKET:
            int len;
            outpb(ctlPort + DEVICE_CONTROL, 0);
            switch (inpb(cmdPort + INTERRUPT_REASON) & (/*Rel|*/ IO | CD))
            {
              case CD:      // transfer the packet
                if (features & DMA)
                {
                    dma->setup(cmd, data, limit - data);
                    outpsw(cmdPort + DATA, packet, current->packetSize / 2);
                    dma->start(cmd);
                }
                else
                {
                    outpsw(cmdPort + DATA, packet, current->packetSize / 2);
                }
                break;
              case 0:       // write
                len = (inpb(cmdPort + BYTE_COUNT_HIGH) << 8) | inpb(cmdPort + BYTE_COUNT_LOW);
                outpsw(cmdPort + DATA, data, len / 2);
                data += len;
                break;
              case IO:      // read
                len = (inpb(cmdPort + BYTE_COUNT_HIGH) << 8) | inpb(cmdPort + BYTE_COUNT_LOW);
                inpsw(cmdPort + DATA, data, len / 2);
                data += len;
                break;
              case IO | CD: // completed
                if (features & DMA)
                {
                    dma->interrupt(limit - data);
                }
                else
                {
                    done = true;
                }
                break;
            }
             break;
        case READ_DMA:
        case READ_DMA_EXT:
        case WRITE_DMA:
        case WRITE_DMA_EXT:
            dma->interrupt(limit - data);
            break;
        case STANDBY:
            done = true;
            break;
        default:
            error = ABRT;
            break;
        }
    }

    if (error)
    {
        status |= ERR;
        done = true;
    }

    if (done)
    {
        current = 0;
        notify();
    }

    return 0;
}

AtaController::
AtaController(int cmdPort, int ctlPort, int irq, AtaDma* dma, es::Context* ata) :
    cmdPort(cmdPort),
    ctlPort(ctlPort),
    irq(irq),
    dma(dma),
    thread(run, this, es::Thread::Highest - (irq - 14))
{
    device[0] = device[1] = 0;

    monitor = es::Monitor::createInstance();

    if (!softwareReset())
    {
        return;
    }

    u8 signature[5];
    if (detectDevice(Device::MASTER, signature))
    {
#ifdef VERBOSE
        esReport("device0: signature %02x %02x %02x %02x %02x\n",
                 signature[0], signature[1], signature[2], signature[3], signature[4]);
#endif
        if (isAtaDevice(signature))
        {
            device[0] = new AtaDevice(this, Device::MASTER, signature);
        }
        else if (isAtapiDevice(signature))
        {
            device[0] = new AtaPacketDevice(this, Device::MASTER, signature);
        }
    }

    if (detectDevice(Device::SLAVE, signature))
    {
#ifdef VERBOSE
        esReport("device1: signature %02x %02x %02x %02x %02x\n",
                 signature[0], signature[1], signature[2], signature[3], signature[4]);
#endif
        if (isAtaDevice(signature))
        {
            device[1] = new AtaDevice(this, Device::SLAVE, signature);
        }
        else if (isAtapiDevice(signature))
        {
            device[1] = new AtaPacketDevice(this, Device::SLAVE, signature);
        }
    }

    if (device[0] == 0 && device[1] == 0)
    {
        return;
    }

    Core::registerInterruptHandler(irq, this);

    for (int i = 0; i < 10; ++i)
    {
        char name[10];

        sprintf(name, "channel%d", i);
        Handle<es::Context> channel = ata->lookup(name);
        if (!channel)
        {
            channel = ata->createSubcontext(name);
            if (channel)
            {
                if (device[0])
                {
                    Handle<es::Binding> binding = channel->bind("device0",
                                                             static_cast<es::Stream*>(device[0]));
                }
                if (device[1])
                {
                    Handle<es::Binding> binding = channel->bind("device1",
                                                             static_cast<es::Stream*>(device[1]));
                }
            }
            break;
        }
    }

    addRef();   // for thread
    thread.start();
}

AtaController::
~AtaController()
{
    monitor->release();
}

Object* AtaController::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int AtaController::
addRef()
{
    return ref.addRef();
}

unsigned int AtaController::
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

void AtaController::
detect()
{
    while (1 < ref)
    {
        if (device[0])
        {
            device[0]->detect();
        }

        if (device[1])
        {
            device[1]->detect();
        }
        esSleep(5 * 10000000);
    }
    release();  // ref is incremenet for the thread executing this method
}

void* AtaController::
run(void* param)
{
    AtaController* ctlr;

    ctlr = static_cast<AtaController*>(param);
    if (ctlr->device[0] || ctlr->device[1])
    {
        ctlr->detect();
    }
    return 0;
}
