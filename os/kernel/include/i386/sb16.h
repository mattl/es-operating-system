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

#ifndef NINTENDO_ES_KERNEL_I386_SB16_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_SB16_H_INCLUDED

#include <es/ref.h>
#include <es/synchronized.h>
#include <es/base/ICallback.h>
#include <es/base/IMonitor.h>
#include <es/device/IDmac.h>
#include "8237a.h"
#include "line.h"

class SoundBlaster16 : public es::Callback
{
    // DSP I/O Addresses
    static const u8 RESET = 0x6;
    static const u8 READ = 0xA;
    static const u8 WRITE = 0xC;
    static const u8 WRITE_STATUS = 0xC;
    static const u8 READ_STATUS = 0xE;
    static const u8 READ_STATUS_16 = 0xF;
    static const u8 MIXER_ADDR = 0x4;
    static const u8 MIXER_DATA = 0x5;

    // Mixer Registers (CT1745 Mixer)
    static const u8 MIXER_RESET = 0x00;
    static const u8 MIXER_MASTER_L = 0x30;
    static const u8 MIXER_MASTER_R = 0x31;
    static const u8 MIXER_VOICE_L = 0x32;
    static const u8 MIXER_VOICE_R = 0x33;
    static const u8 MIXER_MIDI_L = 0x34;
    static const u8 MIXER_MIDI_R = 0x35;
    static const u8 MIXER_CD_L = 0x36;
    static const u8 MIXER_CD_R = 0x37;
    static const u8 MIXER_LINE_L = 0x38;
    static const u8 MIXER_LINE_R = 0x39;
    static const u8 MIXER_MIC = 0x3a;
    static const u8 MIXER_PC_SPEAKER = 0x3b;
    static const u8 MIXER_OUTPUT_SWITCHES = 0x3c;
    static const u8 MIXER_INPUT_SWITCHES_L = 0x3d;
    static const u8 MIXER_INPUT_SWITCHES_R = 0x3e;
    static const u8 MIXER_INPUT_GAIN_L = 0x3f;
    static const u8 MIXER_INPUT_GAIN_R = 0x40;
    static const u8 MIXER_OUTPUT_GAIN_L = 0x41;
    static const u8 MIXER_OUTPUT_GAIN_R = 0x42;
    static const u8 MIXER_AGC = 0x43;
    static const u8 MIXER_TREBLE_L = 0x44;
    static const u8 MIXER_TREBLE_R = 0x45;
    static const u8 MIXER_BASS_L = 0x46;
    static const u8 MIXER_BASS_R = 0x47;
    static const u8 INTERRUPT_SETUP = 0x80;
    static const u8 DMA_SETUP = 0x81;
    static const u8 INTERRUPT_STATUS = 0x82;

    // DSP Commands
    static const u8 GET_VERSION = 0xe1;
    static const u8 SPEAKER_ON = 0xd1;
    static const u8 SPEAKER_OFF = 0xd3;
    static const u8 EXIT_AUTOINIT_DMA_8BIT = 0xda;
    static const u8 EXIT_AUTOINIT_DMA_16BIT = 0xd9;
    static const u8 PUASE_8BIT = 0xd0;
    static const u8 PUASE_16BIT = 0xd5;
    static const u8 SET_8BIT_PCM_OUTPUT = 0x1c;
    static const u8 SET_8BIT_PCM_INPUT = 0x2c;
    static const u8 SET_TRANSFER_TIME_CONSTANT = 0x40;
    static const u8 SET_OUTPUT_SAMPLING_RATE = 0x41;
    static const u8 SET_INPUT_SAMPLING_RATE = 0x42;
    static const u8 SET_BLOCK_TRANSFER_SIZE = 0x48;
    static const u8 SET_MODE_8BIT_INPUT = 0xce;
    static const u8 SET_MODE_8BIT_OUTPUT = 0xc6;
    static const u8 SET_MODE_16BIT_INPUT = 0xbe;
    static const u8 SET_MODE_16BIT_OUTPUT = 0xb6;

    // DSP Modes
    static const u8 MODE_8BIT_MONO = 0x00;
    static const u8 MODE_8BIT_STEREO = 0x20;
    static const u8 MODE_16BIT_MONO = 0x10;
    static const u8 MODE_16BIT_STEREO = 0x30;

    Ref             ref;
    u16             base;
    u16             mpu401;     // base address for MPU-401
    u8              irq;
    u8              chan8;
    u8              chan16;
    es::Dmac*          dmac8;      // for 8-bit transfer
    es::Dmac*          dmac16;     // for 16-bit transfer
    bool            odd8;       // double buffer indicator
    bool            odd16;      // double buffer indicator
    Line*           line8;
    Line*           line16;

    u8              major;
    u8              minor;

    u8 readData();
    void writeData(u8 val);

    u8 readMixer(u8 addr);
    u8 writeMixer(u8 addr, u8 val);

    void start(Line* line);
    void stop(Line* line);

    static u8* const dmaBuffer8;
    static const int dmaBufferSize8 = 0x1000;
    static u8* const dmaBuffer16;
    static const int dmaBufferSize16 = 0x2000;

public:
    InputLine       inputLine;
    OutputLine      outputLine;

    SoundBlaster16(u8 bus, Dmac* master, Dmac* slave,
                   u16 base = 0x220, u8 irq = 5, u8 chan8 = 1, u8 chan16 = 5, u16 mpu401 = 0x330);
    ~SoundBlaster16();

    // ICallback
    int invoke(int);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // NINTENDO_ES_KERNEL_I386_SB16_H_INCLUDED
