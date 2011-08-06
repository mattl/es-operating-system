/*
 * Copyright 2011 Esrille Inc.
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

#include <es.h>
#include <es/context.h>
#include <es/exception.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/reflect.h>
#include "io.h"
#include "8042.h"
#include "8237a.h"
#include "8254.h"
#include "apic.h"
#include "ataController.h"
#include "cache.h"
#include "cga.h"
#include "core.h"
#include "dp8390d.h"
#include "fdc.h"
#include "heap.h"
#include "interfaceStore.h"
#include "loopback.h"
#include "mps.h"
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

namespace
{
    es::Context*    root;
    es::Context*    classStore;
    InterfaceStore* interfaceStore;
    Sched*          sched;
    Pit*            pit;
    es::Rtc*        rtc;
    es::Stream*     reportStream;
    Dmac*           master;
    Dmac*           slave;
    Pic*            pic;
    Mps*            mps;
    Apic*           apic;
    u8              loopbackBuffer[64 * 1024];
    Uart*           uart;
    Pci*            pci;
    es::Stream*     stubStream;
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

int esInit(Object** nameSpace)
{
    set_debug_traps();

    // Initialize trivial constructors.
    Alarm::initializeConstructor();
    Monitor::initializeConstructor();
    PageSet::initializeConstructor();
    Process::initializeConstructor();

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
    Thread* thread = new Thread(0, 0, es::Thread::Normal,
                                (void*) 0x80009000, 0x80010000 - 0x80009000);
    thread->state = es::Thread::RUNNING;
    thread->sched = sched;
    thread->func = (void* (*)(void*)) main;
    thread->core = core;
    core->current = thread;
    core->ktcb.tcb = thread->ktcb;

    pit = new Pit(!mps->getFloatingPointerStructure() ? 1000 : 0);

    root = new Context;
    if (nameSpace)
    {
        *nameSpace = root;
    }

    // Create class store
    classStore = root->createSubcontext("class");

    // Register IMonitor constructor
    classStore->bind(es::Monitor::iid(), es::Monitor::getConstructor());

    es::Binding* binding;

    // Create device name space
    es::Context* device = root->createSubcontext("device");
    binding = device->bind("rtc", rtc);
    binding->release();
    binding = device->bind("cga", cga);
    binding->release();
    device->release();

    // Create network name space
    es::Context* network = root->createSubcontext("network");
    network->release();

    // Create class name space
    binding = root->bind("class", classStore);
    binding->release();

    // Create interface store
    interfaceStore = new InterfaceStore;
    binding = root->bind("interface", static_cast<es::InterfaceStore*>(interfaceStore));
    binding->release();

    // Bind Process constructor
    classStore->bind(es::Process::iid(), es::Process::getConstructor());

    // Bind Cache constructor
    Cache::initializeConstructor();
    es::Cache::setConstructor(new Cache::Constructor);
    classStore->bind(es::Cache::iid(), es::Cache::getConstructor());

    // Bind PageSet constructor
    classStore->bind(es::PageSet::iid(), es::Alarm::getConstructor());

    // Bind Alarm constructor
    classStore->bind(es::Alarm::iid(), es::Alarm::getConstructor());

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
        Core::cpuid(0x00, &eax, &ebx, &ecx, &edx);
        if (0x0a <= eax) {
            Core::cpuid(0x0a, &eax, &ebx, &ecx, &edx);
            if (0 < (eax & 0x0f) && // Check the version identifier
                !(ebx & 0x04))      // Check the availability of UnHalted Reference Cycles event
            {
                esReport("Enabled NMI kernel watchdog.\n");

                apic->enableWatchdog();
            }
        }
    }

    root->bind("device/beep", static_cast<es::Beep*>(pit));

#ifdef USE_SVGA
    Vesa* vesa = new Vesa((u8*) 0x80008000, (u8*) 0x80008200,
                          (u8*) ((*(u16*) (0x80030000 + 128)) | ((*(u16*) (0x80030000 + 130)) << 4)),
                          device);
#endif

    Keyboard* keyboard = new Keyboard(device);

    es::Context* ata = root->createSubcontext("device/ata");
    AtaController* ctlr0 = new AtaController(0x1f0, 0x3f0, 14, 0, ata);
    AtaController* ctlr1 = new AtaController(0x170, 0x370, 15, 0, ata);

    FloppyController* fdc = new FloppyController(&slave->chan[2]);
    FloppyDrive* fdd = new FloppyDrive(fdc, 0);
    root->bind("device/floppy", static_cast<es::Stream*>(fdd));

#ifdef USE_SB16
    try
    {
        SoundBlaster16* sb16 = new SoundBlaster16(Core::isaBus, master, slave);
        ASSERT(static_cast<es::Stream*>(&sb16->inputLine));
        ASSERT(static_cast<es::Stream*>(&sb16->outputLine));
        root->bind("device/soundInput", static_cast<es::Stream*>(&sb16->inputLine));
        root->bind("device/soundOutput", static_cast<es::Stream*>(&sb16->outputLine));
    }
    catch (...)
    {
        esReport("sb16: not detected.\n");
    }
#endif

    // Register the loopback interface
    Loopback* loopback = new Loopback(loopbackBuffer, sizeof loopbackBuffer);
    device->bind("loopback", static_cast<es::Stream*>(loopback));

#ifdef USE_NE2000ISA
    // Register the Ethernet interface
    Dp8390d* ne2000 = new Dp8390d(Core::isaBus, 0xc100, 10);
    device->bind("ethernet", static_cast<es::Stream*>(ne2000));
#endif

    pci = new Pci(mps, device);

#ifdef USE_COM3
    ASSERT(stubStream);
    device->bind("com3", static_cast<es::Stream*>(stubStream));
#endif

    Process::initialize();

    return 0;
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

es::CurrentProcess* esCurrentProcess()
{
    return sched;
}

es::Stream* esReportStream()
{
    return reportStream;
}

es::Thread* esCreateThread(void* (*start)(void* param), void* param)
{
    return new Thread(start, param, es::Thread::Normal);
}

namespace es
{

Reflect::Interface& getInterface(const char* iid)
{
    return interfaceStore->getInterface(iid);
}

Object* getConstructor(const char* iid)
{
    return interfaceStore->getConstructor(iid);
}

const char* getUniqueIdentifier(const char* iid)
{
    return interfaceStore->getUniqueIdentifier(iid);
}

}  // namespace es

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
    int _write(int file, const char *ptr, size_t len);
    int _read(int file, char *ptr, size_t len);
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

int _write(int file, const char *ptr, size_t len)
{
    return -1;
}

int _read(int file, char *ptr, size_t len)
{
    return -1;
}
