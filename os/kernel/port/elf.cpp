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

#include "elfFile.h"

Elf::
Elf(es::File* elf) :
    elf(elf),
    sectionNameOffset(0)
{
    stream = elf->getStream();
    if (!stream)
    {
        return;
    }
    if (stream->read(&ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr))
    {
        return;
    }

    if (ehdr.e_shstrndx != SHN_UNDEF)
    {
        Elf32_Shdr shdr;

        if (!getShdr(ehdr.e_shstrndx, &shdr))
        {
            return;
        }
        sectionNameOffset = shdr.sh_offset;
    }

    for (int i(0); i < ehdr.e_shnum; ++i)
    {
        Elf32_Shdr shdr;
        char name[32];

        if (!getShdr(i, &shdr))
        {
            return;
        }
    }
}

Elf32_Shdr* Elf::
getShdr(Elf32_Half num, Elf32_Shdr* shdr)
{
    if (ehdr.e_shoff == 0 || ehdr.e_shentsize < sizeof(Elf32_Shdr) || ehdr.e_shoff == 0)
    {
        return 0;
    }
    if (stream->read(shdr, sizeof(Elf32_Shdr), ehdr.e_shoff + ehdr.e_shentsize * num) != sizeof(Elf32_Shdr))
    {
        return 0;
    }
    return shdr;
}

char* Elf::
getSectionName(Elf32_Off offset, char* name, size_t count)
{
    if (sectionNameOffset == 0)
    {
        return 0;
    }

    offset += sectionNameOffset;
    char* ptr(name);
    int i;
    for (i = 0; i < count; ++i, ++offset, ++ptr)
    {
        stream->read(ptr, 1, offset);
        if (*ptr == 0)
        {
            break;
        }
    }
    if (count <= i)
    {
        return 0;   // too long name
    }
    return name;
}

Elf32_Phdr* Elf::
getPhdr(Elf32_Half num, Elf32_Phdr* phdr)
{
    if (ehdr.e_phnum == 0 || ehdr.e_phentsize < sizeof(Elf32_Phdr) || ehdr.e_phoff == 0)
    {
        return 0;
    }
    if (stream->read(phdr, sizeof(Elf32_Phdr), ehdr.e_phoff + ehdr.e_phentsize * num) != sizeof(Elf32_Phdr))
    {
        return 0;
    }
    return phdr;
}

void Elf::
dumpEhdr(const Elf32_Ehdr* ehdr)
{
    esReport("[Ehdr] type         : %x\n", ehdr->e_type);
    esReport("[Ehdr] machine      : %x\n", ehdr->e_machine);
    esReport("[Ehdr] version      : %d\n", ehdr->e_version);
    esReport("[Ehdr] header size  : %x\n", ehdr->e_ehsize);
    esReport("[Ehdr] entry point  : %x\n", ehdr->e_entry);

    esReport("[Ehdr] ph offset    : %d\n", ehdr->e_phoff);
    esReport("[Ehdr] ph entry size: %d\n", ehdr->e_phentsize);
    esReport("[Ehdr] ph num       : %d\n", ehdr->e_phnum);

    esReport("[Ehdr] sh offset    : %d\n", ehdr->e_shoff);
    esReport("[Ehdr] sh entry size: %d\n", ehdr->e_shentsize);
    esReport("[Ehdr] sh num       : %d\n", ehdr->e_shnum);

    esReport("[Ehdr] shstrndx     : %d\n", ehdr->e_shstrndx);
}

void Elf::
dumpShdr(const Elf32_Shdr* shdr)
{
    esReport("[Shdr] name  : %x\n", shdr->sh_name);
    esReport("[Shdr] type  : %x\n", shdr->sh_type);
    esReport("[Shdr] flag  : %x\n", shdr->sh_flags);
    esReport("[Shdr] addr  : %x\n", shdr->sh_addr);
    esReport("[Shdr] offset: %x\n", shdr->sh_offset);
    esReport("[Shdr] size  : %x\n", shdr->sh_size);
    esReport("[Shdr] link  : %x\n", shdr->sh_link);
    esReport("[Shdr] info  : %x\n", shdr->sh_info);
    esReport("[Shdr] align : %x\n", shdr->sh_addralign);
    esReport("[Shdr] entrySize: %x\n", shdr->sh_entsize);
}

void Elf::
dumpPhdr(const Elf32_Phdr* phdr)
{
    esReport("[Phdr] type               : %x\n", phdr->p_type);
    esReport("[Phdr] flags              : ");
    if (phdr->p_flags & PF_X)
    {
        esReport("X");
    }
    if (phdr->p_flags & PF_W)
    {
        esReport("W");
    }
    if (phdr->p_flags & PF_R)
    {
        esReport("R");
    }
    esReport("\n");
    esReport("[Phdr] segment offset     : %x\n", phdr->p_offset);
    esReport("[Phdr] virtual address    : %x\n", phdr->p_vaddr);

    esReport("[Phdr] segment size(file) : %x\n", phdr->p_filesz);
    esReport("[Phdr] segment size(mem)  : %x\n", phdr->p_memsz);

    esReport("[Phdr] align              : %x\n", phdr->p_align);
}
