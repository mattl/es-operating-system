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
 * These coded instructions, statements, and computer programs contain
 * software derived from the following specifications:
 *
 *  ENSONIQ, "AudioPCI ES1370 Preliminary Specification", January, 1997.
 *  http://www.alsa-project.org/alsa/ftp/datasheets/ensoniq/es1370.ps.gz
 *
 *  ASAHIKASEI, "AK4531A Audio CODEC with 13ch Mixer & 18bit DAC", December, 1998.
 *  http://www.datasheet4u.com/html/A/K/4/AK4531A_AsahiKaseiMicrosystems.pdf.html
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include "core.h"
#include "io.h"
#include "es1370.h"

int Es1370::
setPlayBuffer(u32 address, u32 size)
{
    outpl(base + MemoryPageRegister, DacFrameInformation);
    outpl(base + Dac2PciAddressRegister, address & ~0xc0000000 /* physical address */);
    outpl(base + MemoryPageRegister, DacFrameInformation);
    outpl(base + Dac2BufferSizeRegister, (size >> 2) - 1); // set the number of longwords in a buffer minus one.
    return 0;
}

int Es1370::
setRecordBuffer(u32 address, u32 size)
{
    outpl(base + MemoryPageRegister, AdcFrameInformation);
    outpl(base + AdcPciAddressRegister, address & ~0xc0000000 /* physical address */);
    outpl(base + MemoryPageRegister, AdcFrameInformation);
    outpl(base + AdcBufferSizeRegister, (size >> 2) - 1); // set the number of longwords in a buffer minus one.
    return 0;
}

void Es1370::
setPlaybackSampleCount(u32 count)
{
    outpl(base + Dac2SampleCountRegister, count-1);
}

void Es1370::
setRecordSampleCount(u32 count)
{
    outpl(base + AdcSampleCountRegister, count-1);
}

int Es1370::
setSamplingRate(u32 rate)
{
    u32 control = inpl(base + ControlRegister);
    control &= ~ControlPclkdiv;
    control |= (1411200/rate-2) << ControlPclkdivShift;
    outpl(base + ControlRegister, control);
    return 0;
}

int Es1370::
setPlaybackFormat(u8 channels, u8 bits)
{
    ASSERT(bits == 8 | bits == 16);
    ASSERT(channels == 1 | channels == 2);

    u32 control = inpl(base + SerialControlRegister);
    control &= ~(SerialP2smb | SerialP2seb);
    if (channels == 2)
    {
        control |= SerialP2smb;
    }
    if (bits == 16)
    {
        control |= SerialP2seb;
    }
    outpl(base + SerialControlRegister, control);
    return 0;
}

int Es1370::
setRecordFormat(u8 channels, u8 bits)
{
    ASSERT(bits == 8 | bits == 16);
    ASSERT(channels == 1 | channels == 2);
    u32 control = inpl(base + SerialControlRegister);

    control &= ~(SerialR1smb | SerialR1seb);
    if (channels == 2)
    {
        control |= SerialR1smb;
    }
    if (bits == 16)
    {
        control |= SerialR1seb;
    }
    outpl(base + SerialControlRegister, control);
    return 0;
}

int Es1370::
play()
{
    ASSERT(lineDac2);
    u8 bits = lineDac2->getBitsPerSample();

    u32 control = inpl(base + SerialControlRegister);
    control &= ~(SerialP2endinc | SerialP2stinc | SerialP2loopsel | SerialP2pause | SerialP2dacsen);
    control |= SerialP2inten | ((bits/8) << SerialP2endincShift);
    outpl(base + SerialControlRegister, control);

    control = inpl(base + ControlRegister);
    control |= ControlDac2En;
    outpl(base + ControlRegister, control); // start DMA

    return 0;
}

int Es1370::
record()
{
    u32 control = inpl(base + SerialControlRegister);
    control &= ~SerialR1loopsel;
    control |= SerialR1inten;
    outpl(base + SerialControlRegister, control);

    control = inpl(base + ControlRegister);
    control |= ControlAdcEn;
    outpl(base + ControlRegister, control); // start DMA

    return 0;
}

