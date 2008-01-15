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

#ifndef NINTENDO_ES_BROKER_H_INCLUDED
#define NINTENDO_ES_BROKER_H_INCLUDED

#include <stdarg.h>

/**
 * This template class provides methods for invoking the method of the alternative interface according to the vptr table.
 * @param broker the function to pass the information about the method and the interface
 * to another function.
 * @param maxInterface the maximum number of interfaces.
 */
template<long long (*broker)(void* self, void* base, int m, va_list ap), unsigned maxInterface>
class Broker
{
    typedef long long (*Method)(void* self, ...);

    static Method* ptbl[maxInterface];
    static Method vtbl[64];

    static long long method0(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 0, ap);
        va_end(ap);
        return rc;
    }

    static long long method1(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 1, ap);
        va_end(ap);
        return rc;
    }

    static long long method2(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 2, ap);
        va_end(ap);
        return rc;
    }

    static long long method3(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 3, ap);
        va_end(ap);
        return rc;
    }

    static long long method4(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 4, ap);
        va_end(ap);
        return rc;
    }

    static long long method5(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 5, ap);
        va_end(ap);
        return rc;
    }

    static long long method6(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 6, ap);
        va_end(ap);
        return rc;
    }

    static long long method7(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 7, ap);
        va_end(ap);
        return rc;
    }

    static long long method8(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 8, ap);
        va_end(ap);
        return rc;
    }

    static long long method9(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 9, ap);
        va_end(ap);
        return rc;
    }

    static long long method10(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 10, ap);
        va_end(ap);
        return rc;
    }

    static long long method11(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 11, ap);
        va_end(ap);
        return rc;
    }

    static long long method12(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 12, ap);
        va_end(ap);
        return rc;
    }

    static long long method13(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 13, ap);
        va_end(ap);
        return rc;
    }

    static long long method14(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 14, ap);
        va_end(ap);
        return rc;
    }

    static long long method15(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 15, ap);
        va_end(ap);
        return rc;
    }

    static long long method16(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 16, ap);
        va_end(ap);
        return rc;
    }

    static long long method17(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 17, ap);
        va_end(ap);
        return rc;
    }

    static long long method18(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 18, ap);
        va_end(ap);
        return rc;
    }

    static long long method19(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 19, ap);
        va_end(ap);
        return rc;
    }

    static long long method20(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 20, ap);
        va_end(ap);
        return rc;
    }

    static long long method21(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 21, ap);
        va_end(ap);
        return rc;
    }

    static long long method22(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 22, ap);
        va_end(ap);
        return rc;
    }

    static long long method23(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 23, ap);
        va_end(ap);
        return rc;
    }

    static long long method24(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 24, ap);
        va_end(ap);
        return rc;
    }

    static long long method25(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 25, ap);
        va_end(ap);
        return rc;
    }

    static long long method26(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 26, ap);
        va_end(ap);
        return rc;
    }

    static long long method27(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 27, ap);
        va_end(ap);
        return rc;
    }

    static long long method28(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 28, ap);
        va_end(ap);
        return rc;
    }

    static long long method29(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 29, ap);
        va_end(ap);
        return rc;
    }

    static long long method30(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 30, ap);
        va_end(ap);
        return rc;
    }

    static long long method31(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 31, ap);
        va_end(ap);
        return rc;
    }

    static long long method32(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 32, ap);
        va_end(ap);
        return rc;
    }

    static long long method33(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 33, ap);
        va_end(ap);
        return rc;
    }

    static long long method34(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 34, ap);
        va_end(ap);
        return rc;
    }

    static long long method35(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 35, ap);
        va_end(ap);
        return rc;
    }

    static long long method36(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 36, ap);
        va_end(ap);
        return rc;
    }

    static long long method37(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 37, ap);
        va_end(ap);
        return rc;
    }

    static long long method38(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 38, ap);
        va_end(ap);
        return rc;
    }

    static long long method39(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 39, ap);
        va_end(ap);
        return rc;
    }

    static long long method40(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 40, ap);
        va_end(ap);
        return rc;
    }

    static long long method41(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 41, ap);
        va_end(ap);
        return rc;
    }

    static long long method42(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 42, ap);
        va_end(ap);
        return rc;
    }

    static long long method43(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 43, ap);
        va_end(ap);
        return rc;
    }

    static long long method44(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 44, ap);
        va_end(ap);
        return rc;
    }

    static long long method45(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 45, ap);
        va_end(ap);
        return rc;
    }

    static long long method46(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 46, ap);
        va_end(ap);
        return rc;
    }

    static long long method47(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 47, ap);
        va_end(ap);
        return rc;
    }

    static long long method48(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 48, ap);
        va_end(ap);
        return rc;
    }

    static long long method49(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 49, ap);
        va_end(ap);
        return rc;
    }

    static long long method50(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 50, ap);
        va_end(ap);
        return rc;
    }

    static long long method51(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 51, ap);
        va_end(ap);
        return rc;
    }

    static long long method52(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 52, ap);
        va_end(ap);
        return rc;
    }

    static long long method53(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 53, ap);
        va_end(ap);
        return rc;
    }

    static long long method54(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 54, ap);
        va_end(ap);
        return rc;
    }

    static long long method55(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 55, ap);
        va_end(ap);
        return rc;
    }

    static long long method56(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 56, ap);
        va_end(ap);
        return rc;
    }

    static long long method57(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 57, ap);
        va_end(ap);
        return rc;
    }

    static long long method58(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 58, ap);
        va_end(ap);
        return rc;
    }

    static long long method59(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 59, ap);
        va_end(ap);
        return rc;
    }

    static long long method60(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 60, ap);
        va_end(ap);
        return rc;
    }

    static long long method61(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 61, ap);
        va_end(ap);
        return rc;
    }

    static long long method62(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 62, ap);
        va_end(ap);
        return rc;
    }

    static long long method63(void* self, ...)
    {
        va_list ap;

        va_start(ap, self);
        long long rc = broker(self, ptbl, 63, ap);
        va_end(ap);
        return rc;
    }

