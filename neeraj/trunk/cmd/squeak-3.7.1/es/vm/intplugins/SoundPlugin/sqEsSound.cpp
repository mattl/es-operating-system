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
 * software derived from Squeak.
 *
 *   Squeak is distributed for use and modification subject to a liberal
 *   open source license.
 *
 *   http://www.squeak.org/SqueakLicense/
 *
 *   Unless stated to the contrary, works submitted for incorporation
 *   into or for distribution with Squeak shall be presumed subject to
 *   the same license.
 *
 *   Portions of Squeak are:
 *
 *   Copyright (c) 1996 Apple Computer, Inc.
 *   Copyright (c) 1997-2001 Walt Disney Company, and/or
 *   Copyrighted works of other contributors.
 *   All rights reserved.
 */

#include <es.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/device/IAudioFormat.h>
#include <es/synchronized.h>



es::CurrentProcess* System();

extern "C" {

#include "sq.h"
#include "SoundPlugin.h"

int synchronizedSignalSemaphoreWithIndex(int semaIndex);

}

Handle<es::Stream> gSoundOutput;
Handle<es::Stream> gSoundInput;

namespace
{
    es::Monitor* monitorPlayState;
    es::Monitor* monitorPlaySize;
    es::Monitor* monitorRecordState;
    es::Monitor* monitorRecording;
}

int soundInit(void)
{
    return (monitorPlayState && monitorPlaySize);
}

int soundShutdown(void)
{
    gSoundOutput = 0;
    gSoundInput = 0;
    return 0;   // XXX check return value
}

#define BYTES_PER_SAMPLE 2
#define MAX_RECORDING_SIZE (4096 * 2)

typedef struct
{
    int open;
    int stereo;
    int frameCount;
    int sampleRate;
    int lastFlipTime;
    int playSemaIndex;
    int bufSizeInBytes;
    int done;
    u8* buffer;
    u8* head;
    u8* tail;
    int played;
    int size; // data size in buffer
    bool playing;
} PlayStateRec;

static PlayStateRec playState =
{
    0,     // open;
    0,     // stereo;
    0,     // frameCount;
    0,     // sampleRate;
    0,     // lastFlipTime;
    0,     // playSemaIndex;
    0,     // bufSizeInBytes;
    true,  // done;
    NULL,  // buffer;
    NULL,  // head;
    NULL,  // tail;
    0,     // played
    0,     // size
    false  // playing
};

typedef struct
{
    int open;
    bool inProgress;
    int recordSemaIndex;

    int stereo;      // true if stereo or false if mono.
    int samplingRate;

    // sound device specific parameters
    int dataFormat;     // 1 (8-bit sampels) or 2 (16-bit samples)
    int bytesPerSample; // size of a sample

    // squeak specific parameter
    //  Data format for squeak is BYTES_PER_SAMPLE. (16-bit sample)
    int outputBytesPerSample; // size of a sample

    // record buffer
    u8* buffer; // ring buffer
    u8* head;
    u8* tail;
    int bufSizeInBytes;
    int recorded; // data size in ring buffer

    u8* next;
    int nextRecordSize;
    int maxRecordSize;
} RecordBufferRec;

RecordBufferRec recordState;

struct Buf
{
    u8* addr;
    int size;
};

static void PlayCallback(int result, int len);

int snd_AvailableSpace(void)
{
    Synchronized<es::Monitor*> method(monitorPlaySize);
    int available = playState.bufSizeInBytes - playState.size;

    return available;
}

