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

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from the following specification:
 *
 * Microsoft, "Microsoft Extensible Firmware Initiative FAT32 File System
 * Specification," 6 Dec. 2000.
 * http://www.microsoft.com/whdc/system/platform/firmware/fatgen.mspx
 */

//
//              +---------------+
//              |               |
//              +---------------+
//              | ES.LDR        |
//  3000:0000   +---------------+
//              |               |
//              +---------------+
//              | Root          |
//  2000:0000   +---------------+
//              |               |
//              +---------------+
//              | FAT           |
//  1000:0000   +---------------+
//              |               |
//              +---------------+
//              | Boot Sector   |
//  0000:7C00   +---------------+
//              |               |
//  0000:0000   +---------------+
//

// 0x00008000 - 0x000081ff      VESA SVGA information
// 0x00008200 - 0x000083ff      VESA SVGA mode information
// 0x00008400 -                 System memory map
// 0x00009000 - 0x0000ffff      Default kernel stack
// 0x00010000 - 0x00010fff      Page directory table
// 0x00011000 - 0x00011fff      User TCB
// 0x00012000 - 0x00012fff      Kernel TCBs
// 0x00013000 - 0x00013fff      AP initial stack
// 0x00014000 - 0x00014fff      Kernel page table
// 0x00020000 - 0x00020fff      FDC DMA buffer
// 0x00021000 - 0x00021fff      SB16 8-bit DMA buffer
// 0x00022000 - 0x00023fff      SB16 16-bit DMA buffer
// 0x00030000 - 0x0004ffff      MPS Core objects
// 0x00100000 - 0x00ffffff      Heap
// 0x01000000 -                 Kernel

#include "fat.h"

#define USE_SVGA
// #define USE_CHS         // Do not use LBA mode

//
// Configuration
//

#define XResolution             18      // Horizontal resolution
#define YResolution             20      // Vertical resolution
#define BitsPerPixel            25      // Bits per pixel

// 0x141 1024x768  32bit Dcolr A000 RW ---- -- 64 64 C000:6F01 FD000000 VMWare
// 0x144 1024x768  32bit Dcolr A000 RW ---- -- 64 64 C000:6C4B E0000000 Bochs
// 0x118 1024x768  32bit Dcolr A000 RW ---- -- 64 64 C800:1DEE F8000000 Visual PC 2004

#define Smap                    0x534d4150      // "SMAP" signature for 0xE820 BIOS service

//
// Memory offset
//

#define DeviceNum               90
#define RootDirSectors          92
#define FirstDataSector         96
#define CurFATSecNum            100
#define CountOfClusters         104

#define Packet                  108
#define PacketSize              108
#define PacketReserved1         109
#define PacketNumSec            110
#define PacketReserved2         111
#define PacketOffset            112
#define PacketSeg               114
#define PacketLBA0              116
#define PacketLBA16             118
#define PacketLBA32             120
#define PacketLBA48             122

#define Dest                    124

#define Shutdown                124     // _shutdown address
#define Startap                 126     // _startap address
#define Font                    128     // VGA font address
#define Entryap                 132     // AP protected-mode entry address [m16:32]
#define Halt                    138
#define Width                   140     // Screen width
#define Height                  142     // Screen height

.code16
.text

// dl           Physical device number the OS is being loaded
.globl _start
_start:                                 // at location 3000:0000
        jmp     0f
        nop

        // BPB is copied here

        .org    Entryap
        .word   0, 0, 8
        .org    Halt
        .word   0
        .org    Width
        .word   1024                    // Screen width
        .org    Height
        .word   768                     // Screen height

0:
        // Set %sp
        cli
        xorw    %ax, %ax
        movw    %ax, %ss
        movw    $0x7c00, %sp
        sti

        // Set %ds and %es
        movw    $0x3000, %ax
        movw    %ax, %ds
        movw    %ax, %es

        // Print title message
        leaw    title, %si
        call    prtstr

        // Enable A20 gate
        call    testA20
        jnz     a20_enabled

        movw    $0x2401, %ax            // Via BIOS
        int     $0x15
        call    testA20
        jnz     a20_enabled

        inb     $0x92, %al              // Via System Control Port A
        orb     $0x02, %al              // Bit 1: 0: disable A20, 1: enable A20
        andb    $0xfe, %al
        outb    %al, $0x92
        call    testA20
        jnz     a20_enabled

        jmp     hang