int Es1370::
setCodec(u8 codecControlRegister, u8 value)
{
    int retry = 10;
    int ret;
    while ((ret = (inpl(base + StatusRegister) & StatusCstat)) && retry--)
    {
        esSleep(10000); // 1msec
    }

    if (ret == 0)
    {
        outpw(base + CodecWriteRegister, ((u16) codecControlRegister << 8) | value); // This register must be accessed as a word.
        codec[codecControlRegister] = value; // save
        esSleep(1000); // 100usec
        return 0;
    }

    esReport("Es1370::setCodec() : Timeout\n"); // wrong device?
    return -1;
}

void Es1370::
resetCodec()
{
    setCodec(CodecResPd, RespdNormalOperation);
    setCodec(CodecCsel, 0); // set the clocks for codec.
    esSleep(200000); // 20msec
    setCodec(CodecAdsel, 0); // select input mixer as the input source to ADC.

    // volumes.
    setCodec(CodecVolVoiceL, DefaultGainLevel);
    setCodec(CodecVolVoiceR, DefaultGainLevel);

    setCodec(CodecVolFmL, VolMute | DefaultGainLevel);
    setCodec(CodecVolFmR, VolMute | DefaultGainLevel);

    setCodec(CodecVolCdL, VolMute | DefaultGainLevel);
    setCodec(CodecVolCdR, VolMute | DefaultGainLevel);

    setCodec(CodecVolLineL, DefaultGainLevel);
    setCodec(CodecVolLineR, DefaultGainLevel);

    setCodec(CodecVolAuxL, VolMute | DefaultGainLevel);
    setCodec(CodecVolAuxR, VolMute | DefaultGainLevel);

    setCodec(CodecVolMono1, VolMute | DefaultGainLevel);
    setCodec(CodecVolMono2, VolMute | DefaultGainLevel);

    setCodec(CodecVolMic, DefaultGainLevel);
    setCodec(CodecVolMonoOut, 0); // 0dB.

    // mic
    setCodec(CodecMgain, MgainAmpOn);

    // mixers.
    setCodec(CodecOmix1, 0);
    setCodec(CodecOmix2, Omix2VoiceL | Omix2VoiceR);

    setCodec(CodecLimix1, Imix1LineL | Imix1Mic);
    setCodec(CodecRimix1, Imix1LineR | Imix1Mic);

    setCodec(CodecLimix2, MixerAllOff);
    setCodec(CodecRimix2, MixerAllOff);

    // turn on the master volume.
    setCodec(CodecVolMasterL, DefaultAttLevel);
    setCodec(CodecVolMasterR, DefaultAttLevel);
}

void Es1370::
start(Line* line)
{
    if (line == &outputLine)
    {
        if (lineDac2)
        {
            return;
        }
        lineDac2 = line;

        u8 channels = line->getChannels();
        u16 rate = line->getSamplingRate();
        u8 bits = line->getBitsPerSample();
        u8 silence = (bits == 8) ? 128 : 0;
        memset(dac2Buffer, silence, dac2BufferSize);

        setPlayBuffer((u32) dac2Buffer, dac2BufferSize);
        setPlaybackSampleCount(dac2BufferSize / 2 / channels / (bits / 8));
        setSamplingRate(rate);
        setPlaybackFormat(channels, bits);

        play();
    }
    else if (line == &inputLine)
    {
        if (lineAdc)
        {
            return;
        }
        lineAdc = line;

        u8 channels = line->getChannels();
        u16 rate = line->getSamplingRate();
        u8 bits = line->getBitsPerSample();
        u8 silence = (bits == 8) ? 128 : 0;

        setRecordBuffer((u32) adcBuffer, adcBufferSize);
        setRecordSampleCount(adcBufferSize / 2 / channels / (bits / 8));
        setSamplingRate(rate);
        setRecordFormat(channels, bits);

        record();
    }
}

void Es1370::
stop(Line* line)
{
    if (line == &outputLine && lineDac2 == &outputLine)
    {
        // stop playback.
        u32 control = inpl(base + SerialControlRegister);
        control |= SerialP2pause;
        outpl(base + SerialControlRegister, control);

        control = inpl(base + ControlRegister);
        control &= ~ControlDac2En;
        outpl(base + ControlRegister, control);
        lineDac2 = 0;
    }
    else if (line == &inputLine && lineAdc == &inputLine)
    {
        // stop recording.
        u32 control = inpl(base + ControlRegister);
        control &= ~ControlAdcEn;
        outpl(base + ControlRegister, control);
        lineAdc = 0;
    }
}