static u8* PutSound(const u8* src, int size)
{
    Synchronized<es::Monitor*> method(monitorPlayState);

    u8* buf = playState.buffer;
    const u8* bufEnd = playState.buffer + playState.bufSizeInBytes;
    const u8* head = playState.head;
    u8* tail = playState.tail;

    if (head <= tail)
    {
        //  buf                   bufEnd
        //  |-------XXXXXXXX------|
        //          head    tail
        if (tail + size <= bufEnd)
        {
            //  |-------XXXXXXXXXXX-|
            memmove(tail, src, size);
            tail += size;
        }
        else
        {
            //  |XX-----XXXXXXXXXXXX|
            int len = bufEnd - tail;
            if (0 < len)
            {
                memmove(tail, src, len);
            }
            memmove(buf, src + len, size - len);
            tail = buf + (size - len);
        }
    }
    else
    {
        //  buf                   bufEnd
        //  |XXXXX------------XXXX|
        //        tail        head
        memmove(tail, src, size);
        tail += size;
    }

    {
        Synchronized<es::Monitor*> method(monitorPlaySize);
        playState.size += size;
    }
    playState.tail = tail;
    monitorPlaySize->notifyAll();
    return tail;
}

int snd_PlaySamplesFromAtLength(int frameCount, int arrayIndex, int startIndex)
{
    if (!playState.open || frameCount <= 0)
    {
        return 0;
    }

    int available = snd_AvailableSpace();

    int bytesPerFrame = 2;
    if (playState.stereo)
    {
        // stereo
        bytesPerFrame *= 2;
    }
    int size = frameCount * bytesPerFrame;

    int framesWritten;
    if (available < size)
    {
        framesWritten = available;
    }
    else
    {
        framesWritten = size;
    }

    u8* src = ((u8*) arrayIndex + (startIndex * bytesPerFrame));
    PutSound(src, size);

    return framesWritten;
}

int snd_InsertSamplesFromLeadTime(int frameCount, int srcBufPtr, int samplesOfLeadTime)
{
    esReport("snd_InsertSamplesFromLeadTime:\n");
    return 0; // dummy
}

int snd_PlaySilence(void)
{
    esReport("snd_PlaySilence:\n");

    if (!playState.open)
    {
        return 0;
    }

    return playState.bufSizeInBytes;
}

int snd_Start(int frameCount, int samplesPerSec, int stereo, int semaIndex)
{
    Synchronized<es::Monitor*> method(monitorPlayState);

    if (!gSoundOutput)
    {
        return false;
    }

    int bytesPerFrame;
    int bufferBytes;

    bytesPerFrame = stereo ? 2 * BYTES_PER_SAMPLE : BYTES_PER_SAMPLE;
    bufferBytes   = ((frameCount * bytesPerFrame) / 8) * 8;

    if (playState.open)
    {
        // still open from last time; clean up before continuing
        snd_Stop();
    }

    int chan = (stereo ? 2 : 1);

    Handle<es::AudioFormat> audio(gSoundOutput);
    audio->setBitsPerSample(16);
    audio->setChannels(chan);
    audio->setSamplingRate(samplesPerSec);

    playState.bufSizeInBytes = bufferBytes * 2;
    playState.buffer = new u8[playState.bufSizeInBytes];
    if (!playState.buffer)
    {
        return false;
    }
    memset(playState.buffer, 0, playState.bufSizeInBytes);

    playState.open           = false;  // set to true if successful
    playState.stereo         = stereo;
    playState.frameCount     = bufferBytes / bytesPerFrame;
    playState.sampleRate     = samplesPerSec;
    playState.lastFlipTime   = ioMicroMSecs();
    playState.playSemaIndex  = semaIndex;
    playState.playing        = true;
    playState.head = playState.tail = playState.buffer;
    playState.done           = false;
    playState.open = true;
    monitorPlayState->notifyAll();

    return true;
}

int snd_Stop(void)
{
    Synchronized<es::Monitor*> method(monitorPlayState);

    if (!playState.open)
    {
        return false;
    }

    playState.open = false;
    playState.done = true;
    playState.playing = false;
    delete [] playState.buffer;

    playState.played = 0;
    {
        Synchronized<es::Monitor*> method(monitorPlaySize);
        playState.size = 0;
    }
    playState.head = playState.tail = NULL;

    synchronizedSignalSemaphoreWithIndex(playState.playSemaIndex);

    return true;
}

/*** exported sound input functions ***/