a20_enabled:
        wbinvd

        // Initialize bss
        xorw    %ax, %ax
        movw    $20, %cx
        movw    $DeviceNum, %di
        rep
        stosw
        movb    $16, (PacketSize)
        movb    $1, (PacketNumSec)

        // Save drive number in %bl
        movb    %dl, (DeviceNum)

        // Set up gdt48
        xorl    %eax, %eax
        movw    %ds, %ax
        shll    $4, %eax
        addl    $gdt, %eax
        movl    %eax, (gdt48+2)

        // Copy BIOS Parameter Block
        pushw   %ds
        movw    $0x07c0, %ax
        movw    %ax, %ds
        xorw    %ax, %ax
        movw    %ax, %si
        movw    %ax, %di
        movw    $90, %cx
        rep
        movsb
        popw    %ds

        // Calculate the number of root directory sectors
        movw    $32, %ax
        mulw    (BPB_RootEntCnt)
        movw    (BPB_BytsPerSec), %cx
        addw    %cx, %ax
        decw    %ax
        xorw    %dx, %dx
        divw    %cx
        movw    %ax, (RootDirSectors)

        // Calculate the first data sector number
        movzbl  (BPB_NumFATs), %eax
        movzwl  (BPB_FATSz16), %edx
        andl    %edx, %edx
        jnz     0f
        movl    (BPB_FATSz32), %edx
0:      mull    %edx
        movzwl  (BPB_RsvdSecCnt), %ecx
        addl    %ecx, %eax
        movl    %eax, %ebx              // ebx = root directory location used by readRoot
        movzwl  (RootDirSectors), %ecx
        addl    %ecx, %eax
        movl    %eax, (FirstDataSector)

        // Determine the count of clusters
        movzwl  (BPB_TotSec16), %eax
        andw    %ax, %ax
        jnz     0f
        movl    (BPB_TotSec32), %eax
0:      subl    (FirstDataSector), %eax
        movzbl  (BPB_SecPerClus), %ecx
        xorl    %edx, %edx
        divl    %ecx                    // eax = count of clusters
        movl    %eax, (CountOfClusters)
        cmpl    $4085, %eax
        jae     1f
        leaw    fat12, %si
        jmp     readRoot
1:      cmpl    $65525, %eax
        jae     2f
        leaw    fat16, %si
        jmp     readRoot
2:      leaw    fat32, %si
        jmp     readRoot32

readRoot:
        // Read the root directory at 2000:0000
        call    prtstr
        movl    %ebx, %eax
        xorl    %edx, %edx              // edx:eax = root directory location
        movw    (RootDirSectors), %cx

0:      pushw   %cx
        movw    $0x2000, %bx
        movw    %bx, %es
        xorw    %di, %di                // es:di = buffer
        movw    $1, %cx
        call    read

        movw    (BPB_BytsPerSec), %cx
        shrw    $5, %cx
        call    list

        movw    (BPB_BytsPerSec), %cx
        shrw    $5, %cx
        call    lookup                  // search "ES.IMG"
        popw    %cx
        jnz     readImg
        loop    0b
        jmp     hang

readRoot32:
        call    prtstr
        movzbw  (BPB_SecPerClus), %ax
        mulw    (BPB_BytsPerSec)        // dx is over written
        shrw    $5, %ax                 // ax = (BPB_BytsPerSec * BPB_SecPerClus) / 32

        movl    (BPB_RootClus), %esi    // esi = root cluster
0:      pushw   %ax
        movw    $0x2000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer
        call    readCluster
        popw    %cx
        pushl   %esi
        pushw   %cx
        call    list
        popw    %cx
        call    lookup                  // search "ES.IMG"
        jnz     1f
        popl    %esi
        cmpl    $0x0ffffff8, %esi
        jc      0b                      // move on to the next cluster
        jmp     hang                    // not found
1:      popl    %eax

readImg:
        // Read the file into memory at 2000:0000
        movl    $0x1000000, (Dest)

0:      movw    $0x2000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer
        call    readCluster
        pushl   %esi

        // Copy the file to memory at 0x1000000
        // esi = src
        // edi = dest
        // ecx = len
        movzwl  (BPB_BytsPerSec), %eax
        movzbl  (BPB_SecPerClus), %ecx
        mull    %ecx
        movl    %eax, %ecx
        movl    (Dest), %eax
        movl    %eax, %edi
        addl    %ecx, %eax
        movl    %eax, (Dest)
        movl    $0x20000, %esi
        call    copy

        // Progress bar
        movb    $0x0d, %al
        call    prtchr
        movl    (Dest), %eax
        call    prthl

        // Determine FAT12, FAT16 or FAT32
        popl    %esi
        movl    (CountOfClusters), %eax
        cmpl    $4085, %eax
        jae     1f
        cmpl    $0xff8, %esi            // FAT12
        jc      0b
        jmp     run
