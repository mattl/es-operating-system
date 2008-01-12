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

#include <string.h>
#include <es.h>
#include <es/clsid.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/device/IAudioFormat.h>
#include <es/naming/IContext.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

extern u8 sample[130792];
extern unsigned char sample8[33210];

int main()
{
    IInterface* nameSpace = 0;
    esInit(&nameSpace);

    Handle<IContext> root(nameSpace);
    Handle<IStream> soundOutput = root->lookup("device/soundOutput");
    TEST(soundOutput);
    Handle<IAudioFormat> format(soundOutput);
    TEST(format);

    long len;
    long offset;
    long n;

    len = sizeof sample8;
    esReport("8-bit sample %d byte\n", len);
    format->setBitsPerSample(8);
    format->setChannels(1);
    format->setSamplingRate(11025);
    TEST(format->getBitsPerSample() == 8);
    TEST(format->getChannels() == 1);
    TEST(format->getSamplingRate() == 11025);
    for (offset = 0; offset < len; offset += n)
    {
        n = soundOutput->write(sample8 + offset, len - offset);
        if (n <= 0)
        {
            break;
        }
    }
    esSleep(20000000);

    len = sizeof sample;
    esReport("16-bit sample %d byte\n", len);
    format->setBitsPerSample(16);
    format->setChannels(2);
    format->setSamplingRate(22050);
    TEST(format->getBitsPerSample() == 16);
    TEST(format->getChannels() == 2);
    TEST(format->getSamplingRate() == 22050);
    for (offset = 0; offset < len; offset += n)
    {
        n = soundOutput->write(sample + offset, len - offset);
        if (n <= 0)
        {
            break;
        }
    }
    esSleep(20000000);

    esReport("done.\n");
}
