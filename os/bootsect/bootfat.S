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

#include "fat.h"

// #define USE_CHS         // Do not use LBA mode

#if !defined(FAT12) && !defined(FAT16)
#define FAT12
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

#ifdef FAT12
        // Parameters for 3.5' 2HD HDD
        .ascii          "NINTENDO"      // BS_OEMName
        .word           512             // BPB_BytsPerSec
        .byte           1               // BPB_SecPerClus
        .word           1               // BPB_RsvdSecCnt
        .byte           2               // BPB_NumFATs
        .word           224             // BPB_RootEntCnt
        .word           2880            // BPB_TotSec16
        .byte           0xf0            // BPB_Media
        .word           9               // BPB_FATSz16
        .word           18              // BPB_SecPerTrk
        .word           2               // BPB_NumHeads
        .long           0               // BPB_HiddSec
        .long           0               // BPB_TotSec32
        .byte           0               // BS_DrvNum
        .byte           0               // BS_Reserved1
        .byte           0x29            // BS_BootSig
        .long           0               // BS_VolID
        .ascii          "NO NAME    "   // BS_VolLab
        .ascii          "FAT12   "      // BS_FilSysType
#endif
#ifdef FAT16
#if 0
        // Parameters for 32MB HDD
        .ascii          "NINTENDO"      // BS_OEMName
        .word           512             // BPB_BytsPerSec
        .byte           1               // BPB_SecPerClus
        .word           4               // BPB_RsvdSecCnt
        .byte           2               // BPB_NumFATs
        .word           512             // BPB_RootEntCnt
        .word           65467           // BPB_TotSec16
        .byte           0xf8            // BPB_Media
        .word           254             // BPB_FATSz16
        .word           17              // BPB_SecPerTrk
        .word           4               // BPB_NumHeads
        .long           17              // BPB_HiddSec
        .long           0               // BPB_TotSec32
        .byte           0x80            // BS_DrvNum
        .byte           0               // BS_Reserved1
        .byte           0x29            // BS_BootSig
        .long           0               // BS_VolID
        .ascii          "NO NAME    "   // BS_VolLab
        .ascii          "FAT16   "      // BS_FilSysType
#endif
#if 0
        // Parameters for 64MB HDD
        .ascii          "NINTENDO"      // BS_OEMName
        .word           512             // BPB_BytsPerSec
        .byte           2               // BPB_SecPerClus
        .word           2               // BPB_RsvdSecCnt
        .byte           2               // BPB_NumFATs
        .word           512             // BPB_RootEntCnt
        .word           0               // BPB_TotSec16
        .byte           0xf8            // BPB_Media
        .word           255             // BPB_FATSz16
        .word           17              // BPB_SecPerTrk
        .word           8               // BPB_NumHeads
        .long           17              // BPB_HiddSec
        .long           130951          // BPB_TotSec32
        .byte           0x80            // BS_DrvNum
        .byte           0               // BS_Reserved1
        .byte           0x29            // BS_BootSig
        .long           0               // BS_VolID
        .ascii          "NO NAME    "   // BS_VolLab
        .ascii          "FAT16   "      // BS_FilSysType
#endif
#if 1
        // Parameters for 256MB CompactFlash without MBR (CHS=695/15/48)
        .ascii          "NINTENDO"      // BS_OEMName
        .word           512             // BPB_BytsPerSec
        .byte           8               // BPB_SecPerClus
        .word           1               // BPB_RsvdSecCnt
        .byte           2               // BPB_NumFATs
        .word           512             // BPB_RootEntCnt
        .word           0               // BPB_TotSec16
        .byte           0xf8            // BPB_Media
        .word           245             // BPB_FATSz16
        .word           48              // BPB_SecPerTrk
        .word           15              // BPB_NumHeads
        .long           0               // BPB_HiddSec
        .long           500400          // BPB_TotSec32
        .byte           0x0             // BS_DrvNum
        .byte           0               // BS_Reserved1
        .byte           0x29            // BS_BootSig
        .long           0               // BS_VolID
        .ascii          "NO NAME    "   // BS_VolLab
        .ascii          "FAT16   "      // BS_FilSysType
