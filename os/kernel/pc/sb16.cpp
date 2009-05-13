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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include "8237a.h"
#include "core.h"
#include "io.h"
#include "sb16.h"

u8* const SoundBlaster16::dmaBuffer8 = (u8*) 0x80021000;
u8* const SoundBlaster16::dmaBuffer16 = (u8*) 0x80022000;

u8 SoundBlaster16::
readData()
{
    for (;;)
    {
        if (inpb(base + READ_STATUS) & 0x80)
        {
            return inpb(base + READ);
        }
    }
}

void SoundBlaster16::
writeData(u8 cmd)
{
    for (;;)
    {
        if (!(inpb(base + WRITE_STATUS) & 0x80))
        {
            outpb(base + WRITE, cmd);
            return;
        }
    }
}

u8 SoundBlaster16::
readMixer(u8 addr)
{
    outpb(base + MIXER_ADDR, addr);
    return inpb(base + MIXER_DATA);
}

u8 SoundBlaster16::
writeMixer(u8 addr, u8 val)
{
    outpb(base + MIXER_ADDR, addr);
    outpb(base + MIXER_DATA, val);
    return inpb(base + MIXER_DATA);
}

SoundBlaster16::
SoundBlaster16(u8 bus, Dmac* master, Dmac* slave,
               u16 base, u8 irq, u8 chan8, u8 chan16, u16 mpu401) :
    base(base),
    mpu401(mpu401),
    irq(irq),
    chan8(chan8),
    chan16(chan16),
    odd8(false),
    odd16(false),
    line8(0),
    line16(0),
    inputLine(this),
    outputLine(this)
{
    memset(dmaBuffer8, 128, dmaBufferSize8);
    memset(dmaBuffer16, 0, dmaBufferSize16);

    // Reset DSP
    outpb(base + RESET, 1);
    esSleep(30);    // wait for 3 microseconds
    outpb(base + RESET, 0);
    esSleep(1000);  // wait for 100 microseconds

    u8 data = readData();
    if (data != 0xaa)
    {
        throw SystemException<ENODEV>();
    }

    writeData(GET_VERSION);
    major = readData();
    minor = readData();

    if (major != 4)
    {
        esReport("No SoundBlaster16 compatible sound card found. (DSP version %d.%d)\n",
                 major, minor);
        throw SystemException<ENODEV>();
    }

    //
    // initialize the mixer
    //
    writeMixer(MIXER_RESET, 0);

    writeMixer(MIXER_OUTPUT_SWITCHES, 0);
    writeMixer(MIXER_INPUT_SWITCHES_L, 0);
    writeMixer(MIXER_INPUT_SWITCHES_R, 0);

    writeMixer(MIXER_MASTER_L, 31 << 3);
    writeMixer(MIXER_MASTER_R, 31 << 3);
    writeMixer(MIXER_VOICE_L, 31 << 3);
    writeMixer(MIXER_VOICE_R, 31 << 3);
    writeMixer(MIXER_MIDI_L, 0);
    writeMixer(MIXER_MIDI_R, 0);
    writeMixer(MIXER_CD_L, 0);
    writeMixer(MIXER_CD_R, 0);
    writeMixer(MIXER_LINE_L, 0);
    writeMixer(MIXER_LINE_R, 0);
    writeMixer(MIXER_MIC, 0);
    writeMixer(MIXER_PC_SPEAKER, 0);
    writeMixer(MIXER_INPUT_GAIN_L, 0);
    writeMixer(MIXER_INPUT_GAIN_R, 0);
    writeMixer(MIXER_OUTPUT_GAIN_L, 0);
    writeMixer(MIXER_OUTPUT_GAIN_R, 0);
    writeMixer(MIXER_AGC, 0);
    writeMixer(MIXER_TREBLE_L, 8 << 4);
    writeMixer(MIXER_TREBLE_R, 8 << 4);
    writeMixer(MIXER_BASS_L, 8 << 4);
    writeMixer(MIXER_BASS_R, 8 << 4);

    //
    // Select DMA channels
    //
    u8 dma;
    dma = writeMixer(DMA_SETUP, (1u << chan8) | (1u << chan16));
    chan8 = ffs(dma & 0xb) - 1;
    chan16 = ffs(dma & 0xe0) - 1;
    if (chan8 != 0xff)
    {
        dmac8 = &slave->chan[chan8 & 3];
    }
    else
    {
        dmac8 = 0;
    }

    if (chan16 != 0xff)
    {
        dmac16 = &master->chan[chan16 & 3];
    }
    else
    {
        dmac16 = 0;
    }

    //
    // Select IRQ
    //
    u8 is;
    switch (irq)
    {
    case 2:
        is = 0x01;
        break;
    case 5:
        is = 0x02;
        break;
    case 7:
        is = 0x04;
        break;
    case 10:
        is = 0x08;
        break;
    default:
        is = readMixer(INTERRUPT_SETUP);
        break;
    }
    is = writeMixer(INTERRUPT_SETUP, is & 0x0f) & 0x0f;
    switch (is)
    {
    case 0x01:
        irq = 2;
        break;
    case 0x02:
        irq = 5;
        break;
    case 0x04:
        irq = 7;
        break;
    case 0x08:
        irq = 10;
        break;
    default:
        throw SystemException<ENODEV>();
        break;
    }
    Core::registerInterruptHandler(bus, irq, this);

    writeData(SPEAKER_ON);
    esSleep(1120000);

    esReport("SoundBlaster DSP version %d.%d - irq %d, chan8 %d chan16 %d\n", major, minor, irq, chan8, chan16);
}