1:      cmpl    $65525, %eax
        jae     2f
        cmpl    $0xfff8, %esi           // FAT16
        jc      0b
        jmp     run
2:      cmpl    $0x0ffffff8, %esi       // FAT32
        jc      0b

run:
        movb    (DeviceNum), %dl        // dl = drive number
        leaw    crlf, %si
        call    prtstr

        // Get system memory map at 0840:0000
        movw    $0x0840, %ax
        movw    %ax, %es
        xorw    %di, %di
        xorl    %ebx, %ebx
0:      movl    $20, %ecx
        movl    $Smap, %edx
        movw    $0xe820, %ax
        int     $0x15
        jc      1f
        cmpl    $Smap, %eax
        jne     hang
        addw    $20, %di
        andl    %ebx, %ebx
        jnz     0b
        jmp     2f
1:      movw    $0x0840, %ax
        movw    %ax, %es
        xorw    %di, %di
        movw    $0xe801, %ax
        int     $0x15                   // %dx = (memory size - 16MB) / 64KB
        jc      hang
        movl    $0x1000000, %eax
        stosl
        xor     %eax, %eax
        stosl
        movw    %dx, %ax
        shll    $16, %eax
        stosl
        xor     %eax, %eax
        stosl
        movl    $1, %eax
        stosl
2:      xorl    %eax, %eax
        movl    $5, %ecx
        rep
        stosl

        // Get VESA SVGA information at 0800:0000
        movw    $0x0800, %ax
        movw    %ax, %es
        xorw    %di, %di
        leaw    vbe2, %si
        movsl
        xorw    %di, %di
        movw    $0x4f00, %ax
        int     $0x10
        cmpb    $0x4f, %al
        jne     text_mode
        andb    %ah, %ah
        jnz     text_mode

        movw    %es:(14), %di
        movw    %es:(16), %ax
        movw    %ax, %es                // %es:%di = mode list
0:
        movw    %es:(%di), %cx
        cmpw    $0xffff, %cx
        je      text_mode
        addw    $2, %di

        // Get VESA SVGA mode information at 0820:0000
        pushw   %es
        pushw   %di

        movw    $0x0820, %ax
        movw    %ax, %es
        xorw    %di, %di
        movw    $0x4f01, %ax
        int     $0x10
        cmpb    $0x4f, %al
        jne     1f
        andb    %ah, %ah
        jnz     1f
        andb    $1, %es:(0)
        jz      1f
        movw    %es:(XResolution), %ax
        cmpw    (Width), %ax
        jne     1f
        movw    %es:(YResolution), %ax
        cmpw    (Height), %ax
        jne     1f
        movb    %es:(BitsPerPixel), %al           // 24 <= depth ?
        cmpb    $24, %al
        jae     2f
1:
        popw    %di
        popw    %es
        jmp     0b
2:
        popw    %di
        popw    %es
3:
        // Switch to SVGA mode using VESA BIOS
#ifdef USE_SVGA
        movw    $0x4f02, %ax
        movw    $0x4000, %bx            // 0x4000: enable linear frame-buffer
        orw     %cx, %bx
        int     $0x10
        cmpb    $0x4f, %al
        jne     0b
        andb    %ah, %ah
        jnz     0b

        // Get VGA font information
        movb    $0x06, %bh              // ROM 8x16 font
        movw    $0x1130, %ax
        int     $0x10
        movw    %bp, (Font)
        movw    %es, %ax
        movw    %ax, (Font+2)
#endif
text_mode:

        // Save APM _shutdown address
        leaw    _shutdown, %ax
        movw    %ax, (Shutdown)

        // Save MPS _startap address
        leaw    _startap, %ax
        movw    %ax, (Startap)

        // Save _halt address
        leaw    _halt, %ax
        movw    %ax, (Halt)

        cli

        lgdt    gdt48                   // Load GDT

        // Switch to the protected mode
        movl    %cr0, %eax
        orl     $1, %eax
        movl    %eax, %cr0

        .code32
        .byte 0x66                      // Need prefix as still using 16bit CS
        ljmp    $8, $(0x3000*16 + 0f)   // Clear prefetch and normalize %eip
