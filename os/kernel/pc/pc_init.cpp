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

#include <new>
#include <stdlib.h>
#include <es.h>
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/reflect.h>
#include <es/base/IClassFactory.h>
#include "8042.h"
#include "8237a.h"
#include "8254.h"
#include "apic.h"
#include "ataController.h"
#include "cache.h"
#include "cga.h"
#include "classStore.h"
#include "context.h"
#include "core.h"
#include "dp8390d.h"
#include "fdc.h"
#include "heap.h"
#include "interfaceStore.h"
#include "loopback.h"
#include "mps.h"
#include "partition.h"
#include "rtc.h"
#include "sb16.h"
#include "thread.h"
#include "uart.h"
#include "vesa.h"

Reflect::Interface* getInterface(const Guid* iid);

namespace
{
    IContext*       root;
    IClassStore*    classStore;
    InterfaceStore* interfaceStore;
    Sched*          sched;
    Pit*            pit;
    IRtc*           rtc;
    IStream*        reportStream;
    IClassFactory*  alarmFactory;
    Dmac*           master;
    Dmac*           slave;
    Pic*            pic;
    Mps*            mps;
    Apic*           apic;
    u8              loopbackBuffer[64 * 1024];
};

const int Page::SIZE = 4096;
const int Page::SHIFT = 12;
const int Page::SECTOR = 512;

struct AddressRangeDesc
{
    u64 base;   // base address
    u64 size;   // length in bytes
    u32 type;   // type of address range
                //   1: available to OS
                //   2: not available
                //   3: ACPI
                //   4: NVS
};

static void initArena()
{
    extern char _end[];
#ifdef __i386__
    char* end = (char*) (reinterpret_cast<unsigned>(_end) & ~0xc0000000);   // XXX 32 bit only
#endif // __i386__

    esReport("System Memory Map\n");
    AddressRangeDesc* map;
    for (map = (AddressRangeDesc*) 0x8400;
         map->type;
         ++map)
    {
        esReport("    %016llx %016llx %d\n", map->base, map->size, map->type);
        if (map->type == 1)
        {
            char* base = (char*) map->base;
            if (base + map->size <= end)
            {
                continue;
            }
            if (base < end)
            {
                base = end;
            }
            base = (char*) (((u64) base + Page::SIZE - 1) & ~(Page::SIZE - 1));
            if ((char*) map->base + map->size <= base)
            {
                continue;
            }

            size_t size = (size_t) (map->size - (base - (char*) map->base));

            esReport("    base: %08p\n", base);
            PageTable::init(base, size);
            break;
        }
    }
    esReport("    _end: %08p\n", _end);

    free(0);    // Just to link malloc.cpp
}

static void initAP(...)
{
    apic->enableLocalApic();
    apic->splHi();
    apic->setTimer(67, 1000);
    Core* core = new Core(sched);
    apic->started();
    core->start();
    // NOT REACHED HERE
}

extern int main(int, char* []);

