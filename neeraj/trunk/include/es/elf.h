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

/*---------------------------------------------------------------------------*

  These coded instructions, statements, and computer programs are based on
  the Executable and Linking Format (ELF) from Tool Interface Standard (TIS)
  Committee, and ELF Handling For Thread-Local Storage..

    ---------------------------------------------------------------------

    Tool Interface Standard (TIS)
    Executable and Linking Format (ELF)
    Specification Version 1.2

    TIS Committee
    May 1995

    The TIS Committee grants you a non-exclusive, worldwide, royalty-free
    license to use the information disclosed in this Specification to
    make your software TIS-compliant; no other license, express or
    implied, is granted or intended hereby.

    The TIS Committee makes no warranty for the use of this standard.

    THE TIS COMMITTEE SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS AND
    IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER
    INDIRECT DAMAGES, FOR THE USE OF THESE SPECIFICATION AND THE
    INFORMATION CONTAINED IN IT, INCLUDING LIABILITY FOR INFRINGEMENT OF
    ANY PROPRIETARY RIGHTS. THE TIS COMMITTEE DOES NOT ASSUME ANY
    RESPONSIBILITY FOR ANY ERRORS THAT MAY APPEAR IN THE SPECIFICATION,
    NOR ANY RESPONSIBILITY TO UPDATE THE INFORMATION CONTAINED IN THEM.

    The TIS Committee retains the right to make changes to this
    specification at any time without notice.

    ---------------------------------------------------------------------

 *---------------------------------------------------------------------------*/

#ifndef NINTENDO_ES_ELF_H_INCLUDED
#define NINTENDO_ES_ELF_H_INCLUDED

#include <es/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*
    32-Bit Data Types
 *---------------------------------------------------------------------------*/

typedef u32 Elf32_Addr;         // Unsigned program address
typedef u32 Elf32_Off;          // Unsigned file offset
typedef u16 Elf32_Half;         // Unsigned medium integer
typedef u32 Elf32_Word;         // Unsigned integer
typedef s32 Elf32_Sword;        // Signed integer

/*---------------------------------------------------------------------------*
    ELF Header
 *---------------------------------------------------------------------------*/

#define EI_NIDENT       16      // Size of e_ident[]

typedef struct Elf32_Ehdr {
    unsigned char   e_ident[EI_NIDENT];
    Elf32_Half      e_type;
    Elf32_Half      e_machine;
    Elf32_Word      e_version;
    Elf32_Addr      e_entry;
    Elf32_Off       e_phoff;
    Elf32_Off       e_shoff;
    Elf32_Word      e_flags;
    Elf32_Half      e_ehsize;
    Elf32_Half      e_phentsize;
    Elf32_Half      e_phnum;
    Elf32_Half      e_shentsize;
    Elf32_Half      e_shnum;
    Elf32_Half      e_shstrndx;
} Elf32_Ehdr;

// e_type
#define ET_NONE         0       // No file type
#define ET_REL          1       // Relocatable file
#define ET_EXEC         2       // Executable file
#define ET_DYN          3       // Shared object file
#define ET_CORE         4       // Core file
#define ET_LOPROC       0xff00  // Processor-specific
#define ET_HIPROC       0xffff  // Processor-specific

// e_machine
#define EM_NONE         0       // No machine
#define EM_M32          1       // AT&T WE 32100
#define EM_SPARC        2       // SPARC
#define EM_386          3       // Intel Architecture
#define EM_68K          4       // Motorola 68000
#define EM_88K          5       // Motorola 88000
#define EM_860          7       // Intel 80860
#define EM_MIPS         8       // MIPS RS3000 Big-Endian
#define EM_PPC          20      // PowerPC
#define EM_PPC64        21      // 64-bit PowerPC
#define EM_IA_64        50      // Intel IA-64 processor architecture
#define EM_X86_64       62      // AMD x86-64 architecture

// e_version
#define EV_NONE         0       // Invalid version
#define EV_CURRENT      1       // Current version

// e_ident[ ] Identification Indexes
#define EI_MAG0         0       // File identification
#define EI_MAG1         1       // File identification
#define EI_MAG2         2       // File identification
#define EI_MAG3         3       // File identification
#define EI_CLASS        4       // File class
#define EI_DATA         5       // Data encoding
#define EI_VERSION      6       // File version
#define EI_PAD          7       // Start of padding bytes

