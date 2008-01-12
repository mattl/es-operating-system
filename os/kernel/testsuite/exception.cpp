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

#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    IInterface* nameSpace;
    esInit(&nameSpace);

    try
    {
        throw SystemException<3>();
    }
    catch (SystemException<3> error)
    {
        TEST(error.getResult() == 3);
    }
    catch (Exception& error)
    {
        TEST(error.getResult() == 4);
    }

    try
    {
        throw SystemException<4>();
    }
    catch (SystemException<3> error)
    {
        TEST(error.getResult() == 3);
    }
    catch (Exception& error)
    {
        TEST(error.getResult() == 4);
    }

    esReport("done.\n");
}