public:

    /**
     * Gets the vptr table which contains objects.
     * Each object only has a vptr pointing to the vtbl.
     * @return the vptr table.
     */
    static void** getInterfaceTable()
    {
        // Note ptbl cannot be initialized by the constructor as
        // system call interface is used before global constructors
        // are called.
        if (!ptbl[0])
        {
            for (Method** p(ptbl); p < &ptbl[maxInterface]; ++p)
            {
                *p = vtbl;
            }
        }
        return reinterpret_cast<void**>(ptbl);
    }

    static int getInterfaceNo(void* interfacePointer)
    {
        Method** p = reinterpret_cast<Method**>(interfacePointer);
        if (ptbl <= p && p < ptbl + maxInterface)
        {
            return p - ptbl;
        }
        else
        {
            return -1;  // Not a member
        }
    }
};

template<long long (*broker)(void*, void*, int, va_list), unsigned maxInterface>
long long (**Broker<broker, maxInterface>::ptbl[maxInterface])(void* self, ...);

template<long long (*broker)(void*, void*, int, va_list), unsigned maxInterface>
long long (*Broker<broker, maxInterface>::vtbl[64])(void* self, ...) =
{
    method0,
    method1,
    method2,
    method3,
    method4,
    method5,
    method6,
    method7,
    method8,
    method9,
    method10,
    method11,
    method12,
    method13,
    method14,
    method15,
    method16,
    method17,
    method18,
    method19,
    method20,
    method21,
    method22,
    method23,
    method24,
    method25,
    method26,
    method27,
    method28,
    method29,
    method30,
    method31,
    method32,
    method33,
    method34,
    method35,
    method36,
    method37,
    method38,
    method39,
    method40,
    method41,
    method42,
    method43,
    method44,
    method45,
    method46,
    method47,
    method48,
    method49,
    method50,
    method51,
    method52,
    method53,
    method54,
    method55,
    method56,
    method57,
    method58,
    method59,
    method60,
    method61,
    method62,
    method63
};

#endif  // NINTENDO_ES_BROKER_H_INCLUDED
