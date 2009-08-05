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

#ifndef NINTENDO_ES_KERNEL_I386_ES1370_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_ES1370_H_INCLUDED

#include <es/ref.h>
#include <es/synchronized.h>
#include <es/base/ICallback.h>
#include <es/base/IMonitor.h>
#include "line.h"

class Es1370 : public es::Callback
{
    Ref   ref;
    u16   base;
    u8    irq;
    bool  oddDac2;     // double buffer indicator.
    bool  oddAdc;      // double buffer indicator.
    Line* lineDac2;
    Line* lineAdc;

    u8*   dac2Buffer;  // DMA buffer for playback.
    u8*   adcBuffer;   // DMA buffer for recording.
    int   dac2BufferSize;
    int   adcBufferSize;
    u8    codec[26];

    int setPlayBuffer(u32 address, u32 size);
    int setRecordBuffer(u32 address, u32 size);
    void setPlaybackSampleCount(u32 count);
    void setRecordSampleCount(u32 count);
    int setSamplingRate(u32 rate);
    int setPlaybackFormat(u8 channels, u8 bits);
    int setRecordFormat(u8 channels, u8 bits);
    int play();
    int record();
    int setCodec(u8 codecControlRegister, u8 value);
    void resetCodec();
    void start(Line* line);
    void stop(Line* line);
    int writeSamplesToPlaybackBuffer();
    int readSamplesFromRecordBuffer();

    static const int BufferSize = 4096 * 2;
    static const u32 DefaultSamplingRate = 11025;

public:
    InputLine       inputLine;
    OutputLine      outputLine;

    Es1370(u8 bus, u16 base, u8 irq);
    ~Es1370();

    // ICallback
    int invoke(int);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

private:
    // Control registers in AudioPCI.
    static const u8 ControlRegister         = 0x00; // Interrupt/Chip Select Control Register
    static const u8 StatusRegister          = 0x04; // Interrupt/Chip Select Status Register
    static const u8 UartDataRegister        = 0x08;
    static const u8 UartStatusRegister      = 0x09;
    static const u8 UartControlRegister     = 0x09;
    static const u8 UartReservedRegister    = 0x0a;
    static const u8 MemoryPageRegister      = 0x0c; // Memory Page Register
    static const u8 CodecWriteRegister      = 0x10;
    static const u8 SerialControlRegister   = 0x20; // Serial Interface Control Register
    static const u8 Dac1SampleCountRegister = 0x24;
    static const u8 Dac2SampleCountRegister = 0x28;
    static const u8 AdcSampleCountRegister  = 0x2c;
    static const u8 Dac1PciAddressRegister  = 0x30;
    static const u8 Dac1BufferSizeRegister  = 0x34;
    static const u8 Dac2PciAddressRegister  = 0x38;
    static const u8 Dac2BufferSizeRegister  = 0x3c;
    static const u8 AdcPciAddressRegister   = 0x30;
    static const u8 AdcBufferSizeRegister   = 0x34;

    // Interrupt/Chip Select Control Register
    static const u32 ControlAdcStop         = (1<<31);
    static const u32 ControlXctl1           = (1<<30);
    static const u32 ControlOpen            = (1<<30);    // not used.
    static const u32 ControlPclkdiv         = 0x1fff0000; // mask
    static const u32 ControlPclkdivShift    = 16;
    static const u32 ControlMsfmtsel        = (1<<15);
    static const u32 ControlMsbb            = (1<<14);
    static const u32 ControlWtsrsel         = 0x00003000; // mask
    static const u32 ControlWtsrselShift    = 12;
    static const u32 ControlDacSync         = (1<<11);
    static const u32 ControlCcbIntrm        = (1<<10);
    static const u32 ControlMcb             = (1<<9);
    static const u32 ControlXctl0           = (1<<8);
    static const u32 ControlBreq            = (1<<7);
    static const u32 ControlDac1En          = (1<<6);
    static const u32 ControlDac2En          = (1<<5);
    static const u32 ControlAdcEn           = (1<<4);
    static const u32 ControlUartEn          = (1<<3);
    static const u32 ControlJystkEn         = (1<<2);
    static const u32 ControlCdcEn           = (1<<1);
    static const u32 ControlSerrDis         = (1<<0);

    // Interrupt/Chip Select Status Register
    static const u32 StatusIntr             = (1<<31);
    static const u32 StatusCstat            = (1<<10);
    static const u32 StatusCbusy            = (1<<9);
    static const u32 StatusCwrip            = (1<<8);
    static const u32 StatusVc               = 0x00000060; // mask
    static const u32 StatusVcShift          = 5;
    static const u32 StatusMccb             = (1<<4);
    static const u32 StatusUart             = (1<<3);
    static const u32 StatusDac1             = (1<<2);
    static const u32 StatusDac2             = (1<<1);
    static const u32 StatusAdc              = (1<<0);

    // Serial Interface Control Register
    static const u32 SerialP2endinc         = 0x00380000; // mask
    static const u32 SerialP2endincShift    = 19;
    static const u32 SerialP2stinc          = 0x00070000; // mask
    static const u32 SerialP2stincShift     = 16;
    static const u32 SerialR1loopsel        = (1<<15);
    static const u32 SerialP2loopsel        = (1<<14);
    static const u32 SerialP1loopsel        = (1<<13);
    static const u32 SerialP2pause          = (1<<12);
    static const u32 SerialP1pause          = (1<<11);
    static const u32 SerialR1inten          = (1<<10);
    static const u32 SerialP2inten          = (1<<9);
    static const u32 SerialP1inten          = (1<<8);
    static const u32 SerialP1sctrld         = (1<<7);
    static const u32 SerialP2dacsen         = (1<<6);
    static const u32 SerialR1seb            = (1<<5);
    static const u32 SerialR1smb            = (1<<4);
    static const u32 SerialP2seb            = (1<<3);
    static const u32 SerialP2smb            = (1<<2);
    static const u32 SerialP1seb            = (1<<1);
    static const u32 SerialP1smb            = (1<<0);

