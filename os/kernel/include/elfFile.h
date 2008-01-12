/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *  
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#ifndef NINTENDO_ES_KERNEL_ELFFILE_H_INCLUDED
#define NINTENDO_ES_KERNEL_ELFFILE_H_INCLUDED

#include <es.h>
#include <es/elf.h>
#include <es/handle.h>
#include <es/base/IFile.h>
#include <es/base/IStream.h>

class Elf
{
    Handle<IFile>   elf;
    Handle<IStream> stream;
    Elf32_Ehdr      ehdr;
    Elf32_Off       sectionNameOffset;

public:
    Elf(IFile* elf);

    Elf32_Shdr* getShdr(Elf32_Half num, Elf32_Shdr* shdr);
    Elf32_Phdr* getPhdr(Elf32_Half num, Elf32_Phdr* phdr);

    char* getSectionName(Elf32_Off offset, char* name, size_t count);

    Elf32_Half getType()
    {
        return ehdr.e_type;
    }

    Elf32_Half getPhnum()
    {
        return ehdr.e_phnum;
    }

    Elf32_Addr getEntry()
    {
        return ehdr.e_entry;
    }

    static void dumpEhdr(const Elf32_Ehdr* ehdr);
    static void dumpShdr(const Elf32_Shdr* shdr);
    static void dumpPhdr(const Elf32_Phdr* shdr);
};

#endif // NINTENDO_ES_KERNEL_ELFFILE_H_INCLUDED