0:
        xorl    %eax, %eax
        lldt    %ax                     // Invalidate LDTR

        movw    $16, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %ss

        movw    $40, %ax
        movw    %ax, %gs

        lidt    idt48                   // Load IDT

        // Turn on PSE
        movl    %cr4, %eax
        orl     $0x10, %eax
        movl    %eax, %cr4

        // Setup page table (PDE at 0x10000)
        movl    $512, %ecx
        movl    $0x10000, %edi
        movl    $0x83, %eax
0:      stosl
        addl    $0x00400000, %eax
        loop    0b

        movl    $256, %ecx
        movl    $0x83, %eax
0:      stosl
        addl    $0x00400000, %eax
        loop    0b

        movl    $256, %ecx
        movl    $0xc0000083, %eax
0:      stosl
        addl    $0x00400000, %eax
        loop    0b

        // Load page directory
        movl    $0x10000, %eax
        movl    %eax, %cr3

        // Enable paging
        movl    %cr0, %eax
        orl     $0x80010000, %eax
        movl    %eax, %cr0              // and set paging (PG) bit with WP

        movl    $0x80010000, %esp
        pushl   %esp
        movl    %esp, %gs:0             // Set kernel TLS thread pointer
        xorl    %eax, %eax
        movl    $511, %ecx              // Reserve 2KB for the default TLS
0:      pushl   %eax
        loop    0b

        pushl   %eax                    // argv
        pushl   %eax                    // argc
        ljmpl   $8, $0x81000010         // Jump to the es.img

.globl _shutdown
_shutdown:
        cli

        // Switch to the real address mode
        ljmp    $24, $1f
1:
        .code16
        movw    $32, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        movw    %ax, %ss

        movl    %cr0, %eax
        andl    $0x7ffffffe, %eax
        movl    %eax, %cr0
        jmpl    $0x3000, $2f
2:      movw    $0x3000, %ax
        movw    %ax, %fs
        movw    %ax, %gs
        movw    %ax, %ds
        movw    %ax, %es

        xorw    %ax, %ax
        movw    %ax, %ss
        movw    $0x7c00, %sp

        lidt    idt48Real

        sti

        // APM shutdown
        movw    $0x5300, %ax    // APM installation check
        xorw    %bx, %bx
        int     $0x15
        jc      3f
        cmpw    $0x0101, %ax
        jb      3f
        movw    $0x5301, %ax    // APM real mode interface connect
        xorw    %bx, %bx
        int     $0x15
        jc      3f
        movw    $0x530e, %ax    // APM driver version
        xorw    %bx, %bx
        movw    $0x0101, %cx
        int     $0x15
        jc      3f
        movw    $0x530f, %ax    // Engage power management
        movw    $1, %bx
        movw    $0x0001, %cx
        int     $0x15
        movw    $0x5308, %ax    // Enable power management
        movw    $1, %bx
        movw    $0x0001, %cx
        int     $0x15
        movw    $0x5307, %ax    // Set power state
        movw    $1, %bx
        movw    $0x0003, %cx
        int     $0x15

3:
        // Switch to text mode
        movw    $0x0003, %ax
        int     $0x10

        leaw    turnoff, %si
        call    prtstr
        jmp     _halt

hang:
        leaw    failed, %si
        call    prtstr
.globl _halt
_halt:
        hlt
        jmp     _halt

prtchr:
        pushw   %ax
        pushw   %bx
        movw    $7,%bx
        movb    $0x0e, %ah
        int     $0x10
        popw    %bx
        popw    %ax
        ret

// ds:si        zero terminated character string
prtstr:
        lodsb
        andb    %al, %al
        jz      0f
        call    prtchr
        jmp     prtstr
0:      ret

prthl:
        pushl   %eax
        shrl    $16, %eax
        call    prthw
        popl    %eax
prthw:
        pushw   %ax
        movb    %ah, %al
        call    prthb
        popw    %ax
prthb:
        pushw   %ax
        shrb    $4, %al
        call    0f
        popw    %ax
        pushw   %ax
        andb    $0x0f, %al
        call    0f
        popw    %ax
        ret
0:      cmpb    $0x0a, %al
        jc      1f
        addb    $0x07, %al
1:      addb    $0x30, %al
        jmp     prtchr

