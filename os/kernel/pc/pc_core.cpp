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

#include <stdexcept>
#include <errno.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include "core.h"

extern "C" void _exit(int i);

Ref Core::numCores(0);
Core* Core::coreTable[Core::MaxCore];
bool Core::fxsr;
bool Core::sse;
Segdesc Core::idt[256] __attribute__ ((aligned (16)));
SegdescLoc Core::idtLoc(sizeof idt - 1, idt);
SpinLock Core::spinLock;
ICallback* Core::exceptionHandlers[255];
IPic* Core::pic;

extern "C"
{
    asm("catchException:");
    asm("   pushl   %ds");
    asm("   pushl   %es");
    asm("   pushl   %fs");
    asm("   pushl   %gs");
    asm("   pushal");
    asm("   movw    %ss,%ax");  // switch to kernel segments
    asm("   movw    %ax,%ds");
    asm("   movw    %ax,%es");
    asm("   movw    %ax,%fs");

    asm("   movw    $40,%ax");  // load KTCBSEL to %gs
    asm("   movw    %ax,%gs");

    asm("   movl    %esp, %eax");
    asm("   pushl   56(%eax)");
    asm("   pushl   8(%eax)");
    asm("   movl    %esp, %ebp");
    asm("   pushl   %eax");

    asm("   call    _ZN4Core17dispatchExceptionEP4Ureg");

    asm("   addl    $12,%esp");
    asm("   popal");
    asm("   popl    %gs");
    asm("   popl    %fs");
    asm("   popl    %es");
    asm("   popl    %ds");
    asm("   addl    $8,%esp");  // error code and trap type
    asm("   iretl");

    #define EXCEPTION(v)            \
    asm("   .text");                \
    asm("   .align  16, 0");        \
    asm("exception" #v ":");        \
    asm("   pushl   $0");           \
    asm("   pushl   $(" #v ")");    \
    asm("   jmp     catchException");

    // for exception 8, 10, 11, 12, 13, 14, 17
    #define EXCEPTION_ERR(v)        \
    asm("   .text");                \
    asm("   .align  16, 0");        \
    asm("exception" #v ":");        \
    asm("   pushl   $(" #v ")");    \
    asm("   jmp     catchException");

    void exceptionTable();
    asm("   .text");
    asm("   .align  16, 0");
    asm("   .local  exceptionTable");
    asm("exceptionTable:");

    EXCEPTION(0)
    EXCEPTION(1)
    EXCEPTION(2)
    EXCEPTION(3)
    EXCEPTION(4)
    EXCEPTION(5)
    EXCEPTION(6)
    EXCEPTION(7)
    EXCEPTION_ERR(8)
    EXCEPTION(9)
    EXCEPTION_ERR(10)
    EXCEPTION_ERR(11)
    EXCEPTION_ERR(12)
    EXCEPTION_ERR(13)
    EXCEPTION_ERR(14)
    EXCEPTION(15)
    EXCEPTION(16)
    EXCEPTION_ERR(17)
    EXCEPTION(18)
    EXCEPTION(19)
    EXCEPTION(20)
    EXCEPTION(21)
    EXCEPTION(22)
    EXCEPTION(23)
    EXCEPTION(24)
    EXCEPTION(25)
    EXCEPTION(26)
    EXCEPTION(27)
    EXCEPTION(28)
    EXCEPTION(29)
    EXCEPTION(30)
    EXCEPTION(31)
    EXCEPTION(32)
    EXCEPTION(33)
    EXCEPTION(34)
    EXCEPTION(35)
    EXCEPTION(36)
    EXCEPTION(37)
    EXCEPTION(38)
    EXCEPTION(39)
    EXCEPTION(40)
    EXCEPTION(41)
    EXCEPTION(42)
    EXCEPTION(43)
    EXCEPTION(44)
    EXCEPTION(45)
    EXCEPTION(46)
    EXCEPTION(47)
    EXCEPTION(48)
    EXCEPTION(49)
    EXCEPTION(50)
    EXCEPTION(51)
    EXCEPTION(52)
    EXCEPTION(53)
    EXCEPTION(54)
    EXCEPTION(55)
    EXCEPTION(56)
    EXCEPTION(57)
    EXCEPTION(58)
    EXCEPTION(59)
    EXCEPTION(60)
    EXCEPTION(61)
    EXCEPTION(62)
    EXCEPTION(63)
    EXCEPTION(64)
    EXCEPTION(65)
    EXCEPTION(66)
    EXCEPTION(67)
    EXCEPTION(68)
    EXCEPTION(69)
    EXCEPTION(70)
    EXCEPTION(71)
    EXCEPTION(72)
    EXCEPTION(73)
    EXCEPTION(74)
    EXCEPTION(75)
    EXCEPTION(76)
    EXCEPTION(77)
    EXCEPTION(78)
    EXCEPTION(79)
    EXCEPTION(80)
    EXCEPTION(81)
    EXCEPTION(82)
    EXCEPTION(83)
    EXCEPTION(84)
    EXCEPTION(85)
    EXCEPTION(86)
    EXCEPTION(87)
    EXCEPTION(88)
    EXCEPTION(89)
    EXCEPTION(90)
    EXCEPTION(91)
    EXCEPTION(92)
    EXCEPTION(93)
    EXCEPTION(94)
    EXCEPTION(95)
    EXCEPTION(96)
    EXCEPTION(97)
    EXCEPTION(98)
    EXCEPTION(99)
    EXCEPTION(100)
    EXCEPTION(101)
    EXCEPTION(102)
    EXCEPTION(103)
    EXCEPTION(104)
    EXCEPTION(105)
    EXCEPTION(106)
    EXCEPTION(107)
    EXCEPTION(108)
    EXCEPTION(109)
    EXCEPTION(110)
    EXCEPTION(111)
    EXCEPTION(112)
    EXCEPTION(113)
    EXCEPTION(114)
    EXCEPTION(115)
    EXCEPTION(116)
    EXCEPTION(117)
    EXCEPTION(118)
    EXCEPTION(119)
    EXCEPTION(120)
    EXCEPTION(121)
    EXCEPTION(122)
    EXCEPTION(123)
    EXCEPTION(124)
    EXCEPTION(125)
    EXCEPTION(126)
    EXCEPTION(127)
    EXCEPTION(128)
    EXCEPTION(129)
    EXCEPTION(130)
    EXCEPTION(131)
    EXCEPTION(132)
    EXCEPTION(133)
    EXCEPTION(134)
    EXCEPTION(135)
    EXCEPTION(136)
    EXCEPTION(137)
    EXCEPTION(138)
    EXCEPTION(139)
    EXCEPTION(140)
    EXCEPTION(141)
    EXCEPTION(142)
    EXCEPTION(143)
    EXCEPTION(144)
    EXCEPTION(145)
    EXCEPTION(146)
    EXCEPTION(147)
    EXCEPTION(148)
    EXCEPTION(149)
    EXCEPTION(150)
    EXCEPTION(151)
    EXCEPTION(152)
    EXCEPTION(153)
    EXCEPTION(154)
    EXCEPTION(155)
    EXCEPTION(156)
    EXCEPTION(157)
    EXCEPTION(158)
    EXCEPTION(159)
    EXCEPTION(160)
    EXCEPTION(161)
    EXCEPTION(162)
    EXCEPTION(163)
    EXCEPTION(164)
    EXCEPTION(165)
    EXCEPTION(166)
    EXCEPTION(167)
    EXCEPTION(168)
    EXCEPTION(169)
    EXCEPTION(170)
    EXCEPTION(171)
    EXCEPTION(172)
    EXCEPTION(173)
    EXCEPTION(174)
    EXCEPTION(175)
    EXCEPTION(176)
    EXCEPTION(177)
    EXCEPTION(178)
    EXCEPTION(179)
    EXCEPTION(180)
    EXCEPTION(181)
    EXCEPTION(182)
    EXCEPTION(183)
    EXCEPTION(184)
    EXCEPTION(185)
    EXCEPTION(186)
    EXCEPTION(187)
    EXCEPTION(188)
    EXCEPTION(189)
    EXCEPTION(190)
    EXCEPTION(191)
    EXCEPTION(192)
    EXCEPTION(193)
    EXCEPTION(194)
    EXCEPTION(195)
    EXCEPTION(196)
    EXCEPTION(197)
    EXCEPTION(198)
    EXCEPTION(199)
    EXCEPTION(200)
    EXCEPTION(201)
    EXCEPTION(202)
    EXCEPTION(203)
    EXCEPTION(204)
    EXCEPTION(205)
    EXCEPTION(206)
    EXCEPTION(207)
    EXCEPTION(208)
    EXCEPTION(209)
    EXCEPTION(210)
    EXCEPTION(211)
    EXCEPTION(212)
    EXCEPTION(213)
    EXCEPTION(214)
    EXCEPTION(215)
    EXCEPTION(216)
    EXCEPTION(217)
    EXCEPTION(218)
    EXCEPTION(219)
    EXCEPTION(220)
    EXCEPTION(221)
    EXCEPTION(222)
    EXCEPTION(223)
    EXCEPTION(224)
    EXCEPTION(225)
    EXCEPTION(226)
    EXCEPTION(227)
    EXCEPTION(228)
    EXCEPTION(229)
    EXCEPTION(230)
    EXCEPTION(231)
    EXCEPTION(232)
    EXCEPTION(233)
    EXCEPTION(234)
    EXCEPTION(235)
    EXCEPTION(236)
    EXCEPTION(237)
    EXCEPTION(238)
    EXCEPTION(239)
    EXCEPTION(240)
    EXCEPTION(241)
    EXCEPTION(242)
    EXCEPTION(243)
    EXCEPTION(244)
    EXCEPTION(245)
    EXCEPTION(246)
    EXCEPTION(247)
    EXCEPTION(248)
    EXCEPTION(249)
    EXCEPTION(250)
    EXCEPTION(251)
    EXCEPTION(252)
    EXCEPTION(253)
    EXCEPTION(254)
    EXCEPTION(255)
}

static void settr(register u16 sel)
{
    __asm__ __volatile__ (
        "ltr    %0\n"
        :: "a" (sel));
}

Core::
Core(Sched* sched, void* stack, unsigned stackSize, Tss* tss) :
    sched(sched),
    currentProc(0),
    current(0),
    currentFPU(0),
    yieldable(true),
    label(stack, stackSize, reschedule, this),
    tss(tss),
    gdtLoc(sizeof gdt - 1, gdt),
    tcb(0)
{
    ASSERT((unsigned long) this % 16 == 0);
    ASSERT((unsigned long) tss % 256 == 0);

    id = numCores.addRef() - 1;
    ASSERT(id < MaxCore);
    coreTable[id] = this;

    // Reserve a page for TCB that can be read from the user level.
    tcb = (Tcb*) Mmu::getPointer(0x11000 + (id * sizeof(Tcb)));
    memset(tcb, 0, sizeof(Tcb));

    ktcb = (Tcb*) Mmu::getPointer(0x12000 + (id * sizeof(Tcb)));
    memset(ktcb, 0, sizeof(Tcb));

    memset(tss, 0, sizeof(Tss));
    tss->ss0 = tss->ds = tss->es = tss->fs = tss->ss = KDATASEL;
    tss->gs = KTCBSEL;
    tss->cs = KCODESEL;
    tss->iomap = sizeof(Tss);
    tss->sp0 = 0;   // kernel stack pointer

    gdt[0].init(0, 0, 0);

    // KCODESEL
    gdt[1].init(0, 0xfffff, Segdesc::SEGG | Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGEXEC | Segdesc::SEGR);

    // KDATASEL
    gdt[2].init(0, 0xfffff, Segdesc::SEGG | Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGDATA | Segdesc::SEGW);

    // RCODESEL
    gdt[3].init(0x30000, 0xffff, Segdesc::SEGP | Segdesc::SEGEXEC | Segdesc::SEGR);

    // RDATASEL
    gdt[4].init(0x30000, 0xffff, Segdesc::SEGP | Segdesc::SEGDATA | Segdesc::SEGW);

    // KTCBSEL
    gdt[5].init(0x80000000 + 0x12000 + id * sizeof(Tcb),
                sizeof(tcb) - 1, Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGDATA);

    // TSSSEL
    gdt[6].init((u32) tss, sizeof(Tss) - 1, Segdesc::SEGP | Segdesc::SEGTSS);

    // UCODESEL
    gdt[7].init(0, 0x7ffff, Segdesc::SEGG | Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGEXEC | Segdesc::SEGR, 3);

    // UDATASEL
    gdt[8].init(0, 0x7ffff, Segdesc::SEGG | Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGDATA | Segdesc::SEGW, 3);

    // TCBSEL
    gdt[9].init(0x80000000 - 8192 + id * sizeof(Tcb),
                sizeof(tcb) - 1, Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGDATA, 3);

    if (id == 0)
    {
        u32 exceptionAddr = (u32) exceptionTable;
        for (Segdesc* desc = idt; desc < &idt[256]; ++desc, exceptionAddr += 16)
        {
            desc->setExceptionHandler(KCODESEL, (void (*)()) exceptionAddr);
        }
    }

    idt[65].setDPL(3);  // for system call
    idt[66].setDPL(3);  // for upcall

    gdtLoc.loadGDT();
    idtLoc.loadIDT();
    settr(TSSSEL);

    // Load KTCBSEL to %gs
    __asm__ __volatile__ (
        "movw   $40, %%ax\n"
        "movw   %%ax, %%gs"
        ::: "%eax");

    int eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    esReport("cpuid(1): %08x %08x\n", ecx, edx);
    fxsr = (edx & (1u << 24)) ? true : false;
    sse = (edx & (1u << 25)) ? true : false;
    initFPU();

    if (!pic)
    {
        pic = new Pic();
    }
}

Core* Core::
getCurrentCore()
{
    return coreTable[0];
}

void Core::
reschedule(void* param)
{
    unsigned x = Thread::splHi();
    Core* core = getCurrentCore();
    for (;;)
    {
        core->label.set();
        Thread* current = (Thread*) core->current;
        if (current)
        {
            current->lock();
            switch (current->getState())
            {
              case IThread::TERMINATED:
                if (core->currentFPU == current)
                {
                    disableFPU();
                }
                current->unlock();
                current->release();
                break;
              case IThread::RUNNING:
                current->state = IThread::RUNNABLE;
                current->setRun();
                // FALL THROUGH
              default:
                if (core->currentFPU == current)
                {
                    current->xreg.save();
                    disableFPU();
                }
                current->unlock();
                break;
            }
            core->current = 0;
        }
        core->current = current = core->sched->selectThread();
        core->currentFPU = 0;
        core->ktcb->tcb = current->ktcb;

        UpcallRecord* record = current->upcallList.getLast();
        if (!record)
        {
            if (current->process)
            {
                core->tcb->tcb = current->tcb;
                current->process->load();
            }
        }
        else
        {
            // The current thread is upcalling a server process.
            core->tcb->tcb = record->tcb;
            record->process->load();
        }
        core->tss->sp0 = current->sp0;

#ifdef VERBOSE
        esReport("reschedule: %d %p %p\n", core->id, current, current->label.eip);
#endif

        current->label.jump();
    }
    Thread::splX(x);
    // NOT REACHED HERE
}

void Core::
initFPU()
{
    __asm__ __volatile__ (
        "movl   %%cr0, %%eax\n"
        "andl   $~0xc, %%eax\n" /* em=0, ts=0 */
        "movl   %%eax, %%cr0\n"
        "finit\n"
        "wait\n"
        "movl   %%cr0, %%eax\n"
        "orl    $8, %%eax\n"
        "movl   %%eax, %%cr0"
        ::: "%eax");
}

void Core::
enableFPU()
{
    // Disable #NM exception
    __asm__ __volatile__ (
        "clts\n");
}

void Core::
disableFPU()
{
    // Enable #NM exception
    __asm__ __volatile__ (
        "fwait\n"
        "movl   %%cr0, %%eax\n"
        "orl    $8, %%eax\n"
        "movl   %%eax, %%cr0"
        ::: "%eax");
}

void Core::
cpuid(int op, int* eax, int* ebx, int* ecx, int* edx)
{
    __asm__("cpuid"
        : "=a" (*eax),
          "=b" (*ebx),
          "=c" (*ecx),
          "=d" (*edx)
        : "a" (op));
}

void Core::
dispatchException(Ureg* ureg)
{
    Core* core = getCurrentCore();
    switch (ureg->trap)
    {
      case NO_NM:   // Device Not Available (No Math Coprocessor)
        if (core->current)
        {
            core->currentFPU = core->current;
            enableFPU();
            core->current->xreg.restore();
        }
        break;
      case NO_PF:
#ifdef __i386__
        register u32 cr2;
        __asm__ __volatile__ (
            "movl   %%cr2, %0\n"
            : "=r"(cr2));
#endif // __i386__
        if (core->currentProc)
        {
            int rc;
            if (ureg->error & Page::PTEVALID)
            {
                rc = core->currentProc->protectionFault((void*) cr2, ureg->error);
            }
            else
            {
                rc = core->currentProc->validityFault((void*) cr2, ureg->error);
            }
            if (0 <= rc)
            {
                break;
            }
        }
        ureg->dump();
        esPanic(__FILE__, __LINE__, "Failed Core::dispatchException: %u @ %x cr2:%x\n", ureg->trap, ureg->eip, cr2);
        break;
      case 65:  // system call interface
        // ureg->eax: self
        // ureg->ecx: array of parameters
        // ureg->edx: method number
        // ureg->esi: base
        if (core->currentProc)
        {
            // esReport("[%p]::dispatchException: %u @ %x\n", core->currentProc, ureg->trap, ureg->eip);
            int errorCode(0);
            long long result;

            core->current->param = ureg;
            try
            {
                va_list param(reinterpret_cast<va_list>(ureg->ecx));

                ureg->ecx = 0;  // To indicate this is not an exception call
                result = core->currentProc->systemCall(
                    reinterpret_cast<void**>(ureg->eax),
                    ureg->edx,
                    param,
                    reinterpret_cast<void**>(ureg->esi));
            }
            catch (Exception& error)
            {
                errorCode = error.getResult();
            }
            catch (std::bad_alloc)
            {
                errorCode = ENOMEM;
            }
            catch (...)
            {
                // Unexpected exception
                errorCode = EINVAL;
            }

            if (errorCode)
            {
                esReport("[SystemException: %d]\n", errorCode);
            }

            ureg->eax = (u32) result;
            ureg->ecx = (u32) errorCode;
            ureg->edx = (u32) (result >> 32);
            core->current->testCancel();
            break;
        }
        esPanic(__FILE__, __LINE__, "Failed Core::dispatchException: %u @ %x\n", ureg->trap, ureg->eip);
        break;
      case 66:  // upcall interface
        if (core->currentProc)
        {
            // esReport("[%p]::dispatchException: %u @ %x\n", core->currentProc, ureg->trap, ureg->eip);
            ureg->ecx = 0;  // To indicate this is not an exception call
            core->current->param = ureg;
            core->currentProc->returnFromUpcall(ureg);
            core->current->testCancel();
            break;
        }
        esPanic(__FILE__, __LINE__, "Failed Core::dispatchException: %u @ %x\n", ureg->trap, ureg->eip);
        break;
      default:
        ICallback* callback = core->exceptionHandlers[ureg->trap];
        if (32 <= ureg->trap)
        {
            core->yieldable = false;
            int irq = ureg->trap - 32;
            if (pic->ack(irq))
            {
                if (callback)
                {
                    callback->invoke(irq);
                }
                pic->end(irq);
            }
            core->yieldable = true;
            Thread::reschedule();
        }
        else
        {
            ureg->dump();
            esPanic(__FILE__, __LINE__, "Core::dispatchException: %u @ %x\n", ureg->trap, ureg->eip);
        }
        break;
    }
}

long Core::
registerExceptionHandler(u8 exceptionNumber, ICallback* callback)
{
    if (!callback)
    {
        return -1;
    }
    unsigned x = Thread::splHi();
    spinLock.lock();
    ICallback* old = exceptionHandlers[exceptionNumber];
    exceptionHandlers[exceptionNumber] = callback;
    if (32 <= exceptionNumber && !old)
    {
        pic->startup(exceptionNumber - 32);
    }
    spinLock.unlock();
    Thread::splX(x);
    callback->addRef();
    if (old)
    {
        old->release();
    }
    return 0;
}

long Core::
unregisterExceptionHandler(u8 exceptionNumber, ICallback* callback)
{
    long rc;
    unsigned x = Thread::splHi();
    spinLock.lock();
    ICallback* old = exceptionHandlers[exceptionNumber];
    if (!callback || old == callback)
    {
        exceptionHandlers[exceptionNumber] = 0;
        rc = 0;
    }
    else
    {
        rc = -1;
    }
    if (32 <= exceptionNumber && old)
    {
        pic->shutdown(exceptionNumber - 32);
    }
    spinLock.unlock();
    Thread::splX(x);
    if (old)
    {
        old->release();
    }
    return rc;
}

void Core::
shutdown()
{
    unsigned x = Thread::splHi();
    // Restore default kernel memory map
    u32* table = static_cast<u32*>(Mmu::getPointer(0x10000));
    for (int i = 0; i < 512; ++i)
    {
        table[i] = (i * 0x400000) | 0x83;
    }
    __asm__ __volatile__ (
        "movl   $0x10000, %%eax\n"
        "movl   %%eax, %%cr3\n"
        ::: "%eax");

    ((int (*)()) (0x30000 + (*(u16*) (0x30000 + 124))))();
    Thread::splX(x);
}
