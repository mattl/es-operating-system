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

//
//              :               :
//              |               |
//              |    MBR, BS    | --+  <- (3) Read a boot sector
//  0000:7c00   +---------------+   |         into 0000:7c00.
//              |               |   |     (4) Jump to 0000:7c00.
//              |               |   |
//              |               |   |
//              |               |   | (1) Copy MBR from 0000:7c00
//              |               |   |              to   0000:0600.
//              |      MBR      |   | (2) Jump to 0000:0600 + offset.
//  0000:0600   +---------------+ <-+
//              |               |
//  0000:0000   +---------------+
//

// define
.set loaded,          0x7c00
.set newLocation,     0x0600
.set partitionTable,  0x1be
.set magicNumber,     0xaa55
.set retry,           0x05

// for LBA
.set Packet,          loaded
.set PacketSize,      Packet+0x00
.set PacketReserved1, Packet+0x01
.set PacketNumSec,    Packet+0x02
.set PacketReserved2, Packet+0x03
.set PacketOffset,    Packet+0x04
.set PacketSeg,       Packet+0x06
.set PacketLBA0,      Packet+0x08
.set PacketLBA16,     Packet+0x0a
.set PacketLBA32,     Packet+0x0c
.set PacketLBA48,     Packet+0x0e

.code16
.text

.globl _start
_start:                                 // at location 0000:7C00

        // Set %sp
        cli
        xorw    %ax, %ax
        movw    %ax, %ss
        movw    $loaded, %sp

        sti

        // Set %ds and %es
        movw    $0x07c0, %bx
        movw    %bx, %ds

        // copy MBR to 0000:0600.
        movw    $newLocation, %di
        movw    $0x0100, %cx            // 256 words.

        movw    $0x0000, %si
        movw    %si, %es

        repnz                           // repeat %cx times.
        movsw                           // move a word from ds:si (07c0:0000)
                                        //             to es:di (0000:0600), and
                                        // increment si and di.
                                        // Note 07c0:0000 is equal to 0000:7C00.
                                        // (0x7C00 == 0x07c0 * 0x10)

        jmp newLocation+continue-loaded // Jump to 0000:0600 + offset.
        // not reach here.

continue:
        xorw    %si, %si                // si: active partition (zero).
        movw    $pTable, %bx            // bx: partition table.
        movb    $0x04, %cl              // cl: number of partitions.

search_active_partition:
        cmpb    $0x00, (%bx)            // inactive?
        je      0f                      // continue, if so.
        cmpb    $0x80, (%bx)            // active?
        jnz     error_invalid_partition // error, if not active.

        testw   %si, %si                // Is an active partition already found?
        jnz     error_invalid_partition_table

        movw    %bx, %si                // set the active partition.
0:      addb    $0x10, %bl              // set bl to the next partition entry.
        loop    search_active_partition

        cmpw    $magicNumber, (%bx)     // check magic number
        jne     error_no_magic_number   // no magic number.

        testw   %si, %si                // Is the active partition found?
        jnz     found_active            // yes

        int     $0x18                   // diskless boot
        // not reach here.

found_active:                           // si: active partition.
                                        // set parameters for int 13.
        movw    (%si), %dx              // dl: drive number,
                                        // 0x80 represents the first hard disk.
                                        // dh: head
        movw    0x02(%si), %cx          // ch: cylinder (low)
                                        // cl: cylinder (high) and sector

        xorw    %ax, %ax
        movw    %ax, %es                // es:bx  read buffer
        movw    $loaded, %bx

        // check the location.
        movl    0x8(%si), %eax
        cmpl    $0x00feffff, %eax
        jns     chs                     // If %eax < $0x00feffff, use CHS mode.
                                        // Note 0x00feffff * 512B = 8GB.
                                        // eax: start address of the boot partition (LBA).

