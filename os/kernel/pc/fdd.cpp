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
#include <errno.h>
#include <es.h>
#include <es/exception.h>
#include <es/clsid.h>
#include "fdc.h"
#include "io.h"

void* const FloppyDrive::dmaBuffer = (void*) 0x80020000;

FloppyDrive::
FloppyDrive(FloppyController* ctlr, u8 drive) :
    ctlr(ctlr),
    drive(drive),
    rate(0)
{
    ctlr->drives.addLast(this);
    ctlr->addRef();

    cylinder = 0;
    head = 0;
    record = 1;
    recordLength = 2;
    eot = 18;
    gpl = 0x1b;
    fgpl = 0x54;

    alarm = reinterpret_cast<IAlarm*>(
        esCreateInstance(CLSID_Alarm, IAlarm::iid()));

    alarm->setEnabled(false);
    alarm->setInterval(10000000);
    alarm->setPeriodic(true);
    alarm->setCallback(this);

    isChanged();    // Clear the DIR flag.
    on();
    recalibrate();
}

FloppyDrive::
~FloppyDrive()
{
    ctlr->drives.remove(this);

    if (alarm)
    {
        alarm->setEnabled(false);
        alarm->release();
    }
}

void FloppyDrive::
on()
{
    motor = DateTime::getNow();
    bool ready = ctlr->motor & (1u << drive);
    ctlr->motor |= (1u << drive);
    outpb(ctlr->base + FloppyController::DOR, (ctlr->motor << 4) | 0x0c | drive);
    if (!ready)
    {
        esSleep(10000000);
        motor = DateTime::getNow();
        alarm->setEnabled(true);
    }
}

void FloppyDrive::
off()
{
    ctlr->motor &= ~(1u << drive);
    outpb(ctlr->base + FloppyController::DOR, (ctlr->motor << 4) | 0x0c | drive);
    alarm->setEnabled(false);
}

bool FloppyDrive::isChanged()
{
    if (inpb(ctlr->base + FloppyController::DIR) & 0x80)
    {
        on();
        recalibrate();
        return true;
    }
    return false;
}

int FloppyDrive::
invoke(int)
{
    if (30000000 < DateTime::getNow().getTicks() - motor.getTicks())
    {
        off();
    }
}

int FloppyDrive::
sense()
{
    ctlr->issue(this, FloppyController::SENSE, 0);
    return ctlr->result();
}

void FloppyDrive::
recalibrate()
{
    do
    {
        ctlr->issue(this, FloppyController::RECALIBRATE, NULL);
        if (ctlr->statusLength < 2)
        {
            esReport("recalibrate: confused.\n");
            break;
        }
        if (ctlr->status[0] & 0xc0)
        {
            esReport("recalibrate: failed.\n");
            break;
        }
        cylinder = ctlr->status[1];
    } while (cylinder != 0);
}

long long FloppyDrive::
getPosition()
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    long sectorSize = 128 * (1 << recordLength);
    return sectorSize * ((2 * cylinder + head) * eot + record - 1);
}

void FloppyDrive::
setPosition(long long offset)
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    long sectorSize = 128 * (1 << recordLength);
    long sector = offset / sectorSize;
    record = (sector % eot) + 1;
    head = (sector / eot) % 2;
    u8 target = (sector / eot) / 2;

    if (cylinder == target)
    {
        return;
    }

    cylinder = target;
    ctlr->issue(this, FloppyController::SEEK, 0);
    if (ctlr->statusLength < 2)
    {
        esReport("setPosition: confused.\n");
        return;
    }
    if ((ctlr->status[0] & (0xc0 | 0x20)) != 0x20)
    {
        esReport("setPosition: failed (%x)\n", ctlr->status[0]);
        return;
    }
}

int FloppyDrive::
read(void* dst, int count)
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    int sectorSize = 128 * (1 << recordLength);
    if (count < sectorSize)
    {
        return 0;
    }

    if (isChanged())
    {
        throw SystemException<ENOMEDIUM>();
    }

    on();
    ctlr->dmac->setup(dmaBuffer, sectorSize, IDmac::READ);
    ctlr->dmac->start();
    ctlr->issue(this, FloppyController::READ, NULL);
    ctlr->dmac->stop();
    if (ctlr->statusLength < 7)
    {
        esReport("read: confused.\n");
        return 0;
    }
    if ((ctlr->status[0] & 0xc0) != 0 || ctlr->status[1] || ctlr->status[2])
    {
        esReport("read: failed (%x)\n", ctlr->status[0]);
        return 0;
    }
    memmove(dst, dmaBuffer, sectorSize);
    return sectorSize;
}

