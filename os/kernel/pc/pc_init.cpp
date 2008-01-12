/*
 * Copyright (c) 2006, 2007
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

#include <stdlib.h>
#include <es.h>
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/context.h>
#include <es/exception.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/reflect.h>
#include <es/base/IClassFactory.h>
#include "io.h"
#include "8042.h"
#include "8237a.h"
#include "8254.h"
#include "apic.h"
#include "ataController.h"
#include "cache.h"
#include "cga.h"
#include "classStore.h"
#include "core.h"
#include "dp8390d.h"
#include "fdc.h"
#include "heap.h"
#include "interfaceStore.h"
#include "loopback.h"
#include "mps.h"
#include "partition.h"
#include "i386/pci.h"
#include "rtc.h"
#include "sb16.h"
#include "thread.h"
#include "uart.h"
#include "vesa.h"

#define USE_SVGA
#define GDB_STUB
#define USE_COM1
// #define USE_COM3
#define USE_SB16
// #define USE_NE2000ISA

void putDebugChar(int ch);

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
    Uart*           uart;
    Pci*            pci;
    IStream*        stubStream;
};

const int Page::SIZE = 4096;
const int Page::SHIFT = 12;
const int Page::SECTOR = 512;

Interlocked esShutdownCount;

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
    apic->setTimer(32, 1000);
    Core* core = new Core(sched);
    core->start();
    // NOT REACHED HERE
}

extern int main(int, char* []);

extern void set_debug_traps(void);
extern void breakpoint(void);

int esInit(IInterface** nameSpace)
{
    set_debug_traps();

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
        Core::isaBus = mps->getISABusID();
        esReport("ISA Bus #: %d\n", Core::isaBus);

        apic = new Apic(mps);
        apic->busFreq();
        Core::pic = apic;

        Core::registerExceptionHandler(32, apic);
        if (2 <= mps->getProcessorCount())
        {
            // Startup APs
            u32 hltAP = 0x30000 + *(u16*) (0x30000 + 138);
            u32 startAP = 0x30000 + *(u16*) (0x30000 + 126);
            *(volatile u32*) (0x30000 + 132) = (u32) initAP;
            apic->startupAllAP(hltAP, startAP);
        }
        apic->setTimer(32, 1000);
    }

    // Invalidate 0-2GB region for debug purposes.
    u32* table = static_cast<u32*>(Mmu::getPointer(0x10000));
    for (int i = 0; i < 512; ++i)
    {
        table[i] = 0;
    }
    __asm__ __volatile__ (
        "movl   $0x10000, %%eax\n"
        "movl   %%eax, %%cr3\n"
        ::: "%eax");

#ifdef USE_COM1
    // COM1 for stdio
    if (int port = ((u16*) 0x80000400)[0])
    {
        Uart* uart = new Uart(port, Core::isaBus, 4);
        if (uart)
        {
            reportStream = uart;
        }
    }
#endif

#ifdef GDB_STUB
    // COM2 for gdb
    if (int port = ((u16*) 0x80000400)[1])
    {
        uart = new Uart(port);
    }
#endif

#ifdef USE_COM3
    // COM3 for network test stub
    if (int port = ((u16*) 0x80000400)[2])
    {
        Uart* uart = new Uart(port, Core::isaBus, 4);
        if (uart)
        {
            stubStream = uart;
            ASSERT(stubStream);
        }
    }
#endif

    // Create the default thread (stack top: 0x80010000)
    Thread* thread = new Thread(0, 0, IThread::Normal,
                                (void*) 0x80009000, 0x80010000 - 0x80009000);
    thread->state = IThread::RUNNING;
    thread->sched = sched;
    thread->func = (void* (*)(void*)) main;
    thread->core = core;
    core->current = thread;
    core->ktcb.tcb = thread->ktcb;

    pit = new Pit(!mps->getFloatingPointerStructure() ? 1000 : 0);

    // Create class store
    classStore = static_cast<IClassStore*>(new ClassStore);

    // Register CLSID_MonitorFactory
    IClassFactory* monitorFactory = new(ClassFactory<Monitor>);
    classStore->add(CLSID_Monitor, monitorFactory);

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

    // Create class name space
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

    if (uart)
    {
        // Set break point here if using a debbugger.
        breakpoint();
        //  Core::pic->startup(4);
    }
    else if (apic)
    {
        // Check Architectural Performance Monitoring leaf.
        int eax, ebx, ecx, edx;
        Core::cpuid(0x0a, &eax, &ebx, &ecx, &edx);
        if (0 < (eax & 0x0f) && // Check the version identifier
            !(ebx & 0x04))      // Check the availability of UnHalted Reference Cycles event
        {
            esReport("Enabled NMI kernel watchdog.\n");

            apic->enableWatchdog();
        }
    }

    root->bind("device/beep", static_cast<IBeep*>(pit));

#ifdef USE_SVGA
    Vesa* vesa = new Vesa((u8*) 0x80008000, (u8*) 0x80008200,
                          (u8*) ((*(u16*) (0x80030000 + 128)) | ((*(u16*) (0x80030000 + 130)) << 4)),
                          device);
#endif

    Keyboard* keyboard = new Keyboard(device);

    IContext* ata = root->createSubcontext("device/ata");
    AtaController* ctlr0 = new AtaController(0x1f0, 0x3f0, 14, 0, ata);
    AtaController* ctlr1 = new AtaController(0x170, 0x370, 15, 0, ata);

    FloppyController* fdc = new FloppyController(&slave->chan[2]);
    FloppyDrive* fdd = new FloppyDrive(fdc, 0);
    root->bind("device/floppy", static_cast<IStream*>(fdd));

#ifdef USE_SB16
    try
    {
        SoundBlaster16* sb16 = new SoundBlaster16(Core::isaBus, master, slave);
        ASSERT(static_cast<IStream*>(&sb16->inputLine));
        ASSERT(static_cast<IStream*>(&sb16->outputLine));
        root->bind("device/soundInput", static_cast<IStream*>(&sb16->inputLine));
        root->bind("device/soundOutput", static_cast<IStream*>(&sb16->outputLine));
    }
    catch (...)
    {
        esReport("sb16: not detected.\n");
    }
#endif

    // Register the loopback interface
    Loopback* loopback = new Loopback(loopbackBuffer, sizeof loopbackBuffer);
    device->bind("loopback", static_cast<IStream*>(loopback));

#ifdef USE_NE2000ISA
    // Register the Ethernet interface
    Dp8390d* ne2000 = new Dp8390d(Core::isaBus, 0xc100, 10);
    device->bind("ethernet", static_cast<IStream*>(ne2000));
#endif

    pci = new Pci(mps, device);

#ifdef USE_COM3
    ASSERT(stubStream);
    device->bind("com3", static_cast<IStream*>(stubStream));
#endif

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

    if (thread && 20000 < timeout)
    {
        thread->sleep(timeout);
        return;
    }

    timeout += 9;
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

bool nmiHandler()
{
    if (apic)
    {
        apic->enableWatchdog();
    }
    return true;
}

/* write a single character      */
void putDebugChar(int ch)
{
    u8 data = (u8) ch;
    uart->write(&data, 1);
}

/* read and return a single char */
int getDebugChar()
{
    u8 data;
    while (uart->read(&data, 1) <= 0)
    {
    }
    return data;
}

extern "C"
{
    int _close(int file);
    void _exit(int i);
}

int _close(int file)
{
    return 0;
}

void _exit(int i)
{
    if (!mps->getFloatingPointerStructure())
    {
        Core::shutdown();
    }
    else
    {
        esShutdownCount.exchange(mps->getProcessorCount());
        Core::splIdle();
    }
    for (;;)
    {
        __asm__ __volatile__ ("hlt");
    }
}