int esInit(IInterface** nameSpace)
{
    if (root)
    {
        if (nameSpace)
        {
            *nameSpace = root;
        }
        return 0;
    }

    Cga* cga = new Cga;
    reportStream = cga;
#if 1
    int port = ((u16*) 0x400)[0];
    if (port)
    {
        Uart* uart = new Uart(port);
        if (uart)
        {
            reportStream = uart;
        }
    }
#endif

    // Initialize 8259 anyways.
    pic = new Pic();

    // Initialize the page table
    initArena();
    ASSERT(PageTable::pageSet);

    // Initialize RTC
    rtc = new Rtc;

    // Create the thread scheduler
    sched = new Sched;

    // Initialize the current core
    Core* core = new Core(sched);

    mps = new Mps;
    if (!mps->getFloatingPointerStructure())
    {
        Core::pic = pic;
    }
    else
    {
        // Startup APs
        u32 hltAP = 0x30000 + *(u16*) (0x30000 + 138);
        u32 startAP = 0x30000 + *(u16*) (0x30000 + 126);
        *(u32*) (0x30000 + 132) = (u32) initAP;

        esReport("Startap: %x\n", startAP);
        esReport("Halt: %x\n", hltAP);

        apic = new Apic(mps);
        apic->busFreq();
        Core::pic = apic;

        Core::registerExceptionHandler(67, sched);
        apic->setTimer(67, 1000);

        apic->startup(hltAP, startAP);
    }

    // Create the default thread (stack top: 0x80010000)
    Thread* thread = new Thread(0, 0, IThread::Normal,
                                (void*) 0x80009000, 0x80010000 - 0x80009000);
    thread->state = IThread::RUNNING;
    thread->sched = sched;
    thread->func = (void* (*)(void*)) main;
    thread->core = core;
    core->current = thread;
    core->ktcb.tcb = thread->ktcb;

    pit = new Pit(1000);

    root = new Context;
    if (nameSpace)
    {
        *nameSpace = root;
    }

    IBinding* binding;

    // Create device name space
    IContext* device = root->createSubcontext("device");
    binding = device->bind("rtc", rtc);
    binding->release();
    binding = device->bind("cga", cga);
    binding->release();
    device->release();

    // Create network name space
    IContext* network = root->createSubcontext("network");
    network->release();

    // Create class store
    classStore = static_cast<IClassStore*>(new ClassStore);
    binding = root->bind("class", classStore);
    binding->release();

    // Create interface store
    interfaceStore = new InterfaceStore;
    binding = root->bind("interface", static_cast<IInterfaceStore*>(interfaceStore));
    binding->release();

    // Register CLSID_Process
    IClassFactory* processFactory = new(ClassFactory<Process>);
    classStore->add(CLSID_Process, processFactory);

    // Register CLSID_CacheFactory
    IClassFactory* cacheFactoryFactory = new(ClassFactory<CacheFactory>);
    classStore->add(CLSID_CacheFactory, cacheFactoryFactory);

    // Register CLSID_MonitorFactory
    IClassFactory* monitorFactory = new(ClassFactory<Monitor>);
    classStore->add(CLSID_Monitor, monitorFactory);

    // Register CLSID_PageSet
    classStore->add(CLSID_PageSet, static_cast<IClassFactory*>(PageTable::pageSet));

    // Register CLSID_Alarm
    alarmFactory = new(ClassFactory<Alarm>);
    classStore->add(CLSID_Alarm, alarmFactory);

    // Register CLSID_Partition
    IClassFactory* partitionFactory = new(ClassFactory<PartitionContext>);
    classStore->add(CLSID_Partition, partitionFactory);

    slave = new Dmac(0x00, 0x80, 0);
    master = new Dmac(0xc0, 0x88, 1);

    Core::pic->splLo();

    root->bind("device/beep", static_cast<IBeep*>(pit));

#if 1
    Vesa* vesa = new Vesa((u8*) 0x8000, (u8*) 0x8200,
                          (u8*) ((*(u16*) (0x30000 + 128)) | ((*(u16*) (0x30000 + 130)) << 4)),
                          device);
#endif

    Keyboard* keyboard = new Keyboard(device);

    IContext* ata = root->createSubcontext("device/ata");
    AtaController* ctlr0 = new AtaController(0x1f0, 0x3f4, 14, 0, ata);
    AtaController* ctlr1 = new AtaController(0x170, 0x374, 15, 0, ata);

#if 1
    FloppyController* fdc = new FloppyController(&slave->chan[2]);
    FloppyDrive* fdd = new FloppyDrive(fdc, 0);
    root->bind("device/floppy", static_cast<IStream*>(fdd));
#endif

    SoundBlaster16* sb16 = new SoundBlaster16(master, slave);
    ASSERT(static_cast<IStream*>(&sb16->inputLine));
    ASSERT(static_cast<IStream*>(&sb16->outputLine));
    root->bind("device/soundInput", static_cast<IStream*>(&sb16->inputLine));
    root->bind("device/soundOutput", static_cast<IStream*>(&sb16->outputLine));

    // Register the loopback interface
    Loopback* loopback = new Loopback(loopbackBuffer, sizeof loopbackBuffer);
    device->bind("loopback", static_cast<IStream*>(loopback));

    // Register the Ethernet interface
    Dp8390d* ne2000 = new Dp8390d(0xc100, 10);
    device->bind("ethernet", static_cast<IStream*>(ne2000));

    Process::initialize();

    return 0;
}

bool esCreateInstance(const Guid& rclsid, const Guid& riid, void** objectPtr)
{
    return classStore->createInstance(rclsid, riid, objectPtr);
}

void esSleep(s64 timeout)
{
    Thread* thread(Thread::getCurrentThread());

    if (thread && 20000 <= timeout)
    {
        thread->sleep(timeout);
        return;
    }

    timeout /= 10;
    while (0 < timeout--)
    {
#ifdef __i386__
        // Reading or writing a byte from/to port 0x80 take almost
        // exactly 1 microsecond independent of the processor type
        // and speed.
        __asm__ __volatile__ ("outb %%al, $0x80" ::: "%eax");
#endif
    }
}

struct Frame
{
    Frame* prev;
    void*  pc;
};

void esPanic(const char* file, int line, const char* msg, ...)
{
    va_list marker;

    va_start(marker, msg);
    esReportv(msg, marker);
    va_end(marker);
    esReport(" in \"%s\" on line %d.\n", file, line);

    Frame* frame = (Frame*) (&file - 2);
    while (frame)
    {
        esReport("%p %p\n", frame->pc, frame->prev);
        frame = frame->prev;
    }

    // XXX
    for (;;)
    {
#ifdef __i386__
        __asm__ __volatile__ ("hlt\n");
#endif
    }
}

int esReportv(const char* spec, va_list list)
{
    static Lock lock;
    unsigned x = Core::splHi();
    lock.lock();
    Formatter formatter(reportStream);
    int len = formatter.format(spec, list);
    lock.unlock();
    Core::splX(x);
    return len;
}

ICurrentProcess* esCurrentProcess()
{
    return sched;
}

IStream* esReportStream()
{
    return reportStream;
}

Reflect::Interface& getInterface(const Guid& iid)
{
    return interfaceStore->getInterface(iid);
}

IThread* esCreateThread(void* (*start)(void* param), void* param)
{
    return new Thread(start, param, IThread::Normal);
}
