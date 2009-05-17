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

/*
 * Note: use "-soundhw es1370" for QEMU.
 */
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/device/IAudioFormat.h>
#include <es/naming/IContext.h>
#include "core.h"
#include "es1370.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

extern u8 sample[130792];
extern unsigned char sample8[33210];

int main()
{
    Object* nameSpace = 0;
    esInit(&nameSpace);

    Handle<es::Context> root(nameSpace);
    Handle<es::Stream> soundOutput = root->lookup("device/soundOutput");
    TEST(soundOutput);

    Handle<es::AudioFormat> format(soundOutput);
    TEST(format);

    long len;
    long offset;
    long n;

#if 0 // playback
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
#endif // playback

#if 0 // recording.

    Handle<es::Stream> soundInput = root->lookup("device/soundInput");
    TEST(soundInput);

    Handle<es::AudioFormat> recFormat(soundInput);
    TEST(recFormat);

    u32 recSize = 128 * 1024;
    u8* recBuf = new u8[recSize];
    ASSERT(recBuf);
    len = recSize;

    esReport("8-bit monaural recording\n");
    recFormat->setBitsPerSample(8);
    recFormat->setChannels(1);
    recFormat->setSamplingRate(11025);
    TEST(recFormat->getBitsPerSample() == 8);
    TEST(recFormat->getChannels() == 1);
    TEST(recFormat->getSamplingRate() == 11025);

    esReport("0%% +");
    int i;
    for (i = 0; i < recSize / 4096 - 2; ++i)
    {
        esReport("-");
    }
    esReport("+ 100%% \n");
    esReport("   ");
    int counter = 0;
    for (offset = 0; offset < len; offset += n)
    {
        n = soundInput->read(recBuf + offset, len - offset);
        if (n <= 0)
        {
            break;
        }
        counter += n;
        while (0 < counter / 4096)
        {
            esReport("*");
            counter -= 4096;
        }
    }
    esReport("\n");

    esReport("replay...\n");
    esSleep(20000000);

    len = recSize;
    format->setBitsPerSample(recFormat->getBitsPerSample());
    format->setChannels(recFormat->getChannels());
    format->setSamplingRate(recFormat->getSamplingRate());

    for (offset = 0; offset < recSize; offset += n)
    {
        n = soundOutput->write(recBuf + offset, recSize - offset);
        if (n <= 0)
        {
            break;
        }
    }
    esSleep(20000000);

#endif // recording.

#if 1 // recording and playback, simultaneously.

    Handle<es::Stream> soundInput = root->lookup("device/soundInput");
    TEST(soundInput);

    Handle<es::AudioFormat> recFormat(soundInput);
    TEST(recFormat);

    u32 recSize = 128 * 1024;
    u8* recBuf = new u8[recSize];
    ASSERT(recBuf);
    len = recSize;

    esReport("8-bit monaural recording\n");
    recFormat->setBitsPerSample(8);
    recFormat->setChannels(1);
    recFormat->setSamplingRate(11025);
    TEST(recFormat->getBitsPerSample() == 8);
    TEST(recFormat->getChannels() == 1);
    TEST(recFormat->getSamplingRate() == 11025);

    format->setBitsPerSample(recFormat->getBitsPerSample());
    format->setChannels(recFormat->getChannels());
    format->setSamplingRate(recFormat->getSamplingRate());

    esReport("0%% +");
    int i;
    for (i = 0; i < recSize / 4096 - 2; ++i)
    {
        esReport("-");
    }
    esReport("+ 100%% \n");
    esReport("   ");

    int counter = 0;
    long m;
    for (offset = 0; offset < len; offset += n)
    {
        n = soundInput->read(recBuf + offset, len - offset);
        if (n <= 0)
        {
            break;
        }
        counter += n;
        while (0 < counter / 4096)
        {
            esReport("*");
            counter -= 4096;
        }

        m = soundOutput->write(recBuf + offset, n);
        ASSERT(n == m);
    }
    esReport("\n");
    esSleep(20000000);

#endif // recording and playback, simultaneously.

    esReport("done.\n");
}
