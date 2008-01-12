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
#include <es/device/IBeep.h>
#include "core.h"

unsigned notes[] =
{
    262,
    294,
    330,
    349,
    392,
    440,
    494,
    523
};

int main()
{
    IInterface* nameSpace;
    esInit(&nameSpace);

    Handle<IContext> root(nameSpace);
    Handle<IBeep> speaker = root->lookup("device/beep");

    for (int i = 0; i < 8; ++i)
    {
        speaker->setFrequency(notes[i]);
        speaker->beep();
        esSleep(10000000);
    }

    esReport("done.\n");
}