int snd_SetRecordLevel(int level)
{
    return true; // dummy
}

static int SetupRecordBuf(int recorded)
{
    Synchronized<es::Monitor*> method(monitorRecordState);

    const u8* tail = recordState.tail;
    const u8* buf  = recordState.buffer;
    const u8* bufEnd = buf + recordState.bufSizeInBytes;

    if (bufEnd < tail + recorded)
    {
        recordState.tail = const_cast<u8*>(buf + ((tail + recorded) - bufEnd));
    }
    else
    {
        recordState.tail += recorded;
    }
    //  buf                   bufEnd
    //  |XXXXXXXXXXXXXXXXXXXXX|
    //                         tail
    //           |
    //           V
    //  buf                   bufEnd
    //  |XXXXXXXXXXXXXXXXXXXXX|
    //  tail
    if (recordState.tail == bufEnd)
    {
        recordState.tail = const_cast<u8*>(buf);
    }

    if (recordState.recorded == recordState.bufSizeInBytes)
    {
        // Head is the same as tail,
        // when record buffer is ful-filled.
        //
        //  buf                   bufEnd
        //  |XXXXXXX|XXXXXXXXXXXXXX|
        //          ^
        //          head = tail
        recordState.head = recordState.tail;
    }

    recordState.next = const_cast<u8*>(tail);

    if (tail + recordState.maxRecordSize <= bufEnd)
    {
        recordState.nextRecordSize = recordState.maxRecordSize;
    }
    else
    {
        recordState.nextRecordSize = (int) (bufEnd - tail);
    }

    recordState.recorded += recorded;
}

int snd_StartRecording(int desiredSamplesPerSec, int stereo, int semaIndex)
{
    Synchronized<es::Monitor*> recording(monitorRecording);

    {
        Synchronized<es::Monitor*> method(monitorRecordState);
        if (recordState.open == true)
        {
            return 0;
        }
        recordState.open = true;

        recordState.bufSizeInBytes = MAX_RECORDING_SIZE * 2;
        recordState.maxRecordSize = MAX_RECORDING_SIZE;

        recordState.buffer = new u8[recordState.bufSizeInBytes];
        if (!recordState.buffer)
        {
            recordState.open = false;
            return 0;
        }
        memset(recordState.buffer, 0x80, recordState.bufSizeInBytes);
        recordState.head = recordState.tail = recordState.buffer;
        recordState.next = recordState.head;
        recordState.nextRecordSize = 0;
        recordState.recorded = 0;
        recordState.stereo = stereo;
        recordState.dataFormat = 1; // 8-bit samples
        recordState.recordSemaIndex = semaIndex;

        if (desiredSamplesPerSec < 5000)
        {
            recordState.samplingRate = 5000;
        }
        else if (44100 < desiredSamplesPerSec)
        {
            recordState.samplingRate = 44100;
        }
        else
        {
            recordState.samplingRate = desiredSamplesPerSec;
        }

        int chan;
        if (stereo)
        {
            recordState.bytesPerSample = recordState.dataFormat * 2;
            recordState.outputBytesPerSample = BYTES_PER_SAMPLE * 2;
            chan = 2;
        }
        else
        {
            recordState.bytesPerSample = recordState.dataFormat;
            recordState.outputBytesPerSample = BYTES_PER_SAMPLE;
            chan = 1;
        }

        Handle<es::AudioFormat> audio(gSoundInput);
        audio->setBitsPerSample(8 * recordState.dataFormat);
        audio->setChannels(chan);
        audio->setSamplingRate(recordState.samplingRate);

        recordState.inProgress = true;
        monitorRecordState->notifyAll();
    }
    return true;
}