    // Memory Page Register
    static const u16 DacFrameInformation    = 0x0c; // These bits are set to memory page register
    static const u16 AdcFrameInformation    = 0x0d; // to select what memory page will be accessed.
    static const u16 UartFifo1              = 0x0e;
    static const u16 UartFifo2              = 0x0f;

    // CODEC register addresses.
    static const u8 CodecVolMasterL         = 0x00;
    static const u8 CodecVolMasterR         = 0x01;
    static const u8 CodecVolVoiceL          = 0x02;
    static const u8 CodecVolVoiceR          = 0x03;
    static const u8 CodecVolFmL             = 0x04;
    static const u8 CodecVolFmR             = 0x05;
    static const u8 CodecVolCdL             = 0x06;
    static const u8 CodecVolCdR             = 0x07;
    static const u8 CodecVolLineL           = 0x08;
    static const u8 CodecVolLineR           = 0x09;
    static const u8 CodecVolAuxL            = 0x0A;
    static const u8 CodecVolAuxR            = 0x0B;
    static const u8 CodecVolMono1           = 0x0C;
    static const u8 CodecVolMono2           = 0x0D;
    static const u8 CodecVolMic             = 0x0E;
    static const u8 CodecVolMonoOut         = 0x0F;
    static const u8 CodecOmix1              = 0x10;
    static const u8 CodecOmix2              = 0x11;
    static const u8 CodecLimix1             = 0x12;
    static const u8 CodecRimix1             = 0x13;
    static const u8 CodecLimix2             = 0x14;
    static const u8 CodecRimix2             = 0x15;
    static const u8 CodecResPd              = 0x16;
    static const u8 CodecCsel               = 0x17;
    static const u8 CodecAdsel              = 0x18;
    static const u8 CodecMgain              = 0x19;

    // CODEC control bits
    static const u8 VolMute                 = (1<<7);
    static const u8 VolAtt4                 = (1<<4); // attenuation level
    static const u8 VolAtt3                 = (1<<3); // 32 levels with 2dB step
    static const u8 VolAtt2                 = (1<<2); // 00000:   0dB
    static const u8 VolAtt1                 = (1<<1); // 11111: -62dB
    static const u8 VolAtt0                 = (1<<0);

    static const u8 VolGai4                 = (1<<4); // gain level
    static const u8 VolGai3                 = (1<<3); // 32 levels with 2dB step
    static const u8 VolGai2                 = (1<<2); // 00000: +12dB
    static const u8 VolGai1                 = (1<<1); // 00110:   0dB
    static const u8 VolGai0                 = (1<<0); // 11111: -50dB

    static const u8 Omix1FmL                = (1<<6); // ON/OFF of Mixer switches.
    static const u8 Omix1FmR                = (1<<5); // 0: OFF, 1: ON.
    static const u8 Omix1LineL              = (1<<4);
    static const u8 Omix1LineR              = (1<<3);
    static const u8 Omix1CdL                = (1<<2);
    static const u8 Omix1CdR                = (1<<1);
    static const u8 Omix1Mic                = (1<<0);

    static const u8 Omix2AuxL               = (1<<5);
    static const u8 Omix2AuxR               = (1<<4);
    static const u8 Omix2VoiceL             = (1<<3);
    static const u8 Omix2VoiceR             = (1<<2);
    static const u8 Omix2Mono2              = (1<<1);
    static const u8 Omix2Mono1              = (1<<0);

    static const u8 Imix1FmL                = (1<<6);
    static const u8 Imix1FmR                = (1<<5);
    static const u8 Imix1LineL              = (1<<4);
    static const u8 Imix1LineR              = (1<<3);
    static const u8 Imix1CdL                = (1<<2);
    static const u8 Imix1CdR                = (1<<1);
    static const u8 Imix1Mic                = (1<<0);

    static const u8 Imix2Tmic               = (1<<7);
    static const u8 Imix2Tmono1             = (1<<6);
    static const u8 Imix2Tmono2             = (1<<5);
    static const u8 Imix2AuxL               = (1<<4);
    static const u8 Imix2AuxR               = (1<<3);
    static const u8 Imix2Voice              = (1<<2);
    static const u8 Imix2Mono2              = (1<<1);
    static const u8 Imix2Mono1              = (1<<0);

    static const u8 RespdPd                 = (1<<1); // Enables the power down. (1: Normal Operation, 0: Power down)
    static const u8 RespdRst                = (1<<0); // initializes the contents of all registers. (1: Normal Operation, 0: Initialize)

    static const u8 Csel2                   = (1<<1); // Select the clocks for codec.
    static const u8 Csel1                   = (1<<0);

    static const u8 AdselAdcSource          = (1<<0); // Selects the input source to ADC.
                                                      // (0: output from input mixer, 1: AINL/AINR inputs)
    static const u8 MgainAmpOn              = (1<<0); // Selects the gain of MIC amp. (0: 0dB, 1: 30dB)

    // alias
    static const u8 DefaultAttLevel         = 0;      // default attenuation level (0dB).
    static const u8 DefaultGainLevel        = (VolGai2 | VolGai1); // 0dB.
    static const u8 MixerAllOff             = 0;
    static const u8 RespdNormalOperation    = (RespdPd | RespdRst);
};

#endif // NINTENDO_ES_KERNEL_I386_ES1370_H_INCLUDED
