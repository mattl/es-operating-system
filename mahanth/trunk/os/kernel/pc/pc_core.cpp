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

#include <stdexcept>
#include <errno.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include "apic.h"
#include "core.h"
#include "8254.h"

// #define VERBOSE

extern void debug(Ureg* ureg);

extern bool nmiHandler();

namespace
{
    // The NullPic class is only used until the Pic class object for 8259 or
    // the Apic class object for APIC are constructed.
    class NullPic : public es::Pic
    {
        Ref ref;
    public:
        int startup(unsigned int bus, unsigned int irq)
        {
            return -1;
        }
        int shutdown(unsigned int bus, unsigned int irq)
        {
            return -1;
        }
        int enable(unsigned int bus, unsigned int irq)
        {
            return -1;
        }
        int disable(unsigned int bus, unsigned int irq)
        {
            return -1;
        }
        bool ack(int vec)
        {
            return false;
        }
        bool end(int vec)
        {
            return false;
        }
        int setAffinity(unsigned int bus, unsigned int irq, unsigned int mask)
        {
            return -1;
        }
        unsigned int splIdle() { return 0; }
        unsigned int splLo() { return 0; }
        unsigned int splHi() { return 0; }
        void splX(unsigned int x) {}
        Object* queryInterface(const char* riid)
        {
            Object* objectPtr;
            if (strcmp(riid, es::Pic::iid()) == 0)
            {
                objectPtr = static_cast<es::Pic*>(this);
            }
            else if (strcmp(riid, Object::iid()) == 0)
            {
                objectPtr = static_cast<es::Pic*>(this);
            }
            else
            {
                return NULL;
            }
            objectPtr->addRef();
            return objectPtr;
        }
        unsigned int addRef()
        {
            return ref.addRef();
        }
        unsigned int release()
        {
            unsigned int count = ref.release();
            if (count == 0)
            {
                delete this;
                return 0;
            }
            return count;
        }
    };

    NullPic nullPic __attribute__ ((init_priority (101)));

    struct Frame
    {
        Frame* prev;
        void*  pc;
    };
}

Core* Core::coreTable[Core::CORE_MAX];
bool Core::fxsr;
bool Core::sse;
Segdesc Core::idt[256] __attribute__ ((aligned (16)));
SegdescLoc Core::idtLoc(sizeof idt - 1, idt);
Lock Core::spinLock;
es::Callback* Core::exceptionHandlers[255];
es::Pic* Core::pic = &nullPic;
u8 Core::isaBus;