int snd_StopRecording(void)
{
    {
        Synchronized<es::Monitor*> method(monitorRecordState);
        recordState.inProgress = false;
    }

    {
        Synchronized<es::Monitor*> recording(monitorRecording);

        recordState.bufSizeInBytes = 0;
        recordState.maxRecordSize = 0;
        recordState.stereo = 0;
        recordState.bytesPerSample = 0;
        recordState.outputBytesPerSample = 0;
        recordState.dataFormat = 0;
        recordState.head = NULL;
        recordState.tail = NULL;
        recordState.buffer = NULL;
        recordState.next = NULL;
        recordState.nextRecordSize = 0;
        recordState.recorded = 0;
        recordState.recordSemaIndex = 0;
        recordState.open = false;
        delete [] recordState.buffer;
    }

    return true;
}

double snd_GetRecordingSampleRate(void)
{
    Synchronized<es::Monitor*> method(monitorRecordState);
    return  (double) recordState.samplingRate;
}

static int getCopyLen(int bufBytes, int offsetSlice)
{
    const int& bps = recordState.bytesPerSample;
    const int& outBps = recordState.outputBytesPerSample;

    int len = ((bufBytes - (offsetSlice * outBps)) / outBps) * bps;

    if (recordState.recorded < len)
    {
        len = recordState.recorded;
    }
    if (recordState.maxRecordSize < len)
    {
        len = recordState.maxRecordSize;
    }

    return len;
}

#define DSP_8BIT_SILENCE 128
static short Convert8To16(u8 value)
{
    return (((u16) value ) - DSP_8BIT_SILENCE) << 8;
}

static u8* CopyRecordedSamples(short* dst, u8* src, int len)
{
    const u8* recBuf = recordState.buffer;
    const u8* bufEnd = recBuf + recordState.bufSizeInBytes;
    int i;
    if (src + len <= bufEnd)
    {
        for (i = 0; i < len; i++)
        {
            dst[i] = Convert8To16(src[i]);
        }
        src += len;
    }
    else
    {
        int lenSrcToEnd = (bufEnd - src);
        for (i = 0; i < lenSrcToEnd; i++)
        {
            dst[i] = Convert8To16(src[i]);
        }
        for (i = 0; i < len - lenSrcToEnd; i++)
        {
            dst[i+lenSrcToEnd] = Convert8To16(recBuf[i]);
        }
        src = const_cast<u8*>(recBuf + (len - lenSrcToEnd));
    }

    if (src == bufEnd)
    {
        src = const_cast<u8*>(recBuf);
    }

    return src;
}

int snd_RecordSamplesIntoAtLength(int buf, int startSliceIndex, int bufferSizeInBytes)
{
    Synchronized<es::Monitor*> method(monitorRecordState);

    if (!recordState.inProgress)
    {
        return 0;
    }

    int copyLen = getCopyLen(bufferSizeInBytes, startSliceIndex);
    int offsetInBytes = startSliceIndex * recordState.outputBytesPerSample;

    short* dst = (short*) ((u8*) buf + offsetInBytes);
    u8* src = recordState.head;

    recordState.head = CopyRecordedSamples(dst, src, copyLen);
    recordState.recorded -= copyLen;
    return copyLen / recordState.bytesPerSample;
}

void snd_Volume(double *left, double *right) //johnmci@smalltalkconsulting.com Nov 6th 2000
{
}

void snd_SetVolume(double left, double right)//johnmci@smalltalkconsulting.com Nov 6th 2000
{
}

static void Setup(void)
{
    Synchronized<es::Monitor*> method(monitorPlayState);

    const u8* buf = playState.buffer;
    const u8* bufEnd = playState.buffer + playState.bufSizeInBytes;
    const u8* head = playState.head;
    const u8* tail = playState.tail;

    monitorPlaySize->lock();
    if (playState.size == 0)
    {
        playState.played = 0;
        // no sound data in the buffer
        monitorPlaySize->unlock();
        return;
    }
    monitorPlaySize->unlock();

    if (head == bufEnd)
    {
        //  buf                   bufEnd
        //  |XXXXXXX--------------|
        //          tail          head
        //
        //         |
        //         V
        //
        //  buf                   bufEnd
        //  |XXXXXXX--------------|
        //  head    tail
        head = buf;
        playState.head = const_cast<u8*>(head);
    }

    if (head < tail)
    {
        //  buf                   bufEnd
        //  |-------XXXXXXXX------|
        //          head    tail

        playState.played = tail - head;
    }
    else
    {
        //  buf                   bufEnd
        //  |XXXXX------------XXXX|
        //        tail        head
        playState.played = bufEnd - head;
    }

    ASSERT(playState.played <= bufEnd - buf);
    ASSERT(head + playState.played <= bufEnd);
    ASSERT(buf <= head);
}

