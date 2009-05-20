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

#include <es.h>
#include "cpu.h"

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from i386-stub.c in "GDB: The GNU Project Debugger."
 *
 * http://www.gnu.org/software/gdb/gdb.html
 */

/****************************************************************************

        THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a trap #1.
 *
 *  The external function exceptionHandler() is
 *  used to attach a specific handler to a specific 386 vector number.
 *  It should use the same privilege level it runs at.  It should
 *  install it as an interrupt gate so that interrupts are masked
 *  while the handler runs.
 *
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area reserved in the int array remcomStack.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include <signal.h>
#include <stdio.h>
#include <string.h>

/************************************************************************
 *
 * external low-level support routines
 */

extern void putDebugChar(int ch);   /* write a single character      */
extern int getDebugChar();          /* read and return a single char */
extern void exceptionHandler();     /* assign an exception handler   */

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400

static char initialized;  /* boolean flag. != 0 means we've been initialized */

int     remote_debug;
/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */

static const char hexchars[]="0123456789abcdef";

/* Number of registers.  */
#define NUMREGS 16

/* Number of bytes of registers.  */
#define NUMREGBYTES (NUMREGS * 4)

enum regnames
{
    EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
    PC /* also known as eip */,
    PS /* also known as eflags */,
    CS, SS, DS, ES, FS, GS
};

/* GDB stores segment registers in 32-bit words (that's just the way
   m-i386v.h is written).  So zero the appropriate areas in registers.  */
extern "C" {
int registers[NUMREGS];

#define STACKSIZE 10000
int remcomStack[STACKSIZE/sizeof(int)] __attribute__ ((aligned (16)));
int* stackPtr = &remcomStack[STACKSIZE/sizeof(int) - 1];

}

/***************************  ASSEMBLY CODE MACROS *************************/
/*                                     */

extern "C" void return_to_prog(void);

/* Restore the program's registers (including the stack pointer, which
   means we get the right stack and don't have to worry about popping our
   return address and any stack frames and so on) and return.  */
asm(".text");
asm(".globl return_to_prog");
asm("return_to_prog:");
asm("        movw registers+44, %ss");
asm("        movl registers+16, %esp");
asm("        movl registers+4, %ecx");
asm("        movl registers+8, %edx");
asm("        movl registers+12, %ebx");
asm("        movl registers+20, %ebp");
asm("        movl registers+24, %esi");
asm("        movl registers+28, %edi");
asm("        movw registers+48, %ds");
asm("        movw registers+52, %es");
asm("        movw registers+56, %fs");
asm("        movw registers+60, %gs");
asm("        movl registers+36, %eax");
asm("        pushl %eax");  /* saved eflags */
asm("        movl registers+40, %eax");
asm("        pushl %eax");  /* saved cs */
asm("        movl registers+32, %eax");
asm("        pushl %eax");  /* saved eip */
asm("        movl registers, %eax");
/* use iret to restore pc and flags together so
   that trace flag works right.  */
asm("        iret");

#define BREAKPOINT() asm("   int $3");

/* Put the error code here just in case the user cares.  */
int gdb_i386errcode;
/* Likewise, the vector number here (since GDB only gets the signal
   number through the usual means, and that's not very specific).  */
int gdb_i386vector = -1;

void _returnFromException()
{
    return_to_prog();
}

int hex(char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);
    if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');
    if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);
    return (-1);
}

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<checksum>     */

char* getpacket(void)
{
    char* buffer = &remcomInBuffer[0];
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while (1)
    {
        /* wait around for the start character, ignore all other characters */
        while ((ch = getDebugChar()) != '$')
            ;

    retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* now, read until a # or end of buffer is found */
        while (count < BUFMAX)
        {
            ch = getDebugChar();
            if (ch == '$')
                goto retry;
            if (ch == '#')
                break;
            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }
        buffer[count] = 0;

        if (ch == '#')
        {
            ch = getDebugChar();
            xmitcsum = hex(ch) << 4;
            ch = getDebugChar();
            xmitcsum += hex(ch);

            if (checksum != xmitcsum)
            {
                if (remote_debug)
                {
                    esReport("bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
                             checksum, xmitcsum, buffer);
                }
                putDebugChar('-');  /* failed checksum */
            }
            else
            {
                putDebugChar('+');  /* successful transfer */

                // esReport("getpacket: ");
                // esDump(buffer, count);

                /* if a sequence char is present, reply the sequence ID */
                if (buffer[2] == ':')
                {
                    putDebugChar(buffer[0]);
                    putDebugChar(buffer[1]);

                    return &buffer[3];
                }
                return &buffer[0];
            }
        }
    }
}

/* send the packet in buffer.  */
void putpacket(char *buffer)
{
    unsigned char checksum;
    int count;
    char ch;

    /*  $<packet info>#<checksum>. */
    do {
        putDebugChar('$');
        checksum = 0;
        count = 0;

        while (ch = buffer[count])
        {
            putDebugChar(ch);
            checksum += ch;
            count += 1;
        }

        putDebugChar('#');
        putDebugChar(hexchars[checksum >> 4]);
        putDebugChar(hexchars[checksum % 16]);

    } while (getDebugChar() != '+');
}