// EI_MAG0 to EI_MAG3
#define ELFMAG0         0x7f    // e_ident[EI_MAG0]
#define ELFMAG1         'E'     // e_ident[EI_MAG1]
#define ELFMAG2         'L'     // e_ident[EI_MAG2]
#define ELFMAG3         'F'     // e_ident[EI_MAG3]

// EI_CLASS
#define ELFCLASSNONE    0       // Invalid class
#define ELFCLASS32      1       // 32-bit objects
#define ELFCLASS64      2       // 64-bit objects

// EI_DATA
#define ELFDATANONE     0       // Invalid data encoding
#define ELFDATA2LSB     1       // Little-Endian
#define ELFDATA2MSB     2       // Big-Endian

/*---------------------------------------------------------------------------*
    Sections
 *---------------------------------------------------------------------------*/

// Special Section Indexes
#define SHN_UNDEF               0
#define SHN_LORESERVE           0xff00
#define SHN_LOPROC              0xff00
#define SHN_HIPROC              0xff1f
#define SHN_ABS                 0xfff1
#define SHN_COMMON              0xfff2
#define SHN_HIRESERVE           0xffff

typedef struct Elf32_Shdr {
    Elf32_Word      sh_name;
    Elf32_Word      sh_type;
    Elf32_Word      sh_flags;
    Elf32_Addr      sh_addr;
    Elf32_Off       sh_offset;
    Elf32_Word      sh_size;
    Elf32_Word      sh_link;
    Elf32_Word      sh_info;
    Elf32_Word      sh_addralign;
    Elf32_Word      sh_entsize;
} Elf32_Shdr;

// Section Types,sh_type
#define SHT_NULL                0
#define SHT_PROGBITS            1
#define SHT_SYMTAB              2
#define SHT_STRTAB              3
#define SHT_RELA                4
#define SHT_HASH                5
#define SHT_DYNAMIC             6
#define SHT_NOTE                7
#define SHT_NOBITS              8
#define SHT_REL                 9
#define SHT_SHLIB               10
#define SHT_DYNSYM              11
#define SHT_LOPROC              0x70000000
#define SHT_HIPROC              0x7fffffff
#define SHT_LOUSER              0x80000000
#define SHT_HIUSER              0xffffffff

// Section Attribute Flags
#define SHF_WRITE               0x1
#define SHF_ALLOC               0x2
#define SHF_EXECINSTR           0x4
#define SHF_TLS                 (1 << 10)
#define SHF_MASKPROC            0xf0000000

/*---------------------------------------------------------------------------*
    Symbol Table
 *---------------------------------------------------------------------------*/

#define STN_UNDEF               0

typedef struct Elf32_Sym {
    Elf32_Word      st_name;
    Elf32_Addr      st_value;
    Elf32_Word      st_size;
    unsigned char   st_info;
    unsigned char   st_other;
    Elf32_Half      st_shndx;
} Elf32_Sym;

// st_info
#define ELF32_ST_BIND(i)        ((i)>>4)
#define ELF32_ST_TYPE(i)        ((i)&0xf)
#define ELF32_ST_INFO(b,t)      (((b)<<4)+((t)&0xf))

// Symbol Binding
#define STB_LOCAL               0
#define STB_GLOBAL              1
#define STB_WEAK                2
#define STB_LOPROC              13
#define STB_HIPROC              15

// Symbol Types
#define STT_NOTYPE              0
#define STT_OBJECT              1
#define STT_FUNC                2
#define STT_SECTION             3
#define STT_FILE                4
#define STT_TLS                 6
#define STT_LOPROC              13
#define STT_HIPROC              15

/*---------------------------------------------------------------------------*
    RelocationB
 *---------------------------------------------------------------------------*/

typedef struct Elf32_Rel {
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
} Elf32_Rel;

typedef struct Elf32_Rela {
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
    Elf32_Sword     r_addend;
} Elf32_Rela;

// r_info
#define ELF32_R_SYM(i)      ((i)>>8)
#define ELF32_R_TYPE(i)     ((unsigned char)(i))
#define ELF32_R_INFO(s,t)   (((s)<<8)+(unsigned char)(t))