static void MoveHead(int played)
{
    Synchronized<es::Monitor*> method(monitorPlayState);

    const u8* buf = playState.buffer;
    const u8* bufEnd = playState.buffer + playState.bufSizeInBytes;

    buf = playState.buffer;
    bufEnd = playState.buffer + playState.bufSizeInBytes;

    {
        Synchronized<es::Monitor*> method(monitorPlaySize);
        playState.size -= played;
    }

    if (playState.head + played <= bufEnd)
    {
        playState.head += played;
    }
    else
    {
        int len = (played - (bufEnd - playState.head));
        playState.head = (u8*) (buf + len);
    }
}

void* audioProcess(void* param)
{
    Handle<es::Context> root = System()->getRoot();
    gSoundOutput = root->lookup("device/soundOutput");

    Handle<es::CurrentThread> currentThread = System()->currentThread();

    monitorPlayState = System()->createMonitor();
    monitorPlaySize = System()->createMonitor();

    long len = 0;
    long offset;
    long n;
    for (;;)
    {
        monitorPlayState->lock();
        while (!playState.playing)
        {
            monitorPlayState->wait();
        }
        monitorPlayState->unlock();

        monitorPlaySize->lock();
        while (playState.size == 0)
        {
            monitorPlaySize->wait();
        }

        len = playState.size;
        monitorPlaySize->unlock();

        for (offset = 0; offset < len; offset += n)
        {
            Setup();
            n = gSoundOutput->write(playState.head, playState.played);
            if (n < 0)
            {
                break;
            }
            else if (n == 0)
            {
                continue;
            }

            MoveHead(n);
            synchronizedSignalSemaphoreWithIndex(playState.playSemaIndex);

            if (len <= offset + n)
            {
                break;
            }
        }
    }

    if (monitorPlayState)
    {
        monitorPlayState->release();
    }

    if (monitorPlaySize)
    {
        monitorPlaySize->release();
    }

    return 0;
}

void* recordProcess(void* param)
{
    Handle<es::Context> root = System()->getRoot();
    gSoundInput = root->lookup("device/soundInput");
    Handle<es::CurrentThread> currentThread = System()->currentThread();
    monitorRecordState = System()->createMonitor();
    monitorRecording = System()->createMonitor();
    {
        Synchronized<es::Monitor*> method(monitorRecordState);
        recordState.inProgress = false;
    }

    long len = 0;
    long offset;
    long n;
    for (;;)
    {
        monitorRecordState->lock();
        while (!recordState.inProgress)
        {
            monitorRecordState->wait();
        }

        recordState.next = recordState.buffer;
        recordState.nextRecordSize = recordState.maxRecordSize;

        monitorRecordState->unlock();

        for (;;)
        {
            Synchronized<es::Monitor*> method(monitorRecording);

            monitorRecordState->lock();
            if (!recordState.inProgress)
            {
                monitorRecordState->unlock();
                break;
            }
            monitorRecordState->unlock();

            n = gSoundInput->read(recordState.next, recordState.nextRecordSize);
            if (n < 0)
            {
                break;
            }
            else if (n == 0)
            {
                continue;
            }

            SetupRecordBuf(n);
            synchronizedSignalSemaphoreWithIndex(recordState.recordSemaIndex);
        }
    }

    if (monitorRecording)
    {
        monitorRecording->release();
    }
    if (monitorRecordState)
    {
        monitorRecordState->release();
    }

    return 0;
}