void debug_error(char* format)
{
    if (remote_debug)
        esReport("%s", format);
}

/* Address of a routine to RTE to if we get a memory fault.  */
static void (*volatile mem_fault_routine)() = NULL;

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int mem_err = 0;

void set_mem_err(void)
{
    mem_err = 1;
}

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to mem_fault, they won't get restored, so there better not be any
   saved).  */
int get_char(char* addr)
{
    return *addr;
}

void set_char(char* addr, int val)
{
    *addr = val;
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, then we should set mem_err in response to
   a fault; if zero treat a fault like any other fault in the stub.  */
char* mem2hex(char* mem, char* buf, int count, int may_fault)
{
    int i;
    unsigned char ch;

    if (may_fault)
        mem_fault_routine = set_mem_err;
    for (i = 0; i < count; i++)
    {
        ch = get_char(mem++);
        if (may_fault && mem_err)
            return (buf);
        *buf++ = hexchars[ch >> 4];
        *buf++ = hexchars[ch % 16];
    }
    *buf = 0;
    if (may_fault)
        mem_fault_routine = NULL;
    return (buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char* hex2mem(char* buf, char* mem, int count, int may_fault)
{
    int i;
    unsigned char ch;

    if (may_fault)
        mem_fault_routine = set_mem_err;
    for (i = 0; i < count; i++)
    {
        ch = hex(*buf++) << 4;
        ch = ch + hex(*buf++);
        set_char(mem++, ch);
        if (may_fault && mem_err)
            return (mem);
    }
    if (may_fault)
        mem_fault_routine = NULL;
    return (mem);
}

/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
int
computeSignal(int exceptionVector)
{
    int sigval;
    switch (exceptionVector)
    {
    case NO_DE:
        sigval = SIGFPE;
        break;            /* divide by zero */
    case NO_DB:
        sigval = SIGTRAP;
        break;            /* debug exception */
    case NO_BP:
        sigval = SIGTRAP;
        break;            /* breakpoint */
    case NO_OF:
        sigval = 16;
        break;            /* into instruction (overflow) */
    case NO_BR:
        sigval = 16;
        break;            /* bound instruction */
    case NO_UD:
        sigval = SIGILL;
        break;            /* Invalid opcode */
    case NO_NM:
        sigval = SIGFPE;
        break;            /* coprocessor not available */
    case NO_DF:
        sigval = SIGEMT;
        break;            /* double fault */
    case NO_MF386:
        sigval = SIGSEGV;
        break;            /* coprocessor segment overrun */
    case NO_TS:
        sigval = SIGSEGV;
        break;            /* Invalid TSS */
    case NO_NP:
        sigval = SIGSEGV;
        break;            /* Segment not present */
    case NO_SS:
        sigval = SIGSEGV;
        break;            /* stack exception */
    case NO_GP:
        sigval = SIGSEGV;
        break;            /* general protection */
    case NO_PF:
        sigval = SIGSEGV;
        break;            /* page fault */
    case NO_MF:
        sigval = SIGEMT;
        break;            /* coprocessor error */
    default:
        sigval = SIGEMT;
        break;            /* "software generated" */
    }
    return (sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hexToInt(char** ptr, int* intValue)
{
    int numChars = 0;
    int hexValue;

    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex(**ptr);
        if (hexValue >= 0)
        {
            *intValue = (*intValue << 4) | hexValue;
            numChars++;
        }
        else
            break;

        (*ptr)++;
    }

    return (numChars);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
void handle_exception(int exceptionVector)
{
    int sigval, stepping;
    int addr, length;
    char *ptr;
    int newPC;

    gdb_i386vector = exceptionVector;

    if (remote_debug)
    {
        esReport("vector=%d, sr=0x%x, pc=0x%x\n",
                 exceptionVector, registers[PS], registers[PC]);
    }

    /* reply to host that an exception has occurred */
    sigval = computeSignal(exceptionVector);

    ptr = remcomOutBuffer;

    *ptr++ = 'T';         /* notify gdb with signo, PC, FP and SP */
    *ptr++ = hexchars[sigval >> 4];
    *ptr++ = hexchars[sigval & 0xf];

    *ptr++ = hexchars[ESP];
    *ptr++ = ':';
    ptr = mem2hex((char*) &registers[ESP], ptr, 4, 0);    /* SP */
    *ptr++ = ';';

    *ptr++ = hexchars[EBP];
    *ptr++ = ':';
    ptr = mem2hex((char*) &registers[EBP], ptr, 4, 0);    /* FP */
    *ptr++ = ';';

    *ptr++ = hexchars[PC];
    *ptr++ = ':';
    ptr = mem2hex((char*) &registers[PC], ptr, 4, 0);     /* PC */
    *ptr++ = ';';

    *ptr = '\0';

    putpacket(remcomOutBuffer);

    stepping = 0;

    while (1 == 1)
    {
        remcomOutBuffer[0] = 0;
        ptr = getpacket();

        switch (*ptr++)
        {
        case '?':
            remcomOutBuffer[0] = 'S';
            remcomOutBuffer[1] = hexchars[sigval >> 4];
            remcomOutBuffer[2] = hexchars[sigval % 16];
            remcomOutBuffer[3] = 0;
            break;
        case 'd':
            remote_debug = !(remote_debug);   /* toggle debug flag */
            break;
        case 'g':       /* return the value of the CPU registers */
            mem2hex((char*) registers, remcomOutBuffer, NUMREGBYTES, 0);
            break;
        case 'G':       /* set the value of the CPU registers - return OK */
            hex2mem(ptr, (char*) registers, NUMREGBYTES, 0);
            strcpy(remcomOutBuffer, "OK");
            break;
        case 'P':       /* set the value of a single CPU register - return OK */
        {
            int regno = -1;

            if (hexToInt(&ptr, &regno) && *ptr++ == '=')
                if (regno >= 0 && regno < NUMREGS)
                {
                    hex2mem(ptr, (char*) &registers[regno], 4, 0);
                    strcpy(remcomOutBuffer, "OK");
                    break;
                }

            if (regno == 41)
            {
                // Ignore orig_eax
                strcpy(remcomOutBuffer, "OK");
            }
            else
            {
                strcpy(remcomOutBuffer, "E01");
            }
            break;
        }

        /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':
            /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
            if (hexToInt(&ptr, &addr))
                if (*(ptr++) == ',')
                    if (hexToInt(&ptr, &length))
                    {
                        ptr = 0;
                        mem_err = 0;
                        mem2hex((char*) addr, remcomOutBuffer, length, 1);
                        if (mem_err)
                        {
                            strcpy(remcomOutBuffer, "E03");
                            debug_error("memory fault");
                        }
                    }

            if (ptr)
            {
                esReport("%s %d\n", __FILE__, __LINE__);
                strcpy(remcomOutBuffer, "E01");
            }
            break;

        /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
        case 'M':
            /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
            if (hexToInt(&ptr, &addr))
                if (*(ptr++) == ',')
                    if (hexToInt(&ptr, &length))
                        if (*(ptr++) == ':')
                        {
                            mem_err = 0;
                            hex2mem(ptr, (char*) addr, length, 1);

                            if (mem_err)
                            {
                                strcpy(remcomOutBuffer, "E03");
                                debug_error("memory fault");
                            }
                            else
                            {
                                strcpy(remcomOutBuffer, "OK");
                            }

                            ptr = 0;
                        }
            if (ptr)
            {
                strcpy(remcomOutBuffer, "E02");
            }
            break;

        /* cAA..AA    Continue at address AA..AA(optional) */
        /* sAA..AA   Step one instruction from AA..AA(optional) */
        case 's':
            stepping = 1;
        case 'c':
            /* try to read optional parameter, pc unchanged if no parm */
            if (hexToInt(&ptr, &addr))
                registers[PC] = addr;

            newPC = registers[PC];

            /* clear the trace bit */
            registers[PS] &= 0xfffffeff;

            /* set the trace bit if we're stepping */
            if (stepping)
                registers[PS] |= 0x100;

            _returnFromException();  /* this is a jump */
            break;

        /* kill the program */
        case 'k':       /* do nothing */
#if 0
            /* Huh? This doesn't look like "nothing".
            m68k-stub.c and sparc-stub.c don't have it.  */
            BREAKPOINT ();
#endif
            break;
        }           /* switch */

        /* reply to the request */
        putpacket(remcomOutBuffer);
    }
}

/* this function is used to set up exception handlers for tracing and
   breakpoints */
void set_debug_traps(void)
{
    stackPtr = &remcomStack[STACKSIZE / sizeof (int) - 1];

    initialized = 1;
}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */

void breakpoint(void)
{
    if (initialized)
        BREAKPOINT();
}

/*
 * debug() is a front end for handle_exception.  It moves the
 * stack pointer into an area reserved for debugger use.
 */
void debug(Ureg* ureg)
{
    // Copy ureg to registers
    registers[EAX] = ureg->eax;
    registers[ECX] = ureg->ecx;
    registers[EDX] = ureg->edx;
    registers[EBX] = ureg->ebx;
    registers[ESP] = ureg->nsp + 36;
    registers[EBP] = ureg->ebp;
    registers[ESI] = ureg->esi;
    registers[EDI] = ureg->edi;
    registers[PC] = ureg->eip;
    registers[PS] = ureg->eflags;
    registers[CS] = ureg->cs;
    registers[SS] = ureg->ds;   // XXX while not using the user mode...
    registers[DS] = ureg->ds;
    registers[ES] = ureg->es;
    registers[FS] = ureg->fs;
    registers[GS] = ureg->gs;

    gdb_i386errcode = ureg->error;

    // XXX a bit dirty code here...
    asm("movl stackPtr, %esp"); /* move to remcom stack area  */

    handle_exception(ureg->trap);
}