// cx           number of sectors to read
// edx:eax      LBA
// es:di        buffer
//
// LBA = sector +
//       head * (BPB_SecPerTrk) +
//       cylinder  * (BPB_SecPerTrk) * (BPB_NumHeads)
//
read:
        pushl   %esi
        addl    (BPB_HiddSec), %eax
        adcl    $0, %edx                // edx:eax = sector location plus hidden

0:      pushw   %cx
        movl    %eax, (PacketLBA0)
        movl    %edx, (PacketLBA32)
        movb    (DeviceNum), %dl

#ifndef USE_CHS
        // Check LBA support
        movb    $0x41, %ah
        movw    $0x55aa, %bx
        int     $0x13
        jc      1f
        cmpw    $0xaa55, %bx
        jne     1f
        andb    $1, %cl
        jz      1f

        // Use LBA
        movw    %di, (PacketOffset)
        movw    %es, (PacketSeg)
        movb    $0x42, %ah              // extended read
        movw    $Packet, %si
        int     $0x13
        jnc     2f
#endif

        // Use CHS
1:      movw    (PacketLBA0), %ax
        movw    (PacketLBA16), %dx
        divw    (BPB_SecPerTrk)
        incb    %dl
        movb    %dl, %cl                // cl = sector
        xorw    %dx, %dx
        divw    (BPB_NumHeads)          // dx = head, ax = cylinder
        movb    %al, %ch                // cylinder
        shl     $6, %ah
        orb     %ah, %cl
        movb    %dl, %dh                // head
        movb    $0x02, %ah              // read disk sector
        movb    $1, %al                 // number of sectors to read
        movw    %di, %bx
        movb    (DeviceNum), %dl
        int     $0x13
        jc      hang

2:
        addw    (BPB_BytsPerSec), %di
        jnc     3f
        movw    %es, %ax
        addw    $0x1000, %ax
        movw    %ax, %es
3:      movl    (PacketLBA0), %eax
        movl    (PacketLBA32), %edx
        addl    $1, %eax
        adcl    $0, %edx
        popw    %cx
        decw    %cx
        jnz     0b

        popl    %esi
        ret

// esi          cluster number
// es:di        buffer
readCluster:
        movl    %esi, %eax
        subl    $2, %eax
        jc      hang
        movzbl  (BPB_SecPerClus), %ecx  // ecx = sector per cluster
        mull    %ecx
        addl    (FirstDataSector), %eax
        adcl    $0, %edx                // edx:eax = first sector of the cluster
        call    read

        // Read FAT at 1000:0000
        pushw   %es
        pushw   %di
        movw    $0x1000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer for a fat sector

        // Determine FAT12, FAT16 or FAT32
        cmpl    $65525, (CountOfClusters)
        jae     rf32
        cmpl    $4085, (CountOfClusters)
        jae     rf16

rf12:   // FAT 12
        movl    %esi, %eax
        shrl    $1, %eax
        addl    %esi, %eax              // eax = fat offset
        xorl    %edx, %edx
        movzwl  (BPB_BytsPerSec), %ecx
        divl    %ecx
        pushw   %dx                     // save fat entry offset
        xorl    %edx, %edx
        movzwl  (BPB_RsvdSecCnt), %ecx
        addl    %ecx, %eax              // edx:eax = FAT sector number
        cmpl    (CurFATSecNum), %eax
        movl    %eax, (CurFATSecNum)
        je      1f
        movw    $2, %cx                 // cx = number of setors to read.
        call    read

        // Get next cluster number
1:      popw    %di                     // restore fat entry offset
        movw    %es:(%di), %ax
        popw    %di
        popw    %es
        andw    $1, %si
        jz      2f
        shrl    $4, %eax
2:      andl    $0xfff, %eax
        movl    %eax, %esi              // esi = next cluster number
        ret

rf16:   // FAT 16
        movzwl  (BPB_BytsPerSec), %ecx
        shrl    $1, %ecx                // ecx = (BPB_BytsPerSec / 2)
        xorl    %edx, %edx
        movl    %esi, %eax
        divl    %ecx                    // edx = FAT entry offset
        shlw    $1, %dx
        pushw   %dx                     // push FAT entry offset
        xorl    %edx, %edx
        movzwl  (BPB_RsvdSecCnt), %ecx
        addl    %ecx, %eax              // edx:eax = FAT sector number

        cmpl    (CurFATSecNum), %eax
        movl    %eax, (CurFATSecNum)    // save CurFATSecNum
        je      4f
        movw    $1, %cx                 // cx = number of setors to read.
        call    read

        // Get next cluster number
