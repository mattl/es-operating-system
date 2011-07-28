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
#include "core.h"
#include "fdc.h"
#include "io.h"

FloppyController::
FloppyController(es::Dmac* dmac, u16 base, u8 irq) :
    base(base),
    current(0),
    dmac(dmac)
{
    monitor = es::Monitor::createInstance();

    // motor off
    for (int i = 3; 0 <= i; --i)
    {
        outpb(base + DOR, 0x0c | i);
    }
    motor = 0;

    outpb(base + DSR, 0);

    Core::registerInterruptHandler(irq, this);
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

Object* FloppyController::
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

unsigned int FloppyController::
addRef()
{
    return ref.addRef();
}

unsigned int FloppyController::
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
