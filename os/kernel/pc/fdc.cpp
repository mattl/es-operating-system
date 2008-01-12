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
#include "core.h"
#include "fdc.h"
#include "io.h"

FloppyController::
FloppyController(IDmac* dmac, u16 base, u8 irq) :
    base(base),
    current(0),
    dmac(dmac)
{
    esCreateInstance(CLSID_Monitor,
                     IID_IMonitor,
                     reinterpret_cast<void**>(&monitor));

    // motor off
    for (int i = 3; 0 <= i; --i)
    {
        outpb(base + DOR, 0x0c | i);
    }
    motor = 0;

    outpb(base + DSR, 0);

    Core::registerExceptionHandler(32 + irq, this);
}

FloppyController::
~FloppyController()
{
    if (monitor)
    {
        monitor->release();
    }
}

void FloppyController::
issue(FloppyDrive* drive, u8 cmd, void* param)
{
    done = false;

    commandLength = 0;
    switch (cmd)
    {
    case SENSE:
        command[commandLength++] = cmd;
        done = true;
        break;
    case RECALIBRATE:
        command[commandLength++] = cmd;
        command[commandLength++] = drive->drive;
        break;
    case SEEK:
        command[commandLength++] = cmd;
        command[commandLength++] = (drive->head << 2) | drive->drive;
        command[commandLength++] = drive->cylinder;
        break;
    case READ:
    case WRITE:
        command[commandLength++] = cmd;
        command[commandLength++] = (drive->head << 2) | drive->drive;
        command[commandLength++] = drive->cylinder;
        command[commandLength++] = drive->head;
        command[commandLength++] = drive->record;
        command[commandLength++] = drive->recordLength;
        command[commandLength++] = drive->eot;
        command[commandLength++] = drive->gpl;
        command[commandLength++] = 0xFF;
        break;
    case FORMAT:
        command[commandLength++] = cmd;
        command[commandLength++] = (drive->head << 2) | drive->drive;
        command[commandLength++] = drive->recordLength;
        command[commandLength++] = drive->eot;
        command[commandLength++] = drive->fgpl;
        command[commandLength++] = 0x00;
        break;
    default:
        esReport("FloppyController: unsupported command %0x2\n", cmd);
        break;
    }

    current = drive;
    statusLength = 0;
    for (int i = 0; i < commandLength; i++)
    {
        u8 msr;

        do
        {
            msr = inpb(base + MSR);
        } while (!(msr & 0x80));
        if (msr & 0x40)
        {
            result();
            done = true;
            break;
        }
        outpb(base + FIFO, command[i]);
    }

    while (!done)
    {
        monitor->wait();
    }
}

int FloppyController::
invoke(int param)
{
    if (!current)
    {
        return -1;
    }

    switch (command[0])
    {
      case READ:
      case WRITE:
      case FORMAT:
        result();
        done = true;
        break;

      case SEEK:
      case RECALIBRATE:
      default:
        current->sense();
        break;
    }

    if (done)
    {
        current = 0;
        monitor->notify();  // XXX Fix timing issue
    }
}

int FloppyController::
result(void)
{
    for (int i = 0; i < sizeof(status); ++i)
    {
        u8 msr;
        do
        {
            msr = inpb(base + MSR);
            if ((msr & 0xc0) == 0x80)
            {
                statusLength = i;
                return statusLength;
            }
        } while ((msr & 0xc0) != 0xc0);
        status[i] = inpb(base + FIFO);
    }
    statusLength = sizeof(status);
    return statusLength;
}

bool FloppyController::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_ICallback)
    {
        *objectPtr = static_cast<ICallback*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<ICallback*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int FloppyController::
addRef(void)
{
    return ref.addRef();
}

unsigned int FloppyController::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
