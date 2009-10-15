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

#ifndef EVENTMANAGER_H_INCLUDED
#define EVENTMANAGER_H_INCLUDED

#include "IEventQueue.h"

/* generic input event definition */
struct InputEvent
{
    int type; /* type of event; either one of EventTypeXXX */
    unsigned int timeStamp; /* time stamp */
        /* the interpretation of the following fields depend on the type of the event */
    int unused1;
    int unused2;
    int unused3;
    int unused4;
    int unused5;
    int unused6;
};

/* mouse input event definition */
struct MouseEvent
{
    int type; /* EventTypeMouse */
    unsigned int timeStamp; /* time stamp */
    int x; /* mouse position x */
    int y; /* mouse position y */
    int buttons; /* combination of xxxButtonBit */
    int modifiers; /* combination of xxxKeyBit */
    int reserved1; /* reserved for future use */
    int reserved2; /* reserved for future use */
};

/* keyboard input event definition */
struct KeyboardEvent
{
    int type; /* EventTypeKeyboard */
    unsigned int timeStamp; /* time stamp */
    int charCode; /* character code in Mac Roman encoding */
    int pressCode; /* press code; any of EventKeyXXX */
    int modifiers; /* combination of xxxKeyBit */
    int reserved1; /* reserved for future use */
    int reserved2; /* reserved for future use */
    int reserved3; /* reserved for future use */
};

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char IEventQueueInfo[];
extern unsigned IEventQueueInfoSize;

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // EVENTMANAGER_H_INCLUDED
