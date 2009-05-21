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

#ifndef NINTENDO_ES_KERNEL_I386_ATACONTROLLER_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_ATACONTROLLER_H_INCLUDED

#include <es/types.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/base/ICallback.h>
#include <es/base/IMonitor.h>
#include <es/base/IStream.h>
#include <es/device/IDiskManagement.h>
#include <es/device/IRemovableMedia.h>
#include <es/device/IPartition.h>
#include "ata.h"
#include "thread.h" // XXX

class AtaController;
class AtaDevice;

class AtaDma
{
    friend class AtaController;

    virtual int setup(u8 cmd, void* buffer, int count /* in byte */) = 0;
    virtual void start(u8 cmd) = 0;
    virtual int stop() = 0;
    virtual void interrupt(int count /* in byte */) = 0;
};

class AtaController : public es::Callback
{
    friend class AtaDevice;
    friend class AtaPacketDevice;

    es::Monitor*   monitor;
    Lock            lock;
    Ref             ref;

    int             cmdPort;
    int             ctlPort;
    int             irq;
    AtaDma*         dma;
    AtaDevice*      device[2];

    AtaDevice*      current;
    u8              cmd;
    u8*             data;
    u8*             limit;
    volatile bool   done;
    Rendezvous      rendezvous;

    u8              packet[16];
    u8              features;

    Thread          thread;

    bool softwareReset();
    bool detectDevice(int dev, u8* signature);

    int condDone(int);
    void wait();
    void notify();

    static bool isAtaDevice(const u8* signature);
    static bool isAtapiDevice(const u8* signature);
    static void* run(void* param);

public:
    AtaController(int cmdPort, int ctlPort, int irq, AtaDma* dma, es::Context* ata);
    ~AtaController();
    void select(u8 device);
    u8 sync(u8 status);
    int issue(AtaDevice* device, u8 cmd,
               void* buffer, int count, long long lba);
    int issue(AtaDevice* device, u8* packet, int packetSize,
              void* buffer = 0, int count = 0, u8 features = 0);
    int invoke(int);
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    void detect();
};

class AtaDevice : public es::Stream, public es::Context, public es::DiskManagement
{
    friend class AtaController;

protected:
    es::Monitor*   monitor;
    Ref             ref;
    AtaController*  ctlr;
    u8              device;
    u16             id[256];
    long long       size;
    u16             sectorSize;
    u8              readCmd;
    u8              writeCmd;
    u16             multiple;

    u16             packetSize;
    u16             dma;
    bool            removal;

    es::Partition* partition;

    bool identify(u8* signature);
    es::Partition* getPartition();

public:
    AtaDevice(AtaController* ctlr, u8 device, u8* signature);
    virtual ~AtaDevice();

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

    // IContext
    es::Binding* bind(const char* name, Object* object);
    es::Context* createSubcontext(const char* name);
    int destroySubcontext(const char* name);
    Object* lookup(const char* name);
    int rename(const char* oldName, const char* newName);
    int unbind(const char* name);
    es::Iterator* list(const char* name);

    // IDiskManagement
    int initialize();
    void getGeometry(Geometry* geometry);
    void getLayout(Partition* partition);
    void setLayout(const Partition* partition);

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    virtual bool detect();
};

class AtaPacketDevice : public AtaDevice, public es::RemovableMedia
{
    u8 testUnitReady();
    int requestSense(void* sense, int count);
    int modeSense(u8 pageCtrl, u8 pageCode, void* modeParamList, int count);
    int readCapacity();
    int startStopUnit(bool immediate, bool loEj, bool start, u8 powerCondition);
    int stopDisc();
    int startDisc();
    int preventAllowMediumRemoval(bool prevent, bool persistent = false);

public:
    AtaPacketDevice(AtaController* ctlr, u8 device, u8* signature);
    ~AtaPacketDevice();

    int eject();
    int load();
    int lock();
    int unlock();

    int read(void* dst, int count, long long offset);
    int write(const void* src, int count, long long offset);
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    virtual bool detect();
};

#endif // NINTENDO_ES_KERNEL_I386_ATACONTROLLER_H_INCLUDED
