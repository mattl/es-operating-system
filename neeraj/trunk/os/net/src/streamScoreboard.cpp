/*
 * Copyright 2008 Google Inc.
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
#include <algorithm>
#include <es/handle.h>
#include "stream.h"

const int StreamReceiver::RXMIT_THRESH = 3;

// Returns SackHole for the oldest pending retransmission.
StreamReceiver::SackHole* StreamReceiver::
getSackHole()
{
    const SackHole* hole;
    const SackHole* holeEnd;

    ASSERT(RXMIT_THRESH <= dupAcks);
    holeEnd = &scoreboard[sendHoles];
    for (hole = scoreboard; hole < holeEnd; ++hole)
    {
        if (hole->rxmit < hole->end && (fastRxmit || RXMIT_THRESH <= hole->dupAcks))
        {
            if (hole->rxmit < sendUna)    // Stale SACK hole
            {
                continue;
            }
            return (SackHole*) hole;
        }
    }
    return 0;
}

// Delete cumulatively ack'd holes.
void StreamReceiver::
deleteSackHoles(TCPSeq ack)
{
    TCPSeq      lastAck;
    SackHole*   hole;
    SackHole*   holeEnd;

    if (!sack || state == &stateListen || sendMax < ack)
    {
        return;
    }
    lastAck = std::max(sendUna, ack);
    holeEnd = &scoreboard[sendHoles];
    for (hole = scoreboard; hole < holeEnd; ++hole)
    {
        if (lastAck < hole->end)
        {
            break;
        }
    }
    if (scoreboard < hole)
    {
        sendHoles = holeEnd - hole;
        memmove(scoreboard, hole, (size_t) ((u8*) holeEnd - (u8*) hole));
    }
    if (0 < sendHoles)
    {
        hole = scoreboard;
        if (hole->start < lastAck)
        {
            hole->start = lastAck;
            if (hole->rxmit < hole->start)
            {
                hole->rxmit = hole->start;
            }
        }
    }
}

// Update the SACK scoreboard parsing the TCP SACK option.
void StreamReceiver::
updateScoreboard(TCPSeq ack, TCPOptSack* optSack)
{
    SackHole*   hole;
    SackHole*   holeEnd;
    TCPSeq      start; // left edge
    TCPSeq      end;   // right edge

    if (!sack || sendMax < ack || ack < sendUna)
    {
        return;
    }

    for (int i = 0; i < (optSack->len - 2) / 8; ++i)
    {
        start = ntohl(optSack->edge[i].left);
        end = ntohl(optSack->edge[i].right);
        if (end <= start)
            continue;   // bad SACK fields
        if (end <= sendUna)
            continue;   // old block
        if (start <= ack)
            continue;   // D-SACK [RFC 2883]
        if (sendMax < end)
            continue;   // bad SACK fields

        // Updates sendFack.
        if (sendFack < end)
        {
            sendFack = end;
        }

        if (sendHoles == 0)   // Scoreboard is empty.
        {
            sendHoles = 1;

            hole = &scoreboard[0];
            hole->start = ack;
            hole->end = start;
            ASSERT(hole->start < hole->end);
            hole->rxmit = hole->start;
            hole->dupAcks = std::min(RXMIT_THRESH, (end - hole->end) / mss);
            if (hole->dupAcks < 1)
            {
                hole->dupAcks = 1;
            }
            lastSack = end;
            continue;
        }

        holeEnd = &scoreboard[sendHoles];
        for (hole = scoreboard; hole < holeEnd; ++hole)
        {
            if (end <= hole->start)     // Left side
            {
                break;
            }
            if (hole->end <= start)     // Right side
            {
                ++hole->dupAcks;
                if (RXMIT_THRESH <= (end - hole->end) / mss)
                {
                    hole->dupAcks = RXMIT_THRESH;
                }
            }
            else if (start <= hole->start)  // Left edge
            {
                rxmitData -= std::min(hole->rxmit, end) - hole->start;
                if (hole->end <= end)       // Cover
                {
                    memmove(hole, hole + 1, (size_t) ((u8*) holeEnd - (u8*) (hole + 1)));
                    --holeEnd;
                    --sendHoles;
                    --hole;
                }
                else
                {
                    hole->start = end;
                    hole->rxmit = std::max(hole->rxmit, hole->start);
                }
            }
            else if (hole->end <= end)      // Right edge
            {
                if (start < hole->rxmit)
                {
                    rxmitData -= hole->rxmit - start;
                }
                hole->end = start;
                hole->rxmit = std::min(hole->rxmit, hole->end);
                ++hole->dupAcks;
                if (RXMIT_THRESH <= (end - hole->end) / mss)
                {
                    hole->dupAcks = RXMIT_THRESH;
                }
            }
            else                // In the middle
            {
                ASSERT(hole->start < start);
                ASSERT(end < hole->end);
                if (sendHoles < TCPHdr::ASB_MAX)
                {
                    memmove(hole + 1, hole, (size_t) ((u8*) holeEnd - (u8*) hole));
                    ++holeEnd;
                    ++sendHoles;
                }
                else if (hole < holeEnd - 1)
                {
                    lastSack = (holeEnd - 1)->start;
                    memmove(hole + 1, hole, (size_t) ((u8*) (holeEnd - 1) - (u8*) hole));
                }
                else
                {
                    lastSack = end;
                }
                if (end < hole->rxmit)
                {
                    rxmitData -= end - start;
                }
                else if (start < hole->rxmit)
                {
                    rxmitData -= hole->rxmit - start;
                }
                hole->end = start;
                hole->rxmit = std::min(hole->rxmit, hole->end);
                ++hole->dupAcks;
                if (RXMIT_THRESH <= (end - hole->end) / mss)
                {
                    hole->dupAcks = RXMIT_THRESH;
                }
                ++hole;
                if (hole < holeEnd)
                {
                    hole->start = end;
                    hole->rxmit = std::max(hole->rxmit, hole->start);
                }
                // else XXX rxmitData would no longer be correct... timeout should recover this...
            }
        }
        if (lastSack < start)   // Append new hole at end.
        {
            if (TCPHdr::ASB_MAX <= sendHoles)
            {
                continue;
            }
            hole = &scoreboard[sendHoles];
            ++sendHoles;
            hole->start = lastSack;
            hole->end = start;
            hole->dupAcks = std::min(RXMIT_THRESH, (end - start) / mss);
            if (hole->dupAcks < 1)
            {
                hole->dupAcks = 1;
            }
            hole->rxmit = hole->start;
            lastSack = end;
        }
    }

    // Update rxmitData and sendAwin.
    rxmitData = 0;
    holeEnd = &scoreboard[sendHoles];
    for (hole = scoreboard; hole < holeEnd; ++hole)
    {
        rxmitData += hole->rxmit - hole->start;
    }
    sendAwin = (sendNext - sendFack) + rxmitData;
}
