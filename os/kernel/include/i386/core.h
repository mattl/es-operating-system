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

#ifndef NINTENDO_ES_KERNEL_I386_CORE_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_CORE_H_INCLUDED

#include <es/base/ICallback.h>
#include "process.h"
#include "thread.h"
#include "8259.h"

class Core
{
    friend class Process;
    friend class Thread;
    friend struct Xreg;
    friend int esInit(IInterface** nameSpace);

    // Thread control block (a read-only region from the user level threads)
    struct Tcb
    {
        void*           tcb;
    };

public:
    static const int MaxCore = 32;

    static const u16 KCODESEL = 8;
    static const u16 KDATASEL = 16;
    static const u16 RCODESEL = 24;     // for ES.LDR
    static const u16 RDATASEL = 32;     // for ES.LDR
    static const u16 KTCBSEL = 40;      // for kernel TCB
    static const u16 TSSSEL = 48;
    static const u16 UCODESEL = 56 | 3;
    static const u16 UDATASEL = 64 | 3;
    static const u16 TCBSEL = 72 | 3;   // for TCB

private:
    long                id;
    Sched*              sched;
    Process*            currentProc;
    Thread*             current;
    Thread*             currentFPU;
    bool                yieldable;

    Label               label;
    Tss*                tss;            // 256 byte aligned
    Tcb*                tcb;
    Tcb*                ktcb;
    Segdesc             gdt[10] __attribute__ ((aligned (16)));
    SegdescLoc          gdtLoc;

    static Ref          numCores;
    static Core*        coreTable[MaxCore];

    static bool         fxsr;
    static bool         sse;
    static Segdesc      idt[256] __attribute__ ((aligned (16)));
    static SegdescLoc   idtLoc;

    // 8259 related
    static SpinLock     spinLock;
    static ICallback*   exceptionHandlers[255];
    static IPic*        pic;

public:
    Core(Sched* sched, void* stack, unsigned stackSize, Tss* tss);

    static Core* getCurrentCore();
    static void reschedule(void* param);
    static void dispatchException(Ureg* ureg);

    static long registerExceptionHandler(u8 exceptionNumber, ICallback* callback);
    static long unregisterExceptionHandler(u8 exceptionNumber, ICallback* callback);

    // i386 specific
    static void cpuid(int op, int* eax, int* ebx, int* ecx, int* edx);
    static void initFPU();
    static void enableFPU();
    static void disableFPU();
    static void shutdown();

} __attribute__ ((aligned (16)));

#endif  // NINTENDO_ES_KERNEL_I386_CORE_H_INCLUDED