int Es1370::
writeSamplesToPlaybackBuffer()
{
    u8* ptr = dac2Buffer;
    if (oddDac2)
    {
        ptr += dac2BufferSize / 2;
    }

    int count = lineDac2->read(ptr, dac2BufferSize / 2);
    if (count == 0)
    {
        stop(lineDac2);
    }
    else
    {
        u8 bits = lineDac2->getBitsPerSample();
        if (count < dac2BufferSize / 2)
        {
            u8 silence = (bits == 8) ? 128 : 0;
            memset(ptr + count, silence, dac2BufferSize / 2 - count);
        }
        oddDac2 ^= true; // select the other buffer.
    }

    return 0;
}

int Es1370::
readSamplesFromRecordBuffer()
{
    u8* ptr = adcBuffer;
    if (oddAdc)
    {
        ptr += adcBufferSize / 2;
    }

    int count = lineAdc->write(ptr, adcBufferSize / 2);
    if (count == 0)
    {
        stop(lineAdc);
    }
    else
    {
        u8 bits = lineAdc->getBitsPerSample();
        if (count < adcBufferSize / 2)
        {
            u8 silence = (bits == 8) ? 128 : 0;
            memset(ptr + count, silence, adcBufferSize / 2 - count);
        }
        oddAdc^= true; // select the other buffer.
    }

    return 0;
}

Es1370::
Es1370(u8 bus, u16 base, u8 irq) :
    base(base),
    irq(irq),
    oddDac2(false),
    oddAdc(false),
    lineDac2(0),
    lineAdc(0),
    dac2Buffer(0),
    adcBuffer(0),
    dac2BufferSize(0),
    adcBufferSize(0),
    inputLine(this),
    outputLine(this)
{
    // DMA buffer for playback.
    dac2Buffer = new u8[BufferSize];
    ASSERT(dac2Buffer);
    dac2BufferSize = BufferSize;

    // DMA buffer for recording.
    adcBuffer = new u8[BufferSize];
    ASSERT(adcBuffer);
    adcBufferSize = BufferSize;

    u32 control = ControlCdcEn | ControlSerrDis;
    outpl(base + ControlRegister, control);
    outpl(base + SerialControlRegister, 0);

    setSamplingRate(DefaultSamplingRate); // This code is necessary for a real device to call setCodec().
    Core::registerInterruptHandler(bus, irq, this);
    resetCodec();
}

Es1370::
~Es1370()
{
    if (dac2Buffer)
    {
        delete [] dac2Buffer;
    }
    if (adcBuffer)
    {
        delete [] adcBuffer;
    }
}

int Es1370::
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

    u32 status = inpl(base + StatusRegister);
    if ((status & StatusIntr) == 0)
    {
        return 0;
    }

    if (status & StatusDac2)
    {
        u32 control = inpl(base + SerialControlRegister);
        control &= ~SerialP2inten;
        outpl(base + SerialControlRegister, control);
        if (lineDac2 == &outputLine)
        {
            writeSamplesToPlaybackBuffer();
            control |= SerialP2inten;
            outpl(base + SerialControlRegister, control);
        }
    }

    if (status & StatusAdc)
    {
        u32 control = inpl(base + SerialControlRegister);
        control &= ~SerialR1inten;
        outpl(base + SerialControlRegister, control);
        if (lineAdc == &inputLine)
        {
            readSamplesFromRecordBuffer();
            control |= SerialR1inten;
            outpl(base + SerialControlRegister, control);
        }
    }

    // just clear flags.
    if (status & StatusDac1)
    {
        u32 control = inpl(base + SerialControlRegister);
        control &= ~SerialP1inten;
        outpl(base + SerialControlRegister, control);
    }
    if (status & StatusMccb)
    {
        u32 control = inpl(base + ControlRegister);
        control &= ~ControlCcbIntrm;
        outpl(base + ControlRegister, control);
    }

    return 0;
}

Object* Es1370::
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

unsigned int Es1370::
addRef()
{
    return ref.addRef();
}

unsigned int Es1370::
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
