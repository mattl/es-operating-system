/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef NINTENDO_ES_KERNEL_ELFFILE_H_INCLUDED
#define NINTENDO_ES_KERNEL_ELFFILE_H_INCLUDED

#include <es.h>
#include <es/elf.h>
#include <es/handle.h>
#include <es/base/IFile.h>
#include <es/base/IStream.h>

class Elf
{
    Handle<es::File>    elf;
    Handle<es::Stream>  stream;
    Elf32_Ehdr          ehdr;
    Elf32_Off           sectionNameOffset;

public:
    Elf(es::File* elf);

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