lba:
        // LBA mode
        pushw   %cx                     // save %cx
        pushw   %bx                     // save %bx

        // Check Extensions Present (cx and bx are modified).
        movw    $0x55aa, %bx

        movb    $0x41, %ah              // EDD extensions
        int     $0x13
        
        jc      no_edd_extension
        cmpw    $0xaa55, %bx            // Extensions present?
        jne     no_edd_extension
        testb   $0x1, %cl               // check packet mode.
        jz      error_no_edd_extension

        popw    %bx                     // restore %bx
        
        // Set a packet
        movb    $0x10, (PacketSize)
        movb    $0x00, (PacketReserved1)
        movb    $0x1,  (PacketNumSec)   // read 1 sector.
        movb    $0x00, (PacketReserved2)
        movw    %bx,   (PacketOffset)
        movw    %es,   (PacketSeg)

        movw    $0x00, (PacketLBA0)
        movw    $0x00, (PacketLBA16)
        movw    $0x00, (PacketLBA32)
        movw    $0x00, (PacketLBA48)
        movl    0x8(%si), %esi
        movl    %esi, (PacketLBA0)

        movw    $Packet, %si
        movw    $0x4200, %ax            // LBA Read.

        jmp read_boot_sector
        // skip the CHS setup.

finit_lba:
        popw    %bx                     // restore %bx
        popw    %cx                     // restore %cx

// CHS mode
chs:
        movw    $0x0201, %ax            // ah: 0x02 (read sectors).

read_boot_sector:
        movw    $retry, %di             // al: number of sectores to be read.
0:      pushw   %di
        int     $0x13                   // read one sector into 0000:7c00.
        popw    %di

        jc      reset
        cmpb    $0x00, %ah              // check error.
        je      read_boot_sector_success

reset:
        xorw    %ax, %ax                // reset disk system (ah : 0x00)
        int     $0x13

        decw    %di
        jnz     0b                      // retry

        leaw    failed_to_read_boot_sector, %si
        call    prtstr

        int     $0x18                   // diskless boot
        // not reach here.

read_boot_sector_success:
        // initialize
        movw    $loaded, %sp
        movw    %sp, %ds
        movw    $0x0000, %si
        movw    %si, %es
        movw    %si, %bp
        movw    $loaded, %bx

        jmp     *%bx                    // jump to the boot sector (0000:7C00).
        // not reach here.

// Print functions
prtchr:
        pushw   %ax
        pushw   %bx
        movw    $7, %bx                 // white.
        movb    $0x0e, %ah              // teletype output.
        int     $0x10
        popw    %bx
        popw    %ax
        ret

prtstr:                                 // [ds:si] zero terminated character string.
        lodsb                           // al = [si], si++ if DF == 0.
        andb    %al, %al                // if al == 0, zf = 1. Otherwize, zf = 0.
        jz      0f                      // if al == 0, goto the next label 0.
        call    prtchr
        jmp     prtstr
0:      ret

// Print errors
error:
        leaw    error, %si
        call    prtstr
        jmp     halt

error_no_edd_extension:
        leaw    no_edd_extension, %si
        call    prtstr
        jmp     halt


error_invalid_partition:
        leaw    invalid_partition, %si
        call    prtstr
        jmp     halt

error_invalid_partition_table:
        leaw    invalid_partition_table, %si
        call    prtstr
        jmp     halt

error_no_magic_number:
        leaw    no_magic_number, %si
        call    prtstr
        jmp     halt

halt:
        jmp     halt

// Messages
error_message:
        .ascii  "Error"
        .byte   0x0d, 0x0a, 0

no_edd_extension:
        .ascii  "No EDD extension"
        .byte   0x0d, 0x0a, 0

invalid_partition:
        .ascii  "Invalid partition"
        .byte   0x0d, 0x0a, 0

invalid_partition_table:
        .ascii  "Invalid partition table"
        .byte   0x0d, 0x0a, 0

failed_to_read_boot_sector:
        .ascii  "Failed to read boot sector"
        .byte   0x0d, 0x0a, 0

no_magic_number:
    .ascii "No magic number"
        .byte   0x0d, 0x0a, 0

// Partition Table
        .org partitionTable
pTable:
        .fill   0x40, 0x1, 0x0          // empty
        .word magicNumber

#if 0 // sample partition table and magic number.
        // the first partition entry
        .byte   0x80  // active
        .byte   0x01
        .byte   0x01
        .byte   0x00

        .byte   0x07

        .byte   0xfe
        .byte   0xbf
        .byte   0x09

        .byte   0x3f
        .byte   0x00
        .byte   0x00
        .byte   0x00
        .byte   0x4b
        .byte   0xf5
        .byte   0x7f
        .byte   0x00

        .fill   0x40 - 0x10, 0x1, 0x0
        .word magicNumber
#endif
