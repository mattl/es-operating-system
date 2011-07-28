/*
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

/*---------------------------------------------------------------------------*

  These coded instructions, statements, and computer programs contain
  software derived from Network Working Group Internet-Draft
  <draft-mealling-uuid-urn-03.txt>.

   ** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
   ** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
   ** Digital Equipment Corporation, Maynard, Mass.
   ** Copyright (c) 1998 Microsoft.
   ** To anyone who acknowledges that this file is provided "AS IS"
   ** without any express or implied warranty: permission to use, copy,
   ** modify, and distribute this file for any purpose is hereby
   ** granted without fee, provided that the above copyright notices and
   ** this notice appears in all source code copies, and that none of
   ** the names of Open Software Foundation, Inc., Hewlett-Packard
   ** Company, or Digital Equipment Corporation be used in advertising
   ** or publicity pertaining to distribution of the software without
   ** specific, written prior permission. Neither Open Software
   ** Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital
   ** Equipment Corporation makes any representations about the suitability
   ** of this software for any purpose.

 *---------------------------------------------------------------------------*/

#ifndef NINTENDO_ES_UUID_H_INCLUDED
#define NINTENDO_ES_UUID_H_INCLUDED

#include <es/types.h>

#ifdef WIN32
#include <guiddef.h>
typedef GUID Guid;
#else   // #ifdef WIN32

#ifdef __cplusplus
extern "C"{
#endif  // #ifdef __cplusplus

/** This structure is defined for the compatibility for COM.
 * @see Uuid
 */
struct _GUID
{
    u32 Data1;
    u16 Data2;
    u16 Data3;
    u8  Data4[8];
};
typedef struct _GUID GUID;
typedef struct _GUID Guid;

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus

// Guid helper functions
#ifdef __cplusplus

#include <functional>   // equal_to<>

const Guid GUID_NULL =
{
   0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 }
};

inline int operator==(const Guid& g1, const Guid& g2)
{
   return (((u32*) &g1)[0] == ((u32*) &g2)[0] &&
           ((u32*) &g1)[1] == ((u32*) &g2)[1] &&
           ((u32*) &g1)[2] == ((u32*) &g2)[2] &&
           ((u32*) &g1)[3] == ((u32*) &g2)[3]) ? true : false;
}

inline int operator!=(const Guid& g1, const Guid& g2)
{
    return !(g1 == g2);
}

namespace std
{
    template<>
    struct equal_to<const Guid&>
    {
        bool operator()(const Guid& x, const Guid& y) const
        {
            return (x == y);
        }
    };
}

#endif  // #ifdef __cplusplus

#endif  // #ifdef WIN32

#ifdef __cplusplus

/**
 * This represents a UUID (Universally Unique Identifier).
 */
struct Uuid
{
    u32 timeLow;
    u16 timeMid;
    u16 timeHiAndVersion;
    u8  clockSeqHiAndReserved;
    u8  clockSeqLow;
    u8  node[6];

    /** Compares UUID "lexically". Note lexical ordering is not temporal ordering.
     * @return -1 if lexically before, 0 if equal, 1 if lexically after.
     */
    int compareTo(const Uuid& u) const
    {
        if (timeLow != u.timeLow)
        {
            return (timeLow < u.timeLow) ? -1 : 1;
        }
        if (timeMid != u.timeMid)
        {
            return (timeMid < u.timeMid) ? -1 : 1;
        }
        if (timeHiAndVersion != u.timeHiAndVersion)
        {
            return (timeHiAndVersion < u.timeHiAndVersion) ? -1 : 1;
        }
        if (clockSeqHiAndReserved != u.clockSeqHiAndReserved)
        {
            return (clockSeqHiAndReserved < u.clockSeqHiAndReserved) ? -1 : 1;
        }
        if (clockSeqLow != u.clockSeqLow)
        {
            return (clockSeqLow < u.clockSeqLow) ? -1 : 1;
        }
        for (int i = 0; i < 6; i++)
        {
            if (node[i] < u.node[i])
            {
                 return -1;
            }
            if (node[i] > u.node[i])
            {
                return 1;
            }
        }
        return 0;
    }

    /** Conversion function to Guid.
     */
    operator const Guid&() const
    {
        return *reinterpret_cast<const Guid*>(this);
    }
};

#if 0

inline bool operator==(const Uuid& u1, const Uuid& u2)
{
    return (!u1.compareTo(u2)) ? true : false;
}

inline bool operator!=(const Uuid& u1, const Uuid& u2)
{
    return u1.compareTo(u2) ? true : false;
}

#endif

#endif  // #ifdef __cplusplus

#endif  // #ifndef NINTENDO_ES_UUID_H_INCLUDED