#endif
#if 0
        // Parameters for 256MB CompactFlash (CHS=695/15/48)
        .ascii          "NINTENDO"      // BS_OEMName
        .word           512             // BPB_BytsPerSec
        .byte           8               // BPB_SecPerClus
        .word           1               // BPB_RsvdSecCnt
        .byte           2               // BPB_NumFATs
        .word           512             // BPB_RootEntCnt
        .word           0               // BPB_TotSec16
        .byte           0xf8            // BPB_Media
        .word           245             // BPB_FATSz16
        .word           48              // BPB_SecPerTrk
        .word           15              // BPB_NumHeads
        .long           48              // BPB_HiddSec
        .long           500352          // BPB_TotSec32
        .byte           0x0             // BS_DrvNum
        .byte           0               // BS_Reserved1
        .byte           0x29            // BS_BootSig
        .long           0               // BS_VolID
        .ascii          "NO NAME    "   // BS_VolLab
        .ascii          "FAT16   "      // BS_FilSysType
#endif
#if 0
        // Parameters for 512MB HDD
        .ascii          "NINTENDO"      // BS_OEMName
        .word           512             // BPB_BytsPerSec
        .byte           16              // BPB_SecPerClus
        .word           2               // BPB_RsvdSecCnt
        .byte           2               // BPB_NumFATs
        .word           512             // BPB_RootEntCnt
        .word           0               // BPB_TotSec16
        .byte           0xf8            // BPB_Media
        .word           255             // BPB_FATSz16
        .word           63              // BPB_SecPerTrk
        .word           255             // BPB_NumHeads
        .long           63              // BPB_HiddSec
        .long           1044162         // BPB_TotSec32
        .byte           0x80            // BS_DrvNum
        .byte           0               // BS_Reserved1
        .byte           0x29            // BS_BootSig
        .long           0               // BS_VolID
        .ascii          "NO NAME    "   // BS_VolLab
        .ascii          "FAT16   "      // BS_FilSysType
#endif
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
        call    prtstr

        // Calculate the number of root directory sectors
        movw    $32, %ax
        mulw    (BPB_RootEntCnt)
        movw    (BPB_BytsPerSec), %cx
        addw    %cx, %ax
        decw    %ax
        divw    %cx
        movw    %ax, %cx                // cx = number of root directory sectors

        // Calculate the first data sector number
        movzbw  (BPB_NumFATs), %ax
        mulw    (BPB_FATSz16)
        addw    (BPB_RsvdSecCnt), %ax   // dx:ax = root directory location
        pushw   %ax
        addw    %cx, %ax
        movw    %ax, (FirstDataSector)
        popw    %ax

        // Read the root directory at 2000:0000
0:      pushw   %cx
        movw    $0x2000, %bx
        movw    %bx, %es
        xorw    %di, %di                // es:di = buffer
        movw    $1, %cx
        call    read

        // Search "ES.LDR"
        movw    (BPB_RootEntCnt), %cx
        shrw    $5, %cx
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
        leaw    filename, %si
        repz
        cmpsb
        popw    %cx
        jz      found
2:      addw    $32, %bx
        loop    1b

        popw    %cx
        loop    0b

        // Not found "ES.LDR"
hang:
        leaw    failed, %si
        call    prtstr
0:      hlt
        jmp     0b

found:
        // Read "ES.LDR" at 3000:0000
        popw    %cx
        movw    %es:DIR_FstClusLO(%bx), %si     // si = first cluster number of the file

        movw    $0x3000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer
        pushw   %es
        pushw   %di
0:      call    readCluster
#ifdef FAT12
        cmpw    $0xff8, %si
#endif
#ifdef FAT16
        cmpw    $0xfff8, %si