4:      popw    %di                     // restore fat entry offset
        movw    %es:(%di), %si          // si = next cluster number
        andl    $0xffff, %esi
        popw    %di
        popw    %es
        ret

rf32:   // FAT 32
        movzwl  (BPB_BytsPerSec), %ecx
        shrl    $2, %ecx                // ecx = (BPB_BytsPerSec / 4)
        xorl    %edx, %edx
        movl    %esi, %eax
        divl    %ecx                    // edx = FAT entry offset
        shlw    $2, %dx
        pushw   %dx                     // push FAT entry offset
        xorl    %edx, %edx
        movzwl  (BPB_RsvdSecCnt), %ecx
        addl    %ecx, %eax              // edx:eax = FAT sector number

        cmpl    (CurFATSecNum), %eax
        movl    %eax, (CurFATSecNum)    // save CurFATSecNum
        je      5f
        movw    $1, %cx                 // cx = number of setors to read.
        call    read

        // Get next cluster number
5:      popw    %di                     // pop FAT entry offset
        movl    %es:(%di), %esi         // esi = next cluster number
        popw    %di
        popw    %es
        ret

// List the root directory
// %cx          # of directory entries
// %es          location where the directory is read
list:
        xorw    %di, %di
0:      movw    %di, %bx
        movb    %es:(%bx), %al
        andb    %al, %al
        jz      3f
        cmpb    $0xe5, %al
        je      2f
        movb    %es:DIR_Attr(%bx), %al
        andb    $(ATTR_VOLUME_ID | ATTR_DIRECTORY), %al
        jnz     2f
        pushw   %cx
        movw    $11, %cx
1:      movb    %es:(%bx), %al
        call    prtchr
        incw    %bx
        loop    1b
        popw    %cx
        leaw    crlf, %si
        call    prtstr
2:      addw    $32, %di
        loop    0b
3:      ret

// Search "ES.IMG"
// %cx          # of directory entries
// %es          location where the directory is read
lookup:
        xorw    %bx, %bx
0:      movb    %es:(%bx), %al
        andb    %al, %al
        jz      hang                    // end of directory
        movb    %es:DIR_Attr(%bx), %al
        andb    $(ATTR_VOLUME_ID | ATTR_DIRECTORY), %al
        jnz     1f
        pushw   %cx
        movw    $11, %cx
        movw    %bx, %di
        leaw    filename, %si
        repz
        cmpsb
        popw    %cx
        jz      2f
1:      addw    $32, %bx
        loop    0b
        xorl    %esi, %esi              // not found
        ret
2:      movzwl  %es:DIR_FstClusHI(%bx), %esi
        shll    $16, %esi
        movzwl  %es:DIR_FstClusLO(%bx), %eax
        orl     %eax, %esi              // esi = first cluster number of the file
        ret

// esi          src
// edi          dest
// ecx          len
copy:
        cli

        // Set up GDT and IDT
        lidt    idt48
        lgdt    gdt48

        // Switch to the protected mode
        movl    %cr0, %eax
        orl     $1, %eax
        movl    %eax, %cr0

        .code32
        .byte 0x66                      // Need prefix as still using 16bit CS
        ljmp    $8, $(0x3000*16 + 0f)   // Clear prefetch and normalize %eip
0:
        movw    $16, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        movw    %ax, %ss

        rep
        movsb

        // Switch to the real address mode
        ljmp    $24, $1f
1:      .code16
        movw    $32, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        movw    %ax, %ss

        lidt    idt48Real

        movl    %cr0, %eax
        andl    $0xfffffffe, %eax
        movl    %eax, %cr0
        jmpl    $0x3000, $2f
2:      xorw    %ax, %ax
        movw    %ax, %fs
        movw    %ax, %gs
        movw    %ax, %ss

        movw    $0x3000, %ax
        movw    %ax, %ds
        movw    %ax, %es

        sti
        ret

title:
        .ascii  "Kernel Loader"
crlf:
        .byte   0x0d, 0x0a, 0

failed:
        .ascii  "Failed."
        .byte   0x0d, 0x0a, 0

turnoff:
        .ascii  "It is now safe to turn off your computer."
        .byte   0x0d, 0x0a, 0

