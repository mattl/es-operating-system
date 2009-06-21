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

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/device/IAudioFormat.h>
#include <es/naming/IContext.h>
#include "core.h"

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