SoundBlaster16::
~SoundBlaster16()
{
}

int SoundBlaster16::
invoke(int irq)
{
    switch (irq)
    {
    case 0:
        start(&inputLine);
        return 0;
    case 1:
        start(&outputLine);
        return 0;
    default:
        break;
    }

    u8* ptr;
    u8 status = readMixer(INTERRUPT_STATUS);
    if (status & 0x01)
    {
        // 8-bit DMA
        ptr = dmaBuffer8;
        if (odd8)
        {
            ptr += dmaBufferSize8 / 2;
        }
        if (line8 == &inputLine)
        {
            int count = line8->write(ptr, dmaBufferSize8 / 2);
            if (count == 0)
            {
                stop(line8);
            }
        }
        else if (line8 == &outputLine)
        {
            int count = line8->read(ptr, dmaBufferSize8 / 2);
            if (count < dmaBufferSize8 / 2)
            {
                memset(ptr + count, 128, dmaBufferSize8 / 2 - count);
            }
            if (count == 0)
            {
                stop(line8);
            }
        }
        odd8 ^= true;
        inpb(base + READ_STATUS);
    }
    if (status & 0x02)
    {
        // 16-bit DMA
        ptr = dmaBuffer16;
        if (odd16)
        {
            ptr += dmaBufferSize16 / 2;
        }
        if (line16 == &inputLine)
        {
            line16->write(ptr, dmaBufferSize16 / 2);
        }
        else if (line16 == &outputLine)
        {
            int count = line16->read(ptr, dmaBufferSize16 / 2);
            if (count < dmaBufferSize16 / 2)
            {
                memset(ptr + count, 0, dmaBufferSize16 / 2 - count);
            }
            if (count == 0)
            {
                stop(line16);
            }
        }
        odd16 ^= true;
        inpb(base + READ_STATUS_16);
    }
    if (status & 0x04)
    {
        // MPU-401
        inpb(mpu401);
    }
    return 0;
}

void SoundBlaster16::
start(Line* line)
{
    u8 channels = line->getChannels();
    if (channels != 1 && channels != 2)
    {
        return;
    }

    u8 cmd;
    u8 mode;
    if (line == &inputLine)
    {
        cmd = SET_INPUT_SAMPLING_RATE;
        mode = es::Dmac::READ | es::Dmac::AUTO_INITIALIZE;
    }
    else if (line == &outputLine)
    {
        cmd = SET_OUTPUT_SAMPLING_RATE;
        mode = es::Dmac::WRITE | es::Dmac::AUTO_INITIALIZE;
    }
    else
    {
        return;
    }

    u8 bits = line->getBitsPerSample();
    switch (bits)
    {
    case 8:
        if (line8)
        {
            return;
        }
        line8 = line;
        dmac8->setup(dmaBuffer8, dmaBufferSize8, mode);
        dmac8->start();
        break;
    case 16:
        if (line16)
        {
            return;
        }
        line16 = line;
        dmac16->setup(dmaBuffer16, dmaBufferSize16, mode);
        dmac16->start();
        break;
    default:
        return;
    }

    writeData(cmd);
    u16 rate = line->getSamplingRate();
    writeData((u8) (rate >> 8));
    writeData((u8) rate);

    u16 samples;
    switch (bits)
    {
    case 8:
        if (line == &inputLine)
        {
            cmd = SET_MODE_8BIT_INPUT;
        }
        else
        {
            cmd = SET_MODE_8BIT_OUTPUT;
        }
        if (channels == 1)
        {
            mode = MODE_8BIT_MONO;
        }
        else
        {
            mode = MODE_8BIT_STEREO;
        }
        samples = dmaBufferSize8 / 2 / channels - 1;
        break;
    case 16:
        if (line == &inputLine)
        {
            cmd = SET_MODE_16BIT_INPUT;
        }
        else
        {
            cmd = SET_MODE_16BIT_OUTPUT;
        }
        if (channels == 1)
        {
            mode = MODE_16BIT_MONO;
        }
        else
        {
            mode = MODE_16BIT_STEREO;
        }
        samples = dmaBufferSize16 / 2 / channels - 1;
        break;
    }

    writeData(cmd);
    writeData(mode);
    writeData((u8) samples);
    writeData((u8) (samples >> 8));
}

void SoundBlaster16::
stop(Line* line)
{
    u8 bits = line->getBitsPerSample();
    switch (bits)
    {
    case 8:
        writeData(EXIT_AUTOINIT_DMA_8BIT);
        writeData(PUASE_8BIT);
        dmac8->stop();
        line8 = 0;
        break;
    case 16:
        writeData(EXIT_AUTOINIT_DMA_16BIT);
        writeData(PUASE_16BIT);
        dmac16->stop();
        line16 = 0;
        break;
    }
}

Object* SoundBlaster16::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int SoundBlaster16::
addRef()
{
    return ref.addRef();
}

unsigned int SoundBlaster16::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
