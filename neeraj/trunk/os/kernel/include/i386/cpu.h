/*
 * Copyright 2008 Google Inc.
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

#ifndef NINTENDO_ES_KERNEL_I386_CPU_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_CPU_H_INCLUDED

#include <es/types.h>

// Exception vector No.
#define NO_DE       0                   // Divide Error
#define NO_DB       1                   // Debug
#define NO_NMI      2                   // NMI Interrupt
#define NO_BP       3                   // Breakpoint
#define NO_OF       4                   // Overflow
#define NO_BR       5                   // BOUND Range Exceeded
#define NO_UD       6                   // Invalid Opcode (UnDefined Opcode)
#define NO_NM       7                   // Device Not Available (No Math Coprocessor)
#define NO_DF       8                   // Double Fault
#define NO_MF386    9                   // CoProcessor Segment Overrun
#define NO_TS       10                  // Invalid TSS
#define NO_NP       11                  // Segment Not Present
#define NO_SS       12                  // Stack Segment Fault
#define NO_GP       13                  // General Protection
#define NO_PF       14                  // Page Fault
#define NO_MF       16                  // Floating-Point Error (Math Fault)
#define NO_AC       17                  // Alignment Check
#define NO_MC       18                  // Machine Check
#define NO_XF       19                  // SIMD Floating-Point Exception

// Segment descriptor/gate
struct Segdesc
{
    // fields in segment descriptors
    static const u32 SEGG = 1<<23;      // granularity (1=4K)
    static const u32 SEGD = 1<<22;      // 32-bit code segment
    static const u32 SEGB = 1<<22;      // 32-bit stack pointer register
    static const u32 SEGP = 1<<15;      // segment present
    static const u32 SEGS = 1<<12;      // descriptor type (0 = system; 1 = code or data)
    static const u32 SEGE = 1<<10;      // expand down
    static const u32 SEGW = 1<<9;       // writable (for data/stack)
    static const u32 SEGR = 1<<9;       // readable (for code)
    static const u32 SEGBUSY = 1<<9;    // busy (for task)
    static const u32 SEGA = 1<<8;       // accessed
    static const u32 SEGDATA = 0x10<<8; // data/stack segment
    static const u32 SEGEXEC = 0x18<<8; // executable segment
    static const u32 SEGLDT = 0x02<<8;  // LDT
    static const u32 SEGTASK = 0x05<<8; // task gate
    static const u32 SEGTSS = 0x09<<8;  // 32-bit TSS segment
    static const u32 SEGCG = 0x0c<<8;   // 32-bit call gate
    static const u32 SEGIG = 0x0e<<8;   // 32-bit interrupt gate
    static const u32 SEGTG = 0x0f<<8;   // 32-bit trap gate

    u32 d0;
    u32 d1;

    void init(u32 base, u32 limit, u32 fields, u8 dpl = 0)
    {
        ASSERT(dpl < 4);

        d0 = base << 16;
        d1 = (base & 0xff000000L) | ((base >> 16) & 0xff);

        d0 |= limit & 0xffff;
        d1 |= limit & 0xf0000;

        d1 |= fields | (dpl << 13);
    }

    void setInterruptHandler(u16 sel, u32 exceptionAddress)
    {
        d0 = (sel << 16) | (exceptionAddress & 0xffff);
        d1 = (exceptionAddress & 0xffff0000) | SEGP | SEGIG;
    }

    void setTrapHandler(u16 sel, u32 exceptionAddress)
    {
        d0 = (sel << 16) | (exceptionAddress & 0xffff);
        d1 = (exceptionAddress & 0xffff0000) | SEGP | SEGTG;
    }

    void setTaskGate(u16 sel)
    {
        d0 = (sel << 16);
        d1 = SEGP | SEGTASK;
    }

    void setDPL(u8 dpl)
    {
        ASSERT(dpl < 4);
        d1 &= ~(3 << 13);
        d1 |= dpl << 13;
    }
};

// Descriptor table location for lgdt, lidt
struct SegdescLoc
{
    u16         alignment __attribute__ ((packed));
    u16         size __attribute__ ((packed));
    Segdesc*    base __attribute__ ((packed));

    SegdescLoc(u16 size, Segdesc* base) :
        alignment(0),
        size(size),
        base(base)
    {
    }

    void loadGDT()
    {
        __asm__ __volatile__ ("lgdt %0" : : "m" (size));
    }

    void loadIDT()
    {
        __asm__ __volatile__ ("lidt %0" : : "m" (size));
    }

} __attribute__ ((aligned (4)));;

// Task state segment
struct Tss
{
    u32 backlink;
    u32 sp0;
    u32 ss0;
    u32 sp1;
    u32 ss1;
    u32 sp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;     // segment selectors
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldt;    // local descriptor table
    u32 iomap;  // io map base

    void dump()
    {
        esReport("edi %08x  esi %08x  ebp %08x  esp %08x\n",
                 edi, esi, ebp, esp);
        esReport("ebx %08x  edx %08x  ecx %08x  eax %08x\n",
                 ebx, edx, ecx, eax);
        esReport("ds %04x  es %04x  fs %04x  gs %04x  ss %04x\n",
                 (u16) ds, (u16) es, (u16) fs, (u16) gs, (u16) ss);
        esReport("trap %d  error %08x  eip %08x  cs %04x\n",
                 NO_DF, 0, eip, (u16) cs);
        esReport("eflags %08x\n",
                 eflags);
    }
};

// Layout of fsave and frstor memory region
struct Freg
{
    u16 fcw;        // control word
    u16 r1;
    u16 fsw;        // status word
    u16 r2;
    u16 ftw;        // tag word
    u16 r3;
    u32 ip;         // instruction pointer offset
    u16 cs;         // instruction pointer selector
    u16 fop;        // last instruction opcode
    u32 dp;         // operand pointer offset
    u16 ds;         // operand pointer selector
    u16 r5;
    u8  st[80];
};

// Layout of fxsave and fxrstor memory region (16-byte aligned)
struct Fxreg
{
    u16 fcw;        // control word
    u16 fsw;        // status word
    u16 ftw;        // tag word
    u16 fop;        // last instruction opcode
    u32 ip;         // instruction pointer offset
    u16 cs;         // instruction pointer selector
    u16 r1;
    u32 dp;         // operand pointer offset
    u16 ds;         // operand pointer selector
    u16 r2;
    u32 mxcsr;
    u32 mxcsr_mask;
    u8  st[128];
    u8  xmm[128];
    u8  r3[224];
} __attribute__ ((aligned (16)));

struct Xreg
{
    union
    {
        Freg    freg;
        Fxreg   fxreg;
    };
    Xreg();
    void save();
    void restore();
    void dump();
} __attribute__ ((aligned (16)));

struct Ureg
{
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 nsp;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;
    u32 trap;   // trap type
    u32 error;  // error code (or zero)
    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp;
    u32 ss;

    void load()
    {
         __asm__ __volatile__ (
            "cli\n"
            "movl   %0, %%esp\n"
            "popal\n"
            "popl   %%gs\n"
            "popl   %%fs\n"
            "popl   %%es\n"
            "popl   %%ds\n"
            "addl   $8,%%esp\n"
            "iretl"
            :: "r" (this) : "%esp");
    }

    void dump()
    {
        esReport("edi %08x  esi %08x  ebp %08x  esp %08x\n",
                 edi, esi, ebp, nsp);
        esReport("ebx %08x  edx %08x  ecx %08x  eax %08x\n",
                 ebx, edx, ecx, eax);
        esReport("ds %04x  es %04x  fs %04x  gs %04x\n",
                 (u16) ds, (u16) es, (u16) fs, (u16) gs);
        esReport("trap %d  error %08x  eip %08x  cs %04x\n",
                 trap, error, eip, (u16) cs);
        esReport("eflags %08x  esp %08x  ss %04x\n",
                 eflags, esp, (u16) ss);
    }

    struct Frame
    {
        Frame* prev;
        void*  pc;
    };

    void where()
    {
        Frame* frame = (Frame*) ebp;
        while (frame)
        {
            esReport("%p %p\n", frame->pc, frame->prev);
            frame = frame->prev;
        }
    }
};

#define rdmsr(msr, val1, val2)      \
    __asm__ __volatile__(           \
        "rdmsr"                     \
        : "=a" (val1), "=d" (val2)  \
        : "c" (msr))

#define wrmsr(msr, val1, val2)      \
    __asm__ __volatile__(           \
        "wrmsr"                     \
        :: "c" (msr), "a" (val1), "d" (val2))

#define rdpmc(counter, low, high)   \
     __asm__ __volatile__(          \
        "rdpmc"                     \
        : "=a" (low), "=d" (high)   \
        : "c" (counter))


#define IA32_APIC_BASE      0x1b    // APIC Location and Status

#define IA32_PERFEVTSEL0    0x186
#define IA32_PERFEVTSEL1    0x187
#define IA32_PMC0           0xc1
#define IA32_PMC1           0xc2

#endif  // NINTENDO_ES_KERNEL_I386_CPU_H_INCLUDED