fat12:
        .ascii  "FAT12"
        .byte   0x0d, 0x0a, 0

fat16:
        .ascii  "FAT16"
        .byte   0x0d, 0x0a, 0

fat32:
        .ascii  "FAT32"
        .byte   0x0d, 0x0a, 0

vbe2:
        .ascii  "VBE2"

filename:
        .ascii  "ES      IMG"

        // Global Descriptor Table (GDT)
        .p2align 4
gdt:
        // Not used
        .word   0
        .word   0
        .word   0
        .word   0

        // Kernel code
        .word   0xFFFF                  // 4GB
        .word   0                       // base address = 0
        .word   0x9A00                  // P:00b:S:1010b:00h (code read/exec)
        .word   0x00CF                  // G:D:0b:0b:fh

        // Kernel data
        .word   0xFFFF                  // 4GB
        .word   0                       // base address = 0
        .word   0x9200                  // P:00b:S:0010b:00h (data read/write)
        .word   0x00CF                  // G:D:0b:0b:fh

        // 64KB code
        .word   0xFFFF                  // 64KB
        .word   0x0000                  // base address = 0x30000
        .word   0x9A03                  // P:00b:S:1010b:03h (code read/exec)
        .word   0x0000                  // 0b:0b:0b:0b:0h

        // 64KB data
        .word   0xFFFF                  // 64KB
        .word   0x0000                  // base address = 0x30000
        .word   0x9203                  // P:00b:S:0010b:03h (data read/write)
        .word   0x0000                  // 0b:0b:0b:0b:0h

        // Kernel TCB
        .word   3                       // sizeof(Core::Tcb) - 1
        .word   0x2000                  // base address = 0x80012000
        .word   0x9201                  // P:00b:S:0010b:01h (data read/write)
        .word   0x8040                  // 80h:0b:D:0b:0b:0h
gdtEnd:

        .p2align 2
        .word   0                       // alignment byte
idt48:
        .word   0                       // idt limit = 0
        .word   0, 0                    // idt base = 0L

        .word   0                       // alignment byte
gdt48:
        .word   gdtEnd - gdt - 1        // gdt limit
        .word   0, 0                    // gdt base

        .word   0                       // alignment byte
idt48Real:
        .word   0x400                   // idt limit = 0
        .word   0, 0                    // idt base = 0L

        .p2align 12
        // Application processor startup routine
.globl _startap
_startap:
        // Set %sp
        cli
        xorw    %ax, %ax
        movw    %ax, %ss
        movw    $0x7c00, %sp

        // Set %ds and %es
        movw    $0x3000, %ax
        movw    %ax, %ds
        movw    %ax, %es

        cli
        cld

        lgdt    gdt48                   // Load GDT

        // Switch to the protected mode
        movl    %cr0, %eax
        orl     $1, %eax
        movl    %eax, %cr0

        .code32
        .byte 0x66                      // Need prefix as still using 16bit CS
        ljmp    $8, $(0x3000*16 + 0f)   // Clear prefetch and normalize %eip
0:
        xorl    %eax, %eax
        lldt    %ax                     // Invalidate LDTR

        movw    $16, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %ss

        movw    $40, %ax
        movw    %ax, %gs

        lidt    idt48                   // Load IDT

        // Turn on PSE
        movl    %cr4, %eax
        orl     $0x10, %eax
        movl    %eax, %cr4

        // Load page directory
        movl    $0x10000, %eax
        movl    %eax, %cr3

        // Enable paging
        movl    %cr0, %eax
        orl     $0x80010000, %eax
        movl    %eax, %cr0              // and set paging (PG) bit with WP

        movl    $0x80014000, %esp
        pushl   %esp
        movl    %esp, %gs:0             // Set AP TLS thread pointer
        xorl    %eax, %eax
        movl    $511, %ecx              // Reserve 2KB for the default TLS
0:      pushl   %eax
        loop    0b

        pushl   %eax                    // argv
        pushl   %eax                    // argc
        jmp     *(0x30000 + Entryap)    // Jump to the es.img

        .code16

testA20:
        xorw    %ax, %ax
        movw    %ax, %fs                // 0x0000
        decw    %ax
        movw    %ax, %gs                // 0xffff
        movw    $2, %cx
0:      incw    %ax
        movw    %ax, %fs:(0x200)
        outb    %al,$0x80               // delay
        cmpw    %gs:(0x210), %ax
        loope   0b
        ret
