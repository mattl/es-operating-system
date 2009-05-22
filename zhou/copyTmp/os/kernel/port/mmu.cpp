/*
 * Copyright 2008, 2009 Google Inc.
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

#include <new>
#include <string.h>
#include <stdlib.h>
#include <es.h>
#include "core.h"
#include "process.h"

void* const Process::USER_MIN = (void*) 0x1000;
void* const Process::USER_MAX = (void*) (0x80000000 - 0x200000);

u32* Mmu::kernelDirectory = (u32*) 0x80010000;

Mmu::
Mmu(Cache* pageTable) :
    pageTable(pageTable)
{
    ASSERT(pageTable);

    // Setup the page directory
    Page* page = pageTable->getPage(0);
    directory = static_cast<u32*>(page->getPointer());
    memset(directory, 0, 2048);

    // Share kernel map
    memmove(directory + 512, kernelDirectory + 512, 2048);

    // Map TCB
    set((const void*) (0x80000000 - 8192), 0, 0x11000 | Page::PTEVALID | Page::PTEUSER);
}

Mmu::
~Mmu()
{
    for (u32* pde(directory); pde < &directory[512]; ++pde)    // user space only
    {
        if (!(*pde & Page::PTEVALID))
        {
            continue;
        }

        Page* page(PageTable::lookup(*pde));
        ASSERT(page);

        // Release swap page
        u32* table = static_cast<u32*>(page->getPointer());
        for (u32* pte(table); pte < &table[1024]; ++pte)
        {
            if (!(*pte & Page::PTEPRIVATE))
            {
                if (*pte & Page::PTEVALID)
                {
                    Page* page(PageTable::lookup(Page::pageBase(*pte)));
                    if (page)
                    {
                        page->release();
                    }
                }
            }
            else
            {
                if (*pte & Page::PTEVALID)
                {
                    Process::swap->put(PageTable::lookup(Page::pageBase(*pte)));
                }
                else
                {
                    Process::swap->put(Page::pageBase(*pte));
                }
            }
        }
        page->release();
    }
    pageTable->release();
}

void Mmu::
set(const void* addr, Page* page, unsigned long pte)
{
    unsigned long offset(reinterpret_cast<unsigned long>(addr));

    long long tableOffset = 4096 * (1 + offset / 0x400000);
    Page* table = pageTable->lookupPage(tableOffset);
    if (!table)
    {
        table = pageTable->getPage(tableOffset);
        ASSERT(table);
        memset(table->getPointer(), 0, 4096);
    }

    int i = (offset % 0x400000) / 4096;
    u32* pt = static_cast<u32*>(table->getPointer());
    u32 prev = pt[i];
    if (prev & (pte ^ prev) & (Page::PTEVALID | Page::PTEWRITE | Page::PTEUSER))
    {
        // Assume called from the current process.
        invalidate(addr);
    }

    pt[i] = pte;
    if (page)
    {
        page->addRef();
        pt[i] |= page->getAddress();
    }

    u32 pde = directory[offset / 0x400000];
    if (!(pde & Page::PTEVALID))
    {
        directory[offset / 0x400000] = table->getAddress() |
                                       Page::PTEVALID | Page::PTEWRITE | Page::PTEUSER;
    }
}

// unset() does not invalidate the TLB since the MMU might not be that of
// the current process.
void Mmu::
unset(const void* start, long count, es::Pageable* pageable, long long offset)
{
    count /= 4096;
    unsigned long addr(reinterpret_cast<unsigned long>(start));
    while (0 < count)
    {
        int i = (addr % 0x400000) / 4096;
        int countInTable = 1024 - i;
        if (count < countInTable)
        {
            countInTable = count;
        }
        count -= countInTable;

        if (!(directory[addr / 0x400000] & Page::PTEVALID))
        {
            addr += 4096 * countInTable;
            offset += 4096 * countInTable;
            continue;
        }

        Page* table(pageTable->getPage(4096 * (1 + addr / 0x400000)));
        ASSERT(table);
        u32* pt(static_cast<u32*>(table->getPointer()));
        while (0 < --countInTable)
        {
            u32 pte = pt[i];
            if (pte & Page::PTEPRIVATE)
            {
                if (pte & Page::PTEVALID)
                {
                    Page* page = PageTable::lookup(pte);
                    Process::swap->put(page);
                }
                else
                {
                    Process::swap->put(Page::pageBase(pte));
                }
            }
            else if (pte & Page::PTEVALID)
            {
                pageable->put(offset, pte);
            }
            pt[i] = 0;
            ++i;
            addr += 4096;
            offset += 4096;
        }
        table->release();
    }
}

unsigned long Mmu::
get(const void* addr)
{
    unsigned long offset = reinterpret_cast<unsigned long>(addr);
    Page* table = pageTable->lookupPage(4096 * (1 + offset / 0x400000));
    if (!table)
    {
        return 0;
    }
    else
    {
        u32* pt = static_cast<u32*>(table->getPointer());
        return pt[(offset % 0x400000) / 4096];
    }
}

void Mmu::
load()
{
#ifdef __i386__
    register u32 cr3 = (u32) directory & ~0xc0000000;
    __asm__ __volatile__ (
        "movl   %0, %%cr3\n"
        :: "a"(cr3));
#endif // __i386__
}

void Mmu::
invalidate(const void* addr)
{
#ifdef __i386__
    __asm__ __volatile__ (
        "invlpg (%0)"
        :: "r" (addr));
#endif // __i386__
}

// For accessing the physical memory from the kernel
void* Mmu::
getPointer(unsigned long pte)
{
    unsigned long base = Page::pageBase(pte);
    if (base < 0x40000000)
    {
        return reinterpret_cast<void*>(0x80000000 + base);
    }
    if (0xc0000000 <= base)
    {
        return reinterpret_cast<void*>(base);
    }
    return 0;
}
