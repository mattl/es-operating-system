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

// Boot Sector and BPB Structure
#define BS_jmpBoot          0
#define BS_OEMName          3
#define BPB_BytsPerSec      11
#define BPB_SecPerClus      13
#define BPB_RsvdSecCnt      14
#define BPB_NumFATs         16
#define BPB_RootEntCnt      17
#define BPB_TotSec16        19
#define BPB_Media           21
#define BPB_FATSz16         22
#define BPB_SecPerTrk       24
#define BPB_NumHeads        26
#define BPB_HiddSec         28
#define BPB_TotSec32        32

// Fat12 and Fat16 Structure Starting at Offset 36
#define BS_DrvNum           36
#define BS_Reserved1        37
#define BS_BootSig          38
#define BS_VolID            39
#define BS_VolLab           43
#define BS_FilSysType       54

// FAT32 Structure Starting at Offset 36
#define BPB_FATSz32         36
#define BPB_ExtFlags        40
#define BPB_FSVer           42
#define BPB_RootClus        44
#define BPB_FSInfo          48
#define BPB_BkBootSec       50
#define BPB_Reserved        52
#define BS32_DrvNum         64
#define BS32_Reserved1      65
#define BS32_BootSig        66
#define BS32_VolID          67
#define BS32_VolLab         71
#define BS32_FilSysType     82

// FAT32 FSInfo Sector Structure and Backup Boot Sector
#define FSI_LeadSig         0
#define FSI_Reserved1       4
#define FSI_StrucSig        484
#define FSI_Free_Count      488
#define FSI_Nxt_Free        492
#define FSI_Reserved2       496
#define FSI_TrailSig        508

// FAT 32 Byte Directory Entry Structure
#define DIR_Name            0
#define DIR_Attr            11
#define DIR_NTRes           12
#define DIR_CrtTimeTenth    13

#define DIR_CrtTime         14
#define DIR_CrtDate         16
#define DIR_LstAccDate      18

#define DIR_FstClusHI       20
#define DIR_WrtTime         22
#define DIR_WrtDate         24
#define DIR_FstClusLO       26
#define DIR_FileSize        28

// File Attributes
#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN         0x02
#define ATTR_SYSTEM         0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_NAME      (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | \
                             ATTR_DIRECTORY | ATTR_ARCHIVE)

// FAT Long Directory Entry Structure
#define LDIR_Ord            0
#define LDIR_Name1          1
#define LDIR_Attr           11
#define LDIR_Type           12
#define LDIR_Chksum         13
#define LDIR_Name2          14
#define LDIR_FstClusLO      26
#define LDIR_Name3          28

#define LAST_LONG_ENTRY     0x40
