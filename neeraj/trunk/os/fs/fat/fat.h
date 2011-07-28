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

//                                     [2HD]
//  +--------------------------------+ 0x0000
//  | Reserved Region                |
//  +--------------------------------+ 0x0200
//  | File Region #1                 |
//  +--------------------------------+ 0x1400
//  | File Region #2                 |
//  +--------------------------------+ 0x2600
//  | Root Directory Region          |
//  +--------------------------------+ 0x4200
//  | File and Directory Data Region |
//  +--------------------------------+

enum
{
    // Boot Sector and BPB Structure
    BS_jmpBoot          = 0,
    BS_OEMName          = 3,
    BPB_BytsPerSec      = 11,
    BPB_SecPerClus      = 13,
    BPB_RsvdSecCnt      = 14,
    BPB_NumFATs         = 16,
    BPB_RootEntCnt      = 17,
    BPB_TotSec16        = 19,
    BPB_Media           = 21,
    BPB_FATSz16         = 22,
    BPB_SecPerTrk       = 24,
    BPB_NumHeads        = 26,
    BPB_HiddSec         = 28,
    BPB_TotSec32        = 32,

    // Fat12 and Fat16 Structure Starting at Offset 36
    BS_DrvNum           = 36,
    BS_Reserved1        = 37,
    BS_BootSig          = 38,
    BS_VolID            = 39,
    BS_VolLab           = 43,
    BS_FilSysType       = 54,

    // FAT32 Structure Starting at Offset 36
    BPB_FATSz32         = 36,
    BPB_ExtFlags        = 40,
    BPB_FSVer           = 42,
    BPB_RootClus        = 44,
    BPB_FSInfo          = 48,
    BPB_BkBootSec       = 50,
    BPB_Reserved        = 52,
    BS32_DrvNum         = 64,
    BS32_Reserved1      = 65,
    BS32_BootSig        = 66,
    BS32_VolID          = 67,
    BS32_VolLab         = 71,
    BS32_FilSysType     = 82,

    // FAT[1] entry bit masks
    FAT16_ClnShutBitMask = 0x8000,
    FAT16_HrdErrBitMask  = 0x4000,
    FAT32_ClnShutBitMask = 0x08000000,
    FAT32_HrdErrBitMask  = 0x04000000,

    // FAT32 FSInfo Sector Structure and Backup Boot Sector
    FSI_LeadSig         = 0,
    FSI_Reserved1       = 4,
    FSI_StrucSig        = 484,
    FSI_Free_Count      = 488,
    FSI_Nxt_Free        = 492,
    FSI_Reserved2       = 496,
    FSI_TrailSig        = 508,

    // FAT 32 Byte Directory Entry Structure
    DIR_Name            = 0,
    DIR_Attr            = 11,
    DIR_NTRes           = 12,
    DIR_CrtTimeTenth    = 13,

    DIR_CrtTime         = 14,
    DIR_CrtDate         = 16,
    DIR_LstAccDate      = 18,

    DIR_FstClusHI       = 20,
    DIR_WrtTime         = 22,
    DIR_WrtDate         = 24,
    DIR_FstClusLO       = 26,
    DIR_FileSize        = 28,

    // File Attributes
    ATTR_READ_ONLY      = 0x01,
    ATTR_HIDDEN         = 0x02,
    ATTR_SYSTEM         = 0x04,
    ATTR_VOLUME_ID      = 0x08,
    ATTR_DIRECTORY      = 0x10,
    ATTR_ARCHIVE        = 0x20,
    ATTR_LONG_NAME      = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID,
    ATTR_LONG_NAME_MASK = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID |
                          ATTR_DIRECTORY | ATTR_ARCHIVE,

    // FAT Long Directory Entry Structure
    LDIR_Ord            = 0,
    LDIR_Name1          = 1,
    LDIR_Attr           = 11,
    LDIR_Type           = 12,
    LDIR_Chksum         = 13,
    LDIR_Name2          = 14,
    LDIR_FstClusLO      = 26,
    LDIR_Name3          = 28,

    LAST_LONG_ENTRY     = 0x40,

    // Etc.
    DIR_LIMIT           = 65536 * 32,   // Directory size limit
    NTRes_LOWER3        = 0x10,
    NTRes_LOWER8        = 0x08
};