int FloppyDrive::
read(void* dst, int count, long long offset)
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    if (isChanged())
    {
        throw SystemException<ENOMEDIUM>();
    }

    on();
    int len;
    int n;
    u8* ptr = (u8*) dst;
    for (len = 0; len < count; len += n, offset += n, ptr += n)
    {
        setPosition(offset);
        n = read(ptr, count - len);
        if (n <= 0)
        {
            break;
        }
    }
    return len;
}

int FloppyDrive::
write(const void* src, int count)
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    int sectorSize = 128 * (1 << recordLength);
    if (count < sectorSize)
    {
        return 0;
    }

    if (isChanged())
    {
        throw SystemException<ENOMEDIUM>();
    }

    on();
    memmove(dmaBuffer, src, sectorSize);
    ctlr->dmac->setup(dmaBuffer, sectorSize, IDmac::WRITE);
    ctlr->dmac->start();
    ctlr->issue(this, FloppyController::WRITE, NULL);
    ctlr->dmac->stop();
    if (ctlr->statusLength < 7)
    {
        esReport("write: confused.\n");
        return 0;
    }
    if ((ctlr->status[0] & 0xc0) != 0 || ctlr->status[1] || ctlr->status[2])
    {
        esReport("write: failed (%x)\n", ctlr->status[0]);
        return 0;
    }
    return sectorSize;
}

int FloppyDrive::
write(const void* src, int count, long long offset)
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    if (isChanged())
    {
        throw SystemException<ENOMEDIUM>();
    }

    on();
    int len;
    int n;
    const u8* ptr = (const u8*) src;
    for (len = 0; len < count; len += n, offset += n, ptr += n)
    {
        setPosition(offset);
        n = write(ptr, count - len);
        if (n <= 0)
        {
            break;
        }
    }
    return len;
}

long long FloppyDrive::
getSize()
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    long long size = 80 * 2 * eot * 128 * (1 << recordLength);
    return size;
}

void FloppyDrive::
setSize(long long size)
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    //                         rate
    // 1440: 512 * 18 * 2 * 80    0
    //  720: 512 *  9 * 2 * 80    2
    switch (size)
    {
    case 512 * 18 * 2 * 80:
        eot = 18;
        rate = 0;
        break;
    case 512 * 9 * 2 * 80:
        eot = 9;
        rate = 2;
        break;
    }
}

void FloppyDrive::
flush()
{
}

int FloppyDrive::
initialize()
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    int sectorSize = 128 * (1 << recordLength);

    on();
    for (int track = 0; track < (unsigned int) 2 * 80; ++track)
    {
        setPosition(track * eot * sectorSize);

        u8* bp = (u8*) dmaBuffer;
        for (int record = 1; record <= eot; ++record)
        {
            *bp++ = cylinder;
            *bp++ = head;
            *bp++ = record;
            *bp++ = recordLength;
        }

        ctlr->dmac->setup(dmaBuffer, 4 * eot, IDmac::WRITE);
        ctlr->dmac->start();
        ctlr->issue(this, FloppyController::FORMAT, 0);
        ctlr->dmac->stop();

        if (ctlr->statusLength < 7)
        {
            esReport("initialize: confused\n");
            return -1;
        }
        if ((ctlr->status[0] & 0xc0) != 0 || ctlr->status[1]|| ctlr->status[2])
        {
            esReport("initialize: failed %ux %ux %ux\n",
                     ctlr->status[0], ctlr->status[1], ctlr->status[2]);
            return -1;
        }
    }
    return 0;
}

void FloppyDrive::
getGeometry(Geometry* geometry)
{
    Synchronized<IMonitor*> method(ctlr->monitor);

    geometry->cylinders = 80;
    geometry->heads = 2;
    geometry->sectorsPerTrack = eot;
    geometry->bytesPerSector = 128 * (1 << recordLength);
    geometry->diskSize = geometry->cylinders *
                         geometry->heads *
                         geometry->sectorsPerTrack *
                         geometry->bytesPerSector;
}

void FloppyDrive::
getLayout(Partition* partition)
{
}

void FloppyDrive::
setLayout(const Partition* partition)
{
}

void* FloppyDrive::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IDiskManagement::iid())
    {
        objectPtr = static_cast<IDiskManagement*>(this);
    }
    else if (riid == IStream::iid())
    {
        objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IDiskManagement*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int FloppyDrive::
addRef()
{
    return ctlr->ref.addRef();
}

unsigned int FloppyDrive::
release()
{
    unsigned int count = ctlr->ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
