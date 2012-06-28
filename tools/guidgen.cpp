/*
 * Copyright 2012 Esrille Inc. 
 * Copyright 2008 Google Inc.
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <es/uuid.h>
#include <es/dateTime.h>

// #define VERBOSE

static void PrintUuid(const Uuid* u)
{
    int i;

    printf("%8.8x-%4.4x-%4.4x-%2.2x%2.2x-",
             u->timeLow,
             u->timeMid,
             u->timeHiAndVersion,
             u->clockSeqHiAndReserved,
             u->clockSeqLow);
    for (i = 0; i < 6; i++)
    {
        printf("%2.2x", u->node[i]);
    }
    printf("\n");
}

static int GetTimeStamp(u64& timestamp)
{
    int ret;

    DateTime now(DateTime::getNow());
    DateTime bias(1582, 10, 15);

    timestamp = now.getTicks() - bias.getTicks();
    timestamp &= 0x0fffffffffffffffLLu;

    return 0;
}

static u16 GetClockSequence(void)
{
    return (u16) (gethostid() & 0x00003fff);
}

int main(int argc, char* argv[])
{
    u64 timestamp;
    u16 clockseq;
    u32 bias;
    u8  dotted[19];
    u8  node[6];
    Uuid uuid;

    if (argc != 2)
    {
        printf("Usage: guidgen MAC_ADDRESS\n");
        return 1;
    }

    sscanf(argv[1], "%2x:%2x:%2x:%2x:%2x:%2x", &node[0], &node[1], &node[2],
        &node[3], &node[4], &node[5]);

#ifdef VERBOSE
    printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        node[0], node[1], node[2], node[3], node[4], node[5]);
#endif // VERBOSE

    /* get time as 60 bit 100ns ticks since whenever. */
    if (GetTimeStamp(timestamp) < 0)
    {
        return 1;
    }
#ifdef VERBOSE
    printf("timestamp: %llx ", timestamp);
    DateTime epoch(1582, 10, 15);
    DateTime ts(epoch.getTicks() + timestamp);
    printf("(%d/%d/%d %02d:%02d:%02d.%03d)\n",
           ts.getYear(), ts.getMonth(), ts.getDay(),
           ts.getHour(), ts.getMinute(), ts.getSecond(), ts.getMillisecond());
#endif // VERBOSE

    /* get 14-bit clock sequence */
    clockseq = GetClockSequence();
#ifdef VERBOSE
    printf("clock sequence: %x\n", clockseq);
#endif // VERBOSE

    /* stuff fields into the UUID */
    /* Construct a version 1 uuid with the information we've gathered
     * plus a few constants. */
    uuid.timeLow = (u32) timestamp;
    uuid.timeMid = (u16) (timestamp >> 32);
    uuid.timeHiAndVersion = (u16) ((timestamp >> 48) & 0x0fff);
    uuid.timeHiAndVersion |= (1 << 12);
    uuid.clockSeqLow = (u8) clockseq;
    uuid.clockSeqHiAndReserved = (u8) ((clockseq & 0x3F00) >> 8);
    uuid.clockSeqHiAndReserved |= 0x80;
    memmove(&uuid.node, node, sizeof node);

    PrintUuid(&uuid);

    return 0;
}
