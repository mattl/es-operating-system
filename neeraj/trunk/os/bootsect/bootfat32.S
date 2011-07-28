/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#include "fat.h"

#ifndef FAT32
#define FAT32
#endif

#define DeviceNum       512
#define FirstDataSector 516
#define CurFATSecNum    520

#define Packet          524
#define PacketSize      524
#define PacketReserved1 525
#define PacketNumSec    526
#define PacketReserved2 527
#define PacketOffset    528
#define PacketSeg       530
#define PacketLBA0      532
#define PacketLBA16     534
#define PacketLBA32     536
#define PacketLBA48     538

.code16
.text

// es:di        Pointer to system BIOS plug and play installation check structure
// dl           Physical device number the OS is being loaded
.globl _start
_start:                                 // at location 0000:7C00
        jmp     0f
        nop

#ifdef FAT32
        // Parameters for 512MB HDD
        .ascii          "NINTENDO"      // BS_OEMName
        .word           512             // BPB_BytsPerSec
        .byte           8               // BPB_SecPerClus
        .word           36              // BPB_RsvdSecCnt
        .byte           2               // BPB_NumFATs
        .word           0               // BPB_RootEntCnt
        .word           0               // BPB_TotSec16
        .byte           0xf8            // BPB_Media
        .word           0               // BPB_FATSz16
        .word           63              // BPB_SecPerTrk
        .word           32              // BPB_NumHeads
        .long           63              // BPB_HiddSec
        .long           1044225         // BPB_TotSec32
        .long           1018            // BPB_FATSz32
        .word           0               // BPB_ExtFlags
        .word           0               // BPB_FSVer
        .long           2               // BPB_RootClus
        .word           1               // BPB_FSInfo
        .word           6               // BPB_BkBootSec
        .long           0, 0, 0         // BPB_Reserved
        .byte           0x80            // BS32_DrvNum
        .byte           0               // BS32_Reserved1
        .byte           0x29            // BS32_BootSig
        .long           0               // BS32_VolID
        .ascii          "NO NAME    "   // BS32_VolLab
        .ascii          "FAT32   "      // BS32_FilSysType
#endif

0:
        // Set %sp
        cli
        xorw    %ax, %ax
        movw    %ax, %ss
        movw    $0x7c00, %sp
        sti

        // Set %ds and %es
        movw    $0x07c0, %bx
        movw    %bx, %ds
        movw    %bx, %es

        // Initialize bss
        movw    $14, %cx
        movw    $DeviceNum, %di
        rep
        stosw
        movb    $16, (PacketSize)
        movb    $1, (PacketNumSec)

        // Save drive number in %bl
        movb    %dl, (DeviceNum)

        // Print title message
        leaw    title, %si
        movw    $13, %cx
0:      lodsb
        movw    $7,%bx
        movb    $0x0e, %ah
        int     $0x10
        loop    0b

        // Calculate the first data sector number
        movzbl  (BPB_NumFATs), %eax
        mull    (BPB_FATSz32)           // edx is over written
        movzwl  (BPB_RsvdSecCnt), %ecx
        addl    %ecx, %eax
        movl    %eax, (FirstDataSector)

        // Read the root directory at 2000:0000
        movl    (BPB_RootClus), %esi    // esi = root cluster
0:      movw    $0x2000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer
        call    readCluster

        // Search "ES.LDR"
        movzbw  (BPB_SecPerClus), %ax
        mulw    (BPB_BytsPerSec)        // dx is over written
        shrw    $5, %ax
        movw    %ax, %cx                // cx = (BPB_BytsPerSec * BPB_SecPerClus) / 32
        xorw    %bx, %bx
1:      movb    %es:(%bx), %al
        andb    %al, %al
        jz      hang
        movb    %es:DIR_Attr(%bx), %al
        andb    $(ATTR_VOLUME_ID | ATTR_DIRECTORY), %al
        jnz     2f
        pushw   %cx
        movw    $11, %cx
        movw    %bx, %di
        pushl   %esi
        leaw    filename, %si
        repz
        cmpsb
        popl    %esi
        popw    %cx
        jz      found
2:      addw    $32, %bx
        loop    1b
        cmpl    $0x0ffffff8, %esi
        jc      0b                      // move on to the next cluster

        // Not found "ES.LDR"
hang:
#if 0
        leaw    failed, %si
        call    prtstr
#endif
0:      hlt
        jmp     0b

found:
        // Read "ES.LDR" at 3000:0000
        movzwl  %es:DIR_FstClusHI(%bx), %esi
        shll    $16, %esi
        movzwl  %es:DIR_FstClusLO(%bx), %eax
        orl     %eax, %esi              // esi = first cluster number of the file
        movw    $0x3000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer
        pushw   %es
        pushw   %di
0:      call    readCluster
        cmpl    $0x0ffffff8, %esi
        jc      0b
        movb    (DeviceNum), %dl        // dl = drive number
        lret                            // jump to the loaded file

// cx           number of sectors to read
// edx:eax      LBA
// es:di        buffer
//
// LBA = sector +
//       head * (BPB_SecPerTrk) +
//       cylinder  * (BPB_SecPerTrk) * (BPB_NumHeads)
//
read:   pushl   %esi
        addl    (BPB_HiddSec), %eax
        adcl    $0, %edx                // edx:eax = sector location plus hidden
        movl    %eax, (PacketLBA0)
        movl    %edx, (PacketLBA32)
        movb    (DeviceNum), %dl

0:      pushw   %cx

        // Check LBA support
        movb    $0x41, %ah
        movw    $0x55aa, %bx
        int     $0x13
        jc      hang
        cmpw    $0xaa55, %bx
        jne     hang
        andb    $1, %cl
        jz      hang

        // Use LBA
        movw    %di, (PacketOffset)
        movw    %es, (PacketSeg)
        movb    $0x42, %ah              // extended read
        movw    $Packet, %si
        int     $0x13
        jc      hang

        addw    (BPB_BytsPerSec), %di
        jnc     1f
        movw    %es, %ax
        addw    $0x1000, %ax
        movw    %ax, %es
1:      addl    $1, (PacketLBA0)
        adcl    $0, (PacketLBA32)
        popw    %cx
        loop    0b

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
        call    read

        // Read FAT at 1000:0000
        pushw   %es
        pushw   %di
        movw    $0x1000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer for a fat sector

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
        je      0f
        movw    $1, %cx
        call    read

        // Get next cluster number
0:      popw    %di                     // pop FAT entry offset
        movl    %es:(%di), %esi         // esi = next cluster number
        popw    %di
        popw    %es
        ret

title:
        .ascii  "Nintendo ES"
        .byte   0x0d, 0x0a

filename:
        .ascii  "ES      LDR"

        .org    510
        .word   0xAA55                  // boot sector magic number
