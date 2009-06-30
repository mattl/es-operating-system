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

#include <string.h>
#include "core.h"

Xreg::
Xreg()
{
    ASSERT((long) &fxreg % 16 == 0);
    if (Core::fxsr)
    {
        memset(&fxreg, 0, sizeof(Fxreg));
        fxreg.fcw = 0x037f;
        // fxreg.ftw = 0xffff;
        if (Core::sse)
        {
            fxreg.mxcsr = 0x1f80;
        }
    }
    else
    {
        memset(&freg, 0, sizeof(Freg));
        freg.fcw = 0x037f;
        // freg.ftw = 0xffff;
    }
}

void Xreg::
save()
{
    if (Core::fxsr)
    {
        __asm__ __volatile__ (
            "fxsave %0\n"
            "fnclex"
            : "=m" (fxreg));
    } else {
        __asm__ __volatile__ (
            "fnsave %0\n"
            "fwait"
            : "=m" (freg));
    }
}

void Xreg::
restore()
{
    if (Core::fxsr)
    {
        __asm__ __volatile__ (
            "fxrstor %0\n"
            "fwait"
            :: "m" (fxreg));
    }
    else
    {
        __asm__ __volatile__(
            "frstor %0"
            :: "m" (freg));
    }
}

void Xreg::
dump()
{
    if (Core::fxsr)
    {
        esReport("fcw: %x\n", fxreg.fcw);
        esReport("fsw: %x\n", fxreg.fsw);
        esReport("ftw: %x\n", fxreg.ftw);
        esReport("fop: %x\n", fxreg.fop);
        esReport("ip: %x\n", fxreg.ip);
        esReport("cs: %x\n", fxreg.cs);
        esReport("r1: %x\n", fxreg.r1);
        esReport("dp: %x\n", fxreg.dp);
        esReport("ds: %x\n", fxreg.ds);
        esReport("r2: %x\n", fxreg.r2);
        esReport("mxcsr: %x\n", fxreg.mxcsr);
        esReport("mxcsr_mask: %x\n", fxreg.mxcsr_mask);
    } else {
        esReport("fcw: %x\n", freg.fcw);
        esReport("r1: %x\n", freg.r1);
        esReport("fsw: %x\n", freg.fsw);
        esReport("r2: %x\n", freg.r2);
        esReport("ftw: %x\n", freg.ftw);
        esReport("r3: %x\n", freg.r3);
        esReport("ip: %x\n", freg.ip);
        esReport("cs: %x\n", freg.cs);
        esReport("fop: %x\n", freg.fop);
        esReport("dp: %x\n", freg.dp);
        esReport("ds: %x\n", freg.ds);
        esReport("r5: %x\n", freg.r5);
    }
}