/*---------------------------------------------------------------------------*
    Relocation Types (Intel386)
 *---------------------------------------------------------------------------*/

#define R_386_NONE              0       //  none    none
#define R_386_32                1       //  word32  S + A
#define R_386_PC32              2       //  word32  S + A - P
#define R_386_GOT32             3       //  word32  G + A - P
#define R_386_PLT32             4       //  word32  L + A - P
#define R_386_COPY              5       //  none    none
#define R_386_GLOB_DAT          6       //  word32  S
#define R_386_JMP_SLOT          7       //  word32  S
#define R_386_RELATIVE          8       //  word32  B + A
#define R_386_GOTOFF            9       //  word32  S + A - GOT
#define R_386_GOTPC             10      //  word32  GOT + A - P

extern  Elf32_Addr  _GLOBAL_OFFSET_TABLE_[];

/*---------------------------------------------------------------------------*
    Relocation Types (PowerPC)
 *---------------------------------------------------------------------------*/

#define R_PPC_NONE              0       //  none    none
#define R_PPC_ADDR32            1       //  word32  S + A
#define R_PPC_ADDR24            2       //  low24*  (S + A) >> 2
#define R_PPC_ADDR16            3       //  half16* S + A
#define R_PPC_ADDR16_LO         4       //  half16  #lo(S + A)
#define R_PPC_ADDR16_HI         5       //  half16  #hi(S + A)
#define R_PPC_ADDR16_HA         6       //  half16  #ha(S + A)
#define R_PPC_ADDR14            7       //  low14*  (S + A) >> 2
#define R_PPC_ADDR14_BRTAKEN    8       //  low14*  (S + A) >> 2
#define R_PPC_ADDR14_BRNTAKEN   9       //  low14*  (S + A) >> 2
#define R_PPC_REL24             10      //  low24*  (S + A - P) >> 2
#define R_PPC_REL14             11      //  low14*  (S + A - P) >> 2
#define R_PPC_REL14_BRTAKEN     12      //  low14*  (S + A - P) >> 2
#define R_PPC_REL14_BRNTAKEN    13      //  low14*  (S + A - P) >> 2

#define R_PPC_GOT16             14      //  half16* G + A
#define R_PPC_GOT16_LO          15      //  half16  #lo(G + A)
#define R_PPC_GOT16_HI          16      //  half16  #hi(G + A)
#define R_PPC_GOT16_HA          17      //  half16  #ha(G + A)
#define R_PPC_PLTREL24          18      //  low24*  (L + A - P) >> 2
#define R_PPC_COPY              19      //  none    none
#define R_PPC_GLOB_DAT          20      //  word32  S + A
#define R_PPC_JMP_SLOT          21      //  none    see below
#define R_PPC_RELATIVE          22      //  word32  B + A

#define R_PPC_LOCAL24PC         23      //  low24*  see below

#define R_PPC_UADDR32           24      //  word32  S + A
#define R_PPC_UADDR16           25      //  half16* S + A
#define R_PPC_REL32             26      //  word32  S + A - P

#define R_PPC_PLT32             27      //  word32  L + A
#define R_PPC_PLTREL32          28      //  word32  L + A - P
#define R_PPC_PLT16_LO          29      //  half16  #lo(L + A)
#define R_PPL_PLT16_HI          30      //  half16  #hi(L + A)
#define R_PPC_PLT16_HA          31      //  half16  #ha(L + A)

#define R_PPC_SDAREL16          32      //  half16* S + A - _SDA_BASE_
#define R_PPC_SECTOFF           33      //  half16* R + A
#define R_PPC_SECTOFF_LO        34      //  half16  #lo(R + A)
#define R_PPC_SECTOFF_HI        35      //  half16  #hi(R + A)
#define R_PPC_SECTOFF_HA        36      //  half16  #ha(R + A)
#define R_PPC_ADDR30            37      //  word30  (S + A - P) >> 2

