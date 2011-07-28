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

#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    Object* nameSpace;
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
