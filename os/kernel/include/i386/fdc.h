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

#ifndef NINTENDO_ES_KERNEL_I386_FDC_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_FDC_H_INCLUDED

#include <es/dateTime.h>
#include <es/list.h>
#include <es/types.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/base/IAlarm.h>
#include <es/base/ICallback.h>
#include <es/base/IMonitor.h>
#include <es/base/IStream.h>
#include <es/device/IDisk.h>
#include <es/device/IDmac.h>
#include <es/device/IRemovableMedia.h>

class FloppyController;
class FloppyDrive;

class FloppyDrive : public es::Disk, public es::Callback // public es::RemovableMedia
{
    friend class FloppyController;

    static void* const dmaBuffer;

    FloppyController*   ctlr;
    u8                  drive;
    u8                  rate;
    u8                  cylinder;
    u8                  head;
    u8                  record;
    u8                  recordLength;   // 128 * (2 << recordLength)

    u8                  eot;
    u8                  gpl;
    u8                  fgpl;

    es::Alarm*             alarm;
    DateTime            motor;

    Link<FloppyDrive>   link;

    void on();      // start motor
    void off();     // stop motor
    bool isChanged();
    int checkDiskChange();
    int sense();
    void recalibrate();

public:
    FloppyDrive(FloppyController* ctlr, u8 drive);
    ~FloppyDrive();

    int initialize();   // XXX how this is called?

    // ICallback
    int invoke(int);

    // IStream
    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    // IDisk
    unsigned int getHeads();
    unsigned int getCylinders();
    unsigned int getSectorsPerTrack();
    unsigned int getBytesPerSector();
    long long getDiskSize();

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

class FloppyController : public es::Callback
{
    friend class FloppyDrive;

    // Registers
    static const u8 SRA = 0;    // read
    static const u8 SRB = 1;    // read
    static const u8 DOR = 2;    // read
    static const u8 TDR = 3;    // read/write
    static const u8 MSR = 4;    // read
    static const u8 DSR = 4;    // write
    static const u8 FIFO = 5;   // read/write
    static const u8 DIR = 7;    // read
    static const u8 CCR = 7;    // write

    // Commands
    static const u8 RECALIBRATE = 0X07;
    static const u8 SEEK = 0X0F;
    static const u8 SENSE = 0X08;
    static const u8 READ = 0X66;
    static const u8 READID = 0X4A;
    static const u8 SPEC = 0X03;
    static const u8 WRITE = 0X45;
    static const u8 FORMAT = 0X4D;

    Ref             ref;
    u16             base;
    u8              motor;

    es::Monitor*       monitor;
    es::Dmac*          dmac;
    FloppyDrive*    current;
    u8              command[14];
    int             commandLength;
    u8              status[14];
    int             statusLength;
    bool            done;

    List<FloppyDrive, &FloppyDrive::link>
                    drives;

    void issue(FloppyDrive* drive, u8 cmd, void* param);
    int result();

public:
    FloppyController(es::Dmac* dmac, u16 base = 0x3f0, u8 irq = 6);
    ~FloppyController();

    // ICallback
    int invoke(int);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // NINTENDO_ES_KERNEL_I386_FDC_H_INCLUDED