#define R_PPC_EMB_NADDR32       101     //  uword32 N       (A - S)
#define R_PPC_EMB_NADDR16       102     //  uhalf16 Y       (A - S)
#define R_PPC_EMB_NADDR16_LO    103     //  uhalf16 N       #lo(A - S)
#define R_PPC_EMB_NADDR16_HI    104     //  uhalf16 N       #hi(A - S)
#define R_PPC_EMB_NADDR16_HA    105     //  uhalf16 N       #ha(A - S)
#define R_PPC_EMB_SDAI16        106     //  uhalf16 Y       T
#define R_PPC_EMB_SDA2I16       107     //  uhalf16 Y       U
#define R_PPC_EMB_SDA2REL       108     //  uhalf16 Y       S + A - _SDA2_BASE_
#define R_PPC_EMB_SDA21         109     //  ulow21  N       See below
#define R_PPC_EMB_MRKREF        110     //  none    N       See below
#define R_PPC_EMB_RELSEC16      111     //  uhalf16 Y       V + A
#define R_PPC_EMB_RELST_LO      112     //  uhalf16 N       #lo(W + A)
#define R_PPC_EMB_RELST_HI      113     //  uhalf16 N       #hi(W + A)
#define R_PPC_EMB_RELST_HA      114     //  uhalf16 N       #ha(W + A)
#define R_PPC_EMB_BIT_FLD       115     //  uword32 Y       See below
#define R_PPC_EMB_RELSDA        116     //  uhalf16 Y       See below

/*---------------------------------------------------------------------------*
    Program Header
 *---------------------------------------------------------------------------*/

typedef struct Elf32_Phdr {
    Elf32_Word      p_type;
    Elf32_Off       p_offset;
    Elf32_Addr      p_vaddr;
    Elf32_Addr      p_paddr;
    Elf32_Word      p_filesz;
    Elf32_Word      p_memsz;
    Elf32_Word      p_flags;
    Elf32_Word      p_align;
} Elf32_Phdr;

// Segment Types, p_type
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6
#define PT_TLS          7
#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7fffffff

// Segment Flag Bits, p_flags
#define PF_X        0x1         // Execute
#define PF_W        0x2         // Write
#define PF_R        0x4         // Read
#define PF_MASKPROC 0xf0000000  // Unspecified

/*---------------------------------------------------------------------------*
    Dynamic Linking
 *---------------------------------------------------------------------------*/

typedef struct Elf32_Dyn {
    Elf32_Sword     d_tag;
    union {
        Elf32_Word  d_val;
        Elf32_Addr  d_ptr;
    } d_un;
} Elf32_Dyn;

extern Elf32_Dyn    _DYNAMIC[];

// d_tag                                  (i)gnored (m)andatory (o)ptional
//      Name                Value          d_un     Executable  Shared Object
#define DT_NULL             0           // i        m           m
#define DT_NEEDED           1           // d_val    o           o
#define DT_PLTRELSZ         2           // d_val    o           o
#define DT_PLTGOT           3           // d_ptr    o           o
#define DT_HASH             4           // d_ptr    m           m
#define DT_STRTAB           5           // d_ptr    m           m
#define DT_SYMTAB           6           // d_ptr    m           m
#define DT_RELA             7           // d_ptr    m           o
#define DT_RELASZ           8           // d_val    m           o
#define DT_RELAENT          9           // d_val    m           o
#define DT_STRSZ            10          // d_val    m           m
#define DT_SYMENT           11          // d_val    m           m
#define DT_INIT             12          // d_ptr    o           o
#define DT_FINI             13          // d_ptr    o           o
#define DT_SONAME           14          // d_val    i           o
#define DT_RPATH            15          // d_val    o           i
#define DT_SYMBOLIC         16          // i        i           o
#define DT_REL              17          // d_ptr    m           o
#define DT_RELSZ            18          // d_val    m           o
#define DT_RELENT           19          // d_val    m           o
#define DT_PLTREL           20          // d_val    o           o
#define DT_DEBUG            21          // d_ptr    o           i
#define DT_TEXTREL          22          // i        o           o
#define DT_JMPREL           23          // d_ptr    o           o
#define DT_BIND_NOW         24          // i        o           o
#define DT_LOPROC           0x70000000  // unspecified
#define DT_HIPROC           0x7fffffff  // unspecified

// Hashing Function
__inline unsigned long
elf_hash(const unsigned char *name)
{
    unsigned long h = 0, g;
    while (*name)
    {
        h = (h << 4) + *name++;
        if (g = h & 0xf0000000)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

#ifdef __cplusplus
}
#endif

#endif  // #ifndef NINTENDO_ES_ELF_H_INCLUDED
