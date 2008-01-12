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

#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "8042.h"

int main()
{
    IInterface* nameSpace;
    esInit(&nameSpace);

    Handle<IContext> root(nameSpace);
    Handle<IStream> keyboard(root->lookup("device/keyboard"));
    Handle<IStream> mouse(root->lookup("device/mouse"));

    esReport("done.\n");    // for testing

    for (;;)
    {
        u8 buffer[8];
        long count;
        int i;

        count = keyboard->read(buffer, 8);
        if (1 < count)
        {
            esReport("kbd:");
            for (i = 0; i < count; ++i)
            {
                esReport(" %02x", buffer[i]);
            }
            esReport("\n");
        }

        count = mouse->read(buffer, 8);
        if (4 <= count && (buffer[0] || buffer[1] || buffer[2] || buffer[3]))
        {
            esReport("aux:");
            for (i = 0; i < count; ++i)
            {
                esReport(" %02x", buffer[i]);
            }
            esReport("\n");
        }

        esSleep(10000000 / 60);
    }

    esPanic(__FILE__, __LINE__, "done.\n");
}