extern "C"
{
    asm("   .text");
    asm("   .align  16, 0");
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
    asm("   pushl   %eax");     // gap for kernel page fault handling
    asm("   pushl   56(%eax)");
    asm("   pushl   8(%eax)");
    asm("   movl    %esp, %ebp");
    asm("   pushl   %eax");

    asm("   call    _ZN4Core17dispatchExceptionEP4Ureg");

    asm("   addl    $16,%esp");
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

void Core::
doubleFault()
{
    Core* core = getCurrentCore();
    Tss* tss = core->tss1;

    esReport("Kernel panic [%d:%p]\n", core->id, core->currentProc);
    tss->dump();
    for (;;)
    {
#ifdef __i386__
        __asm__ __volatile__ ("hlt\n");
#endif
    }
}

Core::
Core(Sched* sched) :
    sched(sched),
    currentProc(0),
    current(0),
    currentFPU(0),
    freeze(0),
    gdtLoc(sizeof gdt - 1, gdt),
    tcb(0)
{
    ASSERT(reinterpret_cast<unsigned long>(this) % 16 == 0);

    u8* ptr = reinterpret_cast<u8*>((reinterpret_cast<unsigned long>(this) & ~0xff) - 256);

    tss0 = reinterpret_cast<Tss*>(ptr);
    ASSERT(reinterpret_cast<unsigned long>(tss0) % 128 == 0);
    tss1 = reinterpret_cast<Tss*>(ptr + 128);
    ASSERT(reinterpret_cast<unsigned long>(tss1) % 128 == 0);

    stack = ptr + 256 + ((sizeof(Core) + 15) & ~15);
    *(int*) stack = 0xa5a5a5a5;
    unsigned stackSize = CORE_SIZE - 256 - ((sizeof(Core) + 15) & ~15);
    label.init(stack, stackSize, reschedule, this);

    id = Sched::numCores - 1;
    coreTable[id] = this;

    esReport("Core %d at %p:%p:%p %p:%d\n", id, this, ptr, tss0, stack, stackSize);

    // Reserve a page for TCBs that can be read from the user level.
    tcb = reinterpret_cast<Tcb*>(0x80011000 + id * sizeof(Tcb));
    memset(tcb, 0, sizeof(Tcb));

    memset(tss0, 0, sizeof(Tss));
    tss0->ss0 = tss0->ds = tss0->es = tss0->fs = tss0->ss = KDATASEL;
    tss0->gs = KTCBSEL;
    tss0->cs = KCODESEL;
    tss0->iomap = sizeof(Tss);
    tss0->sp0 = 0;   // kernel stack pointer

    memset(tss1, 0, sizeof(Tss));
    tss1->ss0 = tss1->ds = tss1->es = tss1->fs = tss1->ss = KDATASEL;
    tss1->gs = KTCBSEL;
    tss1->cs = KCODESEL;
    tss1->iomap = sizeof(Tss);
    tss1->sp0 = 0x80014000;  // kernel stack pointer
    tss1->eip = (u32) doubleFault;
    tss1->esp = 0x80014000;
    tss1->eflags = 0x02;
    tss1->cr3 = 0x10000;

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
    gdt[5].init(reinterpret_cast<u32>(&ktcb),
                sizeof(tcb) - 1, Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGDATA);

    // TSSSEL
    gdt[6].init((u32) tss0, sizeof(Tss) - 1, Segdesc::SEGP | Segdesc::SEGTSS);

    // UCODESEL
    gdt[7].init(0, 0x7ffff, Segdesc::SEGG | Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGEXEC | Segdesc::SEGR, 3);

    // UDATASEL
    gdt[8].init(0, 0x7ffff, Segdesc::SEGG | Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGDATA | Segdesc::SEGW, 3);

    // TCBSEL
    gdt[9].init(0x80000000 - 8192 + id * sizeof(Tcb),
                sizeof(tcb) - 1, Segdesc::SEGD | Segdesc::SEGP | Segdesc::SEGDATA, 3);

    // TSS1SEL
    gdt[10].init((u32) tss1, sizeof(Tss) - 1, Segdesc::SEGP | Segdesc::SEGTSS);

    if (id == 0)
    {
        u32 exceptionAddr = reinterpret_cast<u32>(exceptionTable);
        for (int vec = 0; vec < 256; ++vec, exceptionAddr += 16)
        {
            Segdesc* desc = &idt[vec];
            switch (vec)
            {
            case NO_DE:     // Divide Error
            case NO_DB:     // Debug
            case NO_NMI:    // NMI Interrupt
            case NO_BP:     // Breakpoint
            case NO_OF:     // Overflow
            case NO_BR:     // BOUND Range Exceeded
            case NO_UD:     // Invalid Opcode (UnDefined Opcode)
            case NO_NM:     // Device Not Available (No Math Coprocessor)
            case NO_MF386:  // CoProcessor Segment Overrun
            case NO_TS:     // Invalid TSS
            case NO_NP:     // Segment Not Present
            case NO_SS:     // Stack Segment Fault
            case NO_GP:     // General Protection
            case NO_PF:     // Page Fault
            case NO_MF:     // Floating-Point Error (Math Fault)
            case NO_AC:     // Alignment Check
            case NO_MC:     // Machine Check
            case NO_XF:     // SIMD Floating-Point Exception
                desc->setInterruptHandler(KCODESEL, exceptionAddr);
                break;
            case NO_DF:     // Double Fault
                desc->setTaskGate(TSS1SEL);
                break;
            case 65:        // System call
            case 66:        // Upcall
                desc->setInterruptHandler(KCODESEL, exceptionAddr);
                desc->setDPL(3);
                break;
            default:
                desc->setInterruptHandler(KCODESEL, exceptionAddr);
                break;
            }
        }
    }

    gdtLoc.loadGDT();
    idtLoc.loadIDT();
    settr(TSSSEL);

    // Load KTCBSEL to %gs
    __asm__ __volatile__ (
        "movw   $40, %%ax\n"
        "movw   %%ax, %%gs"
        ::: "%eax");

    int eax, ebx, ecx, edx;
    cpuid(0x01, &eax, &ebx, &ecx, &edx);
    esReport("cpuid(1): %08x %08x\n", ecx, edx);
    fxsr = (edx & (1u << 24)) ? true : false;
    sse = (edx & (1u << 25)) ? true : false;
    initFPU();
}

Core* Core::
getCurrentCore()
{
    return coreTable[Apic::getLocalApicID()];   // XXX The Local APIC IDs need not to be consecutive.
}

void Core::
reschedule(void* param)
{
    unsigned x = splHi();
    Apic::started();
    Core* core = getCurrentCore();
    for (;;)
    {
        core->label.set();
        Thread* current = core->current;
        if (current)
        {
            // Note the current has been locked when rescheduled by
            // Rendezvous::sleep().
            current->tryLock();
            core->current = 0;
#ifdef VERBOSE
            esReport("[%d:%p] Core::reschedule in. current = %p %p %p\n",
                     core->id, &param, current, current ? current->label.esp : 0);
#endif
            ASSERT(current->core == core);
            switch (current->getState())
            {
              case es::Thread::TERMINATED:
                if (core->currentFPU == current)
                {
                    disableFPU();
                }
                current->unlock();
                current->release();
                break;
              case es::Thread::RUNNING:
                current->setRun();
                // FALL THROUGH
              default:
                if (core->currentFPU == current)
                {
                    current->xreg.save();
                    disableFPU();
                }
                ASSERT(current->label.isSane());
                current->core = 0;
                current->unlock();
                break;
            }
        }
        core->current = current = core->sched->selectThread();
        ASSERT(core->checkStack());
        core->currentFPU = 0;
        core->ktcb.tcb = current->ktcb;

        UpcallRecord* record = current->upcallList.getLast();
        if (!record)
        {
            if (current->process)
            {
                core->tcb->tcb = current->tcb;
                current->process->load();
            }
            else
            {
                // To make kernel threads be able to call kill(), etc. safely.
                core->currentProc = 0;
            }
        }
        else
        {
            // The current thread is upcalling a server process.
            core->tcb->tcb = record->tcb;
            ASSERT(record->process);
            record->process->load();
        }
        core->tss0->sp0 = current->sp0;

#ifdef VERBOSE
        esReport("[%d:%p] Core::reschedule out. current = %p: %p %p\n",
                 core->id, &param, current, current ? current->label.esp : 0);
#endif

        ASSERT(current->label.isSane());
        current->label.jump();
    }
    splX(x);
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

static void kernelFault()
{
    throw SystemException<EFAULT>();
}

void Core::
dispatchException(Ureg* ureg)
{
    unsigned x = splHi();
    Core* core = getCurrentCore();
    Thread* current = core->current;
    Process* process = core->currentProc;

#ifdef VERBOSE
    esReport("[%d:%p]: dispatchException(%p:%d) at %p\n",
              core->id, (u8*) &ureg + sizeof(Ureg) - 8,
              ureg, ureg->trap, ureg->eip);
#endif

    switch (ureg->trap)
    {
      case NO_DB:   // Debug
      case NO_BP:   // Breakpoint
        debug(ureg);
        break;
      case NO_NM:   // Device Not Available (No Math Coprocessor)
        if (current)
        {
            core->currentFPU = current;
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
        if (process)
        {
            // splLo();

            int rc;
            if (ureg->error & Page::PTEVALID)
            {
                rc = process->protectionFault((void*) cr2, ureg->error);
            }
            else
            {
                rc = process->validityFault((void*) cr2, ureg->error);
            }
            if (0 <= rc)
            {
                break;
            }
            if (ureg->cs == KCODESEL && process->isValid((void*) cr2, 1))
            {
                // Kernel page fault: Create a stack frame as if kernelFault() is
                // called. By using the -fnon-call-exceptions g++ compiler option,
                // the function incurred a kernel page fault will receive an
                // EFFALT system exception.
                if (process->log)
                {
                    esReport("[kernel page fault at %p]\n", cr2);
                    Frame* frame = (Frame*) (&ureg - 2);
                    while (frame)
                    {
                        esReport("%p %p\n", frame->pc, frame->prev);
                        frame = frame->prev;
                    }
                }
                Ureg* exc = (Ureg*) ((char*) ureg - sizeof(u32));
                memmove(exc, ureg, sizeof(Ureg) - 8);
                u32 ret = exc->eip;
                exc->esp = ret;     // Note &exc->esp is the new esp address after iret.
                exc->eip = (u32) kernelFault;

                ASSERT(!current || current->checkStack());

                splX(x);
                exc->load();
                break;
            }
        }
        esReport("Kernel panic [%d:%p]\n", core->id, core->currentProc);
        ureg->dump();
        if (process)
        {
            process->dump();
        }
        esPanic(__FILE__, __LINE__, "Failed Core::dispatchException: %u @ %x cr2:%x", ureg->trap, ureg->eip, cr2);
        break;
      case 65:  // system call interface
        // ureg->eax: self
        // ureg->ecx: array of parameters
        // ureg->edx: method number
        // ureg->esi: base
        if (process)
        {
            splLo();

            // esReport("[%p]::dispatchException: %u @ %x\n", process, ureg->trap, ureg->eip);
            int errorCode(0);
            long long result;

            current->param = ureg;
            try
            {
                va_list param(reinterpret_cast<va_list>(ureg->ecx));

                ureg->ecx = 0;  // To indicate this is not an exception call
                result = process->systemCall(
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
            current->testCancel();
            break;
        }
        esPanic(__FILE__, __LINE__, "Failed Core::dispatchException: %u @ %x", ureg->trap, ureg->eip);
        break;
      case 66:  // upcall interface
        if (process)
        {
            splLo();

            ureg->ecx = 0;  // To indicate this is not an exception call
            current->param = ureg;
            process->returnFromUpcall(ureg);
            current->testCancel();
            break;
        }
        esPanic(__FILE__, __LINE__, "Failed Core::dispatchException: %u @ %x", ureg->trap, ureg->eip);
        break;
      case NO_NMI:
        if (nmiHandler())
        {
            static long long nmiTick;
            if (Pit::tick != nmiTick)
            {
                nmiTick = Pit::tick;
                break;
            }
        }
        // FALL THROUGH
      default:
        if (32 <= ureg->trap)
        {
            core->freeze.increment();
            core->current = 0;
            if (pic->ack(ureg->trap))
            {
                // x = splLo();     // Enable interrupts
                es::Callback* callback = core->exceptionHandlers[ureg->trap];
                if (callback)
                {
                    callback->invoke(ureg->trap);
                }
                pic->end(ureg->trap);
                // splX(x);
            }

            core->current = current;
            ASSERT(!current || current->checkStack());
            core->freeze.decrement();
            Thread::reschedule();
        }
        else
        {
            esReport("Kernel panic [%d:%p]\n", core->id, core->currentProc);
            ureg->dump();
            esPanic(__FILE__, __LINE__, "Core::dispatchException: %u @ %x", ureg->trap, ureg->eip);
        }
        break;
    }

    ASSERT(!current || current->checkStack());
    splX(x);
}

long Core::
registerExceptionHandler(u8 exceptionNumber, es::Callback* callback)
{
    Lock::Synchronized method(spinLock);

    if (!callback)
    {
        return -1;
    }
    es::Callback* old = exceptionHandlers[exceptionNumber];
    exceptionHandlers[exceptionNumber] = callback;
    callback->addRef();
    if (old)
    {
        old->release();
    }
    return 0;
}

long Core::
unregisterExceptionHandler(u8 exceptionNumber, es::Callback* callback)
{
    Lock::Synchronized method(spinLock);

    long rc;
    es::Callback* old = exceptionHandlers[exceptionNumber];
    if (!callback || old == callback)
    {
        exceptionHandlers[exceptionNumber] = 0;
        rc = 0;
    }
    else
    {
        rc = -1;
    }
    if (old)
    {
        old->release();
    }
    return rc;
}

long Core::
registerInterruptHandler(u8 bus, u8 irq, es::Callback* callback)
{
    Lock::Synchronized method(spinLock);

    if (!callback)
    {
        return -1;
    }

    int vec = pic->startup(bus, irq);
    if (vec < 0 || exceptionHandlers[vec])
    {
        return -1;
    }

    callback->addRef();
    exceptionHandlers[vec] = callback;

    return pic->enable(bus, irq);;
}

long Core::
unregisterInterruptHandler(u8 bus, u8 irq, es::Callback* callback)
{
    Lock::Synchronized method(spinLock);

    if (!callback)
    {
        return -1;
    }

    int vec = pic->disable(bus, irq);
    if (vec < 0 || !exceptionHandlers[vec])
    {
        return -1;
    }

    es::Callback* old = exceptionHandlers[vec];
    ASSERT(old == callback);
    pic->shutdown(bus, irq);
    old->release();
    exceptionHandlers[vec] = 0;

    return pic->shutdown(bus, irq);;
}

void Core::
shutdown()
{
    unsigned x = splHi();
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
    splX(x);
}

bool Core::
checkStack()
{
    return *(int*) stack == 0xa5a5a5a5;
}

// Core memory layout:
//
// [ TSS0 (128 bytes) | Tss1 (128 bytes for DF#) | Core | Stack ] in 16384 bytes
//

// Allocates memory for Core, Core stack, and Core TSS at the same time.
void* Core::
operator new(size_t size) throw(std::bad_alloc)
{
    u8* ptr = reinterpret_cast<u8*>(0x80030000);
    int n = Sched::numCores.addRef() - 1;
    ASSERT(n < CORE_MAX);
    if (CORE_MAX <= n)
    {
        throw std::bad_alloc();
    }
    ptr += CORE_SIZE * n;
    return ptr + 256;
}

void Core::
operator delete(void*) throw()
{
}
