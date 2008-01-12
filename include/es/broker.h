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
    static Method vtbl[33];

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
};

template<long long (*broker)(void*, void*, int, va_list), unsigned maxInterface>
long long (**Broker<broker, maxInterface>::ptbl[maxInterface])(void* self, ...);

template<long long (*broker)(void*, void*, int, va_list), unsigned maxInterface>
long long (*Broker<broker, maxInterface>::vtbl[33])(void* self, ...) =
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
    method32
};

#endif  // NINTENDO_ES_BROKER_H_INCLUDED