#endif
        jc      0b
        movb    (DeviceNum), %dl        // dl = drive number
        lret                            // jump to the loaded file

// ds:si        zero terminated character string
prtstr:
        lodsb
        andb    %al, %al
        jz      0f
        movw    $7,%bx
        movb    $0x0e, %ah
        int     $0x10
        jmp     prtstr
0:      ret

// cx           number of sectors to read
// dx:ax        LBA
// es:di        buffer
//
// LBA = sector +
//       head * (BPB_SecPerTrk) +
//       cylinder  * (BPB_SecPerTrk) * (BPB_NumHeads)
//
read:   pushw   %si
        addw    (BPB_HiddSec), %ax
        adcw    (BPB_HiddSec+2), %dx    // dx:ax = root directory location plus hidden

0:      pushw   %cx
        movw    %ax, (PacketLBA0)
        movw    %dx, (PacketLBA16)
        movb    (DeviceNum), %dl

#ifdef FAT16
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
#endif

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
3:      movw    (PacketLBA0), %ax
        movw    (PacketLBA16), %dx
        addw    $1, %ax
        adcw    $0, %dx
        popw    %cx
        loop    0b

        popw    %si
        ret

// si           cluster number
// es:di        buffer
readCluster:
        movw    %si, %ax
        subw    $2, %ax
        jc      hang
        movzbw  (BPB_SecPerClus), %cx   // ecx = sector per cluster
        mulw    %cx
        addw    (FirstDataSector), %ax
        adcw    $0, %dx                 // dx:ax = first sector of the cluster
        call    read

        // Read FAT at 1000:0000
        pushw   %es
        pushw   %di
        movw    $0x1000, %ax
        movw    %ax, %es
        xorw    %di, %di                // es:di = buffer for a fat sector

#ifdef FAT12
        movw    %si, %ax
        shrw    $1, %ax
        addw    %si, %ax                // ax = fat offset
        xorw    %dx, %dx
        divw    (BPB_BytsPerSec)
        pushw   %dx                     // save fat entry offset
        xorw    %dx, %dx
        addw    (BPB_RsvdSecCnt), %ax   // dx:ax = FAT location
        movw    $2, %cx                 // cx = number of setors to read.
        cmpw    (CurFATSecNum), %ax
        movw    %ax, (CurFATSecNum)
        je      1f
        call    read

        // Get next cluster number
1:      popw    %di                     // restore fat entry offset
        movw    %es:(%di), %ax
        popw    %di
        popw    %es
        andw    $1, %si
        jz      2f
        shrw    $4, %ax
        jmp     3f
2:      andw    $0xfff, %ax
3:      movw    %ax, %si                // si = next cluster number
        ret
#endif

#ifdef FAT16
        movw    (BPB_BytsPerSec), %cx
        shrw    $1, %cx                 // cx = (BPB_BytsPerSec / 2)
        xorw    %dx, %dx
        movw    %si, %ax
        divw    %cx                     // dx = FAT entry offset
        shlw    $1, %dx
        pushw   %dx                     // push FAT entry offset
        xorw    %dx, %dx
        movw    (BPB_RsvdSecCnt), %cx
        addw    %cx, %ax                // dx:ax = FAT sector number

        cmpw    (CurFATSecNum), %ax
        movw    %ax, (CurFATSecNum)     // save CurFATSecNum
        je      4f
        movw    $1, %cx                 // cx = number of setors to read.
        call    read

        // Get next cluster number
4:      popw    %di                     // restore fat entry offset
        movw    %es:(%di), %si          // si = next cluster number
        popw    %di
        popw    %es
        ret
#endif

title:
        .ascii  "Nintendo ES"
        .byte   0x0d, 0x0a
        .byte   0

failed:
        .ascii  "Failed."
        .byte   0x0d, 0x0a
        .byte   0

filename:
        .ascii  "ES      LDR"

        .org    510
        .word   0xAA55                  // boot sector magic number
