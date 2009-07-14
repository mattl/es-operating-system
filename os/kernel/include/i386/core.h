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

#ifndef NINTENDO_ES_KERNEL_I386_CORE_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_CORE_H_INCLUDED

#include <new>
#include <es/object.h>
#include <es/base/ICallback.h>
#include "cache.h"
#include "process.h"
#include "thread.h"
#include "8259.h"

class Core
{
    friend class Process;
    friend class Thread;
    friend struct Xreg;
    friend int esInit(Object** nameSpace);

    // Thread control block (a read-only region from the user level threads)
    struct Tcb
    {
        void*           tcb;
        Tcb() : tcb(0)
        {
        }
    };

public:
    static const int CORE_MAX = 8;
    static const int CORE_SIZE = 16384;

    static const u16 KCODESEL = 8;
    static const u16 KDATASEL = 16;
    static const u16 RCODESEL = 24;     // for ES.LDR
    static const u16 RDATASEL = 32;     // for ES.LDR
    static const u16 KTCBSEL = 40;      // for kernel TCB
    static const u16 TSSSEL = 48;
    static const u16 UCODESEL = 56 | 3;
    static const u16 UDATASEL = 64 | 3;
    static const u16 TCBSEL = 72 | 3;   // for TCB
    static const u16 TSS1SEL = 80;

private:
    u8                  id;             // Core ID (may not be equal to local APIC ID)
    Sched*              sched;
    Process*            currentProc;
    Thread*             current;
    Thread*             currentFPU;
    Interlocked         freeze;

    void*               stack;
    Label               label;
    Tss*                tss0;           // 128 byte aligned
    Tss*                tss1;           // 128 byte aligned
    Tcb*                tcb;
    Tcb                 ktcb;
    Segdesc             gdt[11] __attribute__ ((aligned (16)));
    SegdescLoc          gdtLoc;

    static Core*        coreTable[CORE_MAX];

    static bool         fxsr;
    static bool         sse;
    static Segdesc      idt[256] __attribute__ ((aligned (16)));
    static SegdescLoc   idtLoc;

    // 8259/APIC related
    static Lock         spinLock;
    static es::Callback*   exceptionHandlers[255];
    static es::Pic*        pic;
    static u8           isaBus;     // ISA Bus #

public:
    Core(Sched* sched);

    void setID(u8 id)
    {
        this->id = id;
    }

    u8 getID()
    {
        return id;
    }

    void start()
    {
        label.jump();     // Jump to reschedule().
    }

    bool checkStack();

    static Core* getCurrentCore();
    static void reschedule(void* param);
    static void dispatchException(Ureg* ureg);

    static long registerExceptionHandler(u8 exceptionNumber, es::Callback* callback);
    static long unregisterExceptionHandler(u8 exceptionNumber, es::Callback* callback);

    static long registerInterruptHandler(u8 bus, u8 irq, es::Callback* callback);
    static long unregisterInterruptHandler(u8 bus, u8 irq, es::Callback* callback);

    static long registerInterruptHandler(u8 irq, es::Callback* callback)
    {
        return registerInterruptHandler(isaBus, irq, callback);
    }
    static long unregisterInterruptHandler(u8 irq, es::Callback* callback)
    {
        return unregisterInterruptHandler(isaBus, irq, callback);
    }

    // processor execution level
    static unsigned int splIdle()
    {
        return pic->splIdle();
    }
    static unsigned int splLo()
    {
        return pic->splLo();
    }
    static unsigned int splHi()
    {
        return pic->splHi();
    }
    static void splX(unsigned int x)
    {
        pic->splX(x);
    }

    // i386 specific
    static void cpuid(int op, int* eax, int* ebx, int* ecx, int* edx);
    static void initFPU();
    static void enableFPU();
    static void disableFPU();
    static void shutdown();
    static void doubleFault();

    // class specific allocator
    static void* operator new(size_t size) throw(std::bad_alloc);
    static void operator delete(void*) throw();

} __attribute__ ((aligned (16)));

int esInit(Object** nameSpace);
es::Thread* esCreateThread(void* (*start)(void* param), void* param);

#endif  // NINTENDO_ES_KERNEL_I386_CORE_H_INCLUDED
